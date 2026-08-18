#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/concat.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinor.h>
#include "jz_sfc_common.h"
#include "jz_sfc_concat.h"
#ifdef CONFIG_BURNER
#include <cloner/cloner.h>
#endif

#ifdef CONFIG_SFC_NOR_CONCAT
#define SFC_NOR_SR_WIP			(1U << 0)
#define SFC_NOR_WRITE_TIMEOUT_MS	3000
#define SFC_NOR_ERASE_TIMEOUT_MS	15000

extern struct sfc_flash *flash;
extern struct burner_params params;
extern unsigned int sfc_params_addr;

void sfc_clk_set(struct sfc *sfc, uint32_t sfc_rate);
int sfc_nor_flash_init(void);
unsigned int sfc_nor_read_id(void);
unsigned int sfc_nor_read_params(unsigned int addr, unsigned char *buf,
				 unsigned int len);
int sfc_nor_reset(void);
int sfc_read(unsigned int from, unsigned int len, unsigned char *buf);
unsigned int sfc_do_write(unsigned int addr, unsigned int len,
			  unsigned char *buf);
int sfc_do_erase(uint32_t addr);
void sfc_nor_clear_status(struct sfc_flash *flash);
void sfc_nor_do_special_func_internal(int disable_quad);
void sfc_nor_create_cdt_table(struct sfc_flash *flash, uint32_t flag);
#ifdef CONFIG_BURNER
int sfc_nor_partition_erase(void);

static int sfc_nor_concat_params_valid(
		const struct spiflash_info *concat_info);

int sfc_nor_concat_inject_burner_params(uint32_t offset, uint32_t len,
					unsigned char *buf)
{
	const struct spiflash_info *info;
	const struct burner_params *concat_params;
	size_t param_size = sizeof(*info);
	uint32_t param_offset = CONFIG_SPIFLASH_PART_OFFSET;

	if (!spi_args || !buf || spi_args->download_params == 0)
		return 0;

	info = (const struct spiflash_info *)
		((unsigned char *)spi_args + sizeof(struct spi_param));
	concat_params = &info->burner_params;
	if (concat_params->magic != NOR_MAGIC ||
	    concat_params->version != NOR_CONCAT_VERSION)
		return 0;

	if (!sfc_nor_concat_params_valid(info))
		return -EINVAL;

	if ((int)spi_args->param_offset > 0)
		param_offset = spi_args->param_offset;

	if (offset != 0)
		return 0;

	if (param_offset + param_size > len) {
		LOG_ERROR("SF: concat params not covered by uboot image, off=%#x size=%zu len=%#x\n",
			  param_offset, param_size, len);
		return -EINVAL;
	}

	memcpy(buf + param_offset, info, param_size);
	LOG_INFO("SF: concat params injected into flash0 image @ %#x size=%zu\n",
		 param_offset, param_size);
	return 1;
}
#endif

struct sfc_nor_runtime_chip {
	const struct sfc_flash_concat_chip *desc;
	struct spi_nor_info info;
	struct jz_sfc_concat_mtd_child child_mtd;
	struct sfc_flash_concat_segment segment;
	uint32_t jedec_id;
	int valid;
	int quad_succeed;
	uint8_t current_die_id;
	uint32_t die_shift;
	uint32_t die_num;
};

static struct sfc_nor_runtime_chip
	nor_concat_chips[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
static struct sfc_flash_concat_segment
	nor_concat_segments[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
static uint32_t nor_concat_chip_count;
static uint32_t nor_concat_total_size;
static int nor_concat_enabled;
static int nor_concat_active_chip = -1;
static struct mtd_info *nor_concat_subdevs[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
static struct mtd_info *nor_concat_mtd;

static void sfc_nor_child_mtd_init(struct sfc_nor_runtime_chip *chip);
static int sfc_nor_install_mtdconcat_bridge(void);

#ifndef CONFIG_BURNER
#ifdef CONFIG_NOR_COMMON_PARAMS
enum {
	SFC_NOR_CONCAT_COMMON_ID_CACHE_MAX = 128,
};

struct sfc_nor_concat_common_entry {
	uint32_t nor_id;
	struct mini_spi_nor_info mini;
};

static uint32_t sfc_nor_concat_default_chip_size(uint32_t jedec_id)
{
	uint32_t capacity = jedec_id & 0xff;

	if (capacity < 0x14 || capacity >= 32)
		return 0;

	return 1U << capacity;
}
#endif

static uint32_t sfc_nor_concat_erase_cmd_by_size(uint32_t erase_size)
{
	switch (erase_size) {
	case 0x1000:
		return 0x20;
	case 0x8000:
		return 0x52;
	case 0x10000:
		return 0xd8;
	default:
		return 0;
	}
}

struct sfc_nor_concat_param_cache {
	struct spi_nor_info base_info;
#ifdef CONFIG_NOR_COMMON_PARAMS
	struct sfc_nor_concat_common_entry
		entries[SFC_NOR_CONCAT_COMMON_ID_CACHE_MAX];
	uint32_t entry_count;
#endif
};

static struct sfc_nor_concat_param_cache nor_concat_param_cache;
#endif

static int sfc_nor_concat_child_read(struct jz_sfc_concat_mtd_child *child,
				     loff_t from, size_t len,
				     size_t *retlen, u_char *buf);
static int sfc_nor_concat_child_write(struct jz_sfc_concat_mtd_child *child,
				      loff_t to, size_t len,
				      size_t *retlen, const u_char *buf);
static int sfc_nor_concat_child_erase(struct jz_sfc_concat_mtd_child *child,
				      struct erase_info *instr);
static const struct jz_sfc_concat_mtd_child_ops sfc_nor_child_mtd_ops = {
	.read = sfc_nor_concat_child_read,
	.write = sfc_nor_concat_child_write,
	.erase = sfc_nor_concat_child_erase,
};
#endif

//#define SFC_NOR_CLONER_DEBUG

#if defined(CONFIG_SFC_NOR_CONCAT)
int sfc_nor_concat_select_boot_chip(void)
{
#ifdef CONFIG_BURNER
	const struct sfc_flash_concat_context *ctx =
		jz_sfc_flash_concat_context();
	const struct sfc_flash_concat_chip *chips;
	uint32_t count = 0;
	int have_concat_topology;

	have_concat_topology = ctx && ctx->chips && ctx->chip_count;
	chips = board_sfc_flash_concat_chips(&count);
	have_concat_topology = have_concat_topology || (chips && count);
	if (!have_concat_topology)
		return 0;
#endif

	return jz_sfc_flash_concat_select_boot_chip();
}

int sfc_nor_read_chip_id(uint32_t chip_index, uint32_t *jedec_id)
{
	const struct sfc_flash_concat_context *ctx;
	uint32_t nor_id;
	int restore_boot_chip = 0;
	int ret = 0;

	if (!jedec_id)
		return -EINVAL;

	if (!flash && sfc_nor_flash_init())
		return -EIO;

	ctx = jz_sfc_flash_concat_context();
	if (ctx && ctx->chips && ctx->chip_count) {
		if (chip_index >= ctx->chip_count)
			return -EINVAL;
		if (jz_sfc_flash_concat_select_chip(chip_index))
			return -EIO;
		restore_boot_chip = 1;
	} else if (chip_index) {
		return -EINVAL;
	}

	sfc_nor_reset();
	nor_id = sfc_nor_read_id();
	if (!nor_id || nor_id == (uint32_t)-EIO) {
		ret = -EIO;
		goto out;
	}

	*jedec_id = nor_id;
out:
	if (restore_boot_chip && jz_sfc_flash_concat_select_boot_chip() && !ret)
		ret = -EIO;

	return ret;
}

#ifndef CONFIG_BURNER
static void sfc_nor_concat_mini_to_full(const struct mini_spi_nor_info *mini,
					struct spi_nor_info *full)
{
	memset(full, 0, sizeof(*full));
	memcpy(full->name, mini->name, sizeof(full->name));
	full->id = mini->id;
	full->read_standard = mini->read_standard;
	full->read_quad = mini->read_quad;
	full->write_standard.cmd = SPINOR_OP_PP;
	full->write_standard.dummy_byte = 0;
	full->write_standard.addr_nbyte = mini->read_standard.addr_nbyte;
	full->write_standard.transfer_mode = TM_STD_SPI;
	full->write_quad.cmd = SPINOR_OP_QPP;
	full->write_quad.dummy_byte = 0;
	full->write_quad.addr_nbyte = mini->read_quad.addr_nbyte;
	full->write_quad.transfer_mode = mini->read_quad.transfer_mode;
	full->sector_erase.cmd =
		sfc_nor_concat_erase_cmd_by_size(mini->erase_size);
	full->sector_erase.addr_nbyte = mini->read_standard.addr_nbyte;
	full->sector_erase.transfer_mode = TM_STD_SPI;
	if (!full->sector_erase.cmd)
		full->sector_erase = nor_concat_param_cache.base_info.sector_erase;
	full->wr_en = mini->wr_en;
	full->en4byte = mini->en4byte;
	full->quad_set = mini->quad_set;
	full->quad_get = mini->quad_get;
	full->busy = mini->busy;
	full->quad_ops_mode = mini->quad_ops_mode;
	full->addr_ops_mode = mini->addr_ops_mode;
	full->tCHSH = nor_concat_param_cache.base_info.tCHSH ?
		      nor_concat_param_cache.base_info.tCHSH : DEF_TCHSH;
	full->tSLCH = nor_concat_param_cache.base_info.tSLCH ?
		      nor_concat_param_cache.base_info.tSLCH : DEF_TSLCH;
	full->tSHSL_RD = nor_concat_param_cache.base_info.tSHSL_RD ?
			 nor_concat_param_cache.base_info.tSHSL_RD : DEF_TSHSL_R;
	full->tSHSL_WR = nor_concat_param_cache.base_info.tSHSL_WR ?
			 nor_concat_param_cache.base_info.tSHSL_WR : DEF_TSHSL_W;
	full->chip_size = mini->chip_size;
	full->page_size = mini->page_size;
	full->erase_size = mini->erase_size;
	full->chip_erase_cmd = nor_concat_param_cache.base_info.chip_erase_cmd ?
			       nor_concat_param_cache.base_info.chip_erase_cmd :
			       SPINOR_OP_CHIP_ERASE;
}

static int sfc_nor_concat_cache_static_params(void)
{
	memcpy(&nor_concat_param_cache.base_info, &params.spi_nor_info,
	       sizeof(nor_concat_param_cache.base_info));

#ifdef CONFIG_NOR_COMMON_PARAMS
	{
		struct mini_spi_nor_info common[CONFIG_NOR_COMMON_PARAMS_COUNT];
		uint32_t common_offset;
		uint32_t common_len;
		uint32_t info_offset;
		uint32_t info_len;
		uint32_t i;
		uint32_t common_count = CONFIG_NOR_COMMON_PARAMS_COUNT;

		nor_concat_param_cache.entry_count = 0;
		common_offset = sfc_params_addr + sizeof(struct builtin_params);
		common_len = sizeof(common);
		if (sfc_nor_read_params(common_offset,
					(unsigned char *)common,
					common_len) != common_len)
			return -EIO;

		info_len = sizeof(struct nor_id_info) - sizeof(struct nor_id *);
		info_offset = common_offset + common_len;

		for (i = 0; i < CONFIG_NOR_COMMON_PARAMS_COUNT; i++) {
			struct nor_id_info info;
			uint32_t list_offset;
			uint32_t j;
			const struct mini_spi_nor_info *mini_common = NULL;

			memset(&info, 0, sizeof(info));
			if (sfc_nor_read_params(info_offset,
						(unsigned char *)&info,
						info_len) != info_len)
				return -EIO;

			for (j = 0; j < common_count; j++) {
				if (common[j].id == info.cmd_type) {
					mini_common = &common[j];
					break;
				}
			}
			if (!mini_common)
				return -ENOENT;

			list_offset = info_offset + info_len;
			for (j = 0; j < info.id_count; j++) {
				struct nor_id id;
				struct sfc_nor_concat_common_entry *entry;
				uint32_t chip_size;

				if (sfc_nor_read_params(list_offset +
							j * sizeof(id),
							(unsigned char *)&id,
							sizeof(id)) != sizeof(id))
					return -EIO;

				if (nor_concat_param_cache.entry_count >=
				    SFC_NOR_CONCAT_COMMON_ID_CACHE_MAX) {
					return -E2BIG;
				}

				entry = &nor_concat_param_cache.entries[
					nor_concat_param_cache.entry_count++];
				memcpy(&entry->mini, mini_common,
				       sizeof(entry->mini));
				entry->nor_id = id.id;
				entry->mini.id = id.id;
				chip_size = sfc_nor_concat_default_chip_size(id.id);
				if (chip_size)
					entry->mini.chip_size = chip_size;
				if (entry->mini.chip_size > 0x1000000) {
					entry->mini.read_standard.addr_nbyte = 4;
					entry->mini.read_quad.addr_nbyte = 4;
					if (!entry->mini.en4byte.cmd)
						entry->mini.en4byte.cmd = SPINOR_OP_EN4B;
				}
			}

			info_offset = list_offset + sizeof(struct nor_id) *
				      info.id_count;
		}
	}
#endif

	return 0;
}

static int sfc_nor_concat_resolve_static_info(uint32_t nor_id,
					      struct spi_nor_info *info)
{
	struct mini_spi_nor_info mini;
#ifdef CONFIG_NOR_COMMON_PARAMS
	uint32_t i;
#endif

	if (nor_id == nor_concat_param_cache.base_info.id) {
		memcpy(info, &nor_concat_param_cache.base_info, sizeof(*info));
		return 0;
	}

#ifdef CONFIG_NOR_COMMON_PARAMS
	for (i = 0; i < nor_concat_param_cache.entry_count; i++) {
		if (nor_concat_param_cache.entries[i].nor_id != nor_id)
			continue;

		memcpy(&mini, &nor_concat_param_cache.entries[i].mini,
		       sizeof(mini));
		sfc_nor_concat_mini_to_full(&mini, info);
		return 0;
	}
#endif

	return -ENOENT;
}
#endif

static int sfc_nor_concat_read_status(unsigned char *status)
{
	struct sfc_cdt_xfer xfer;

	if (!flash || !status)
		return -EINVAL;

	memset(&xfer, 0, sizeof(xfer));
	xfer.cmd_index = NOR_GET_STATUS;
	xfer.dataen = ENABLE;
	xfer.config.datalen = 1;
	xfer.config.data_dir = GLB_TRAN_DIR_READ;
	xfer.config.ops_mode = CPU_OPS;
	xfer.config.buf = status;

	if (sfc_sync_cdt(flash->sfc, &xfer)) {
		printf("sfc_sync_cdt error ! %s %s %d\n",
		       __FILE__, __func__, __LINE__);
		return -EIO;
	}

	return xfer.config.cur_len >= 1 ? 0 : -EIO;
}

static int sfc_nor_concat_wait_ready(unsigned int timeout_ms)
{
	ulong start = get_timer(0);
	unsigned char status = SFC_NOR_SR_WIP;
	unsigned int polls = 0;
	int ret;

	do {
		ret = sfc_nor_concat_read_status(&status);
		if (ret)
			return ret;

		polls++;
		if (!(status & SFC_NOR_SR_WIP))
			return 0;

		udelay(1000);
	} while (get_timer(start) <= timeout_ms);

	ret = sfc_nor_concat_read_status(&status);
	if (!ret && !(status & SFC_NOR_SR_WIP))
		return 0;

	printf("sfc nor wait ready timeout, status=0x%02x polls=%u elapsed=%lu\n",
	       status, polls, get_timer(start));
	return -ETIMEDOUT;
}

static int sfc_nor_concat_check_write_len(int ret, unsigned int expected,
					  unsigned int addr)
{
	if (ret < 0)
		return ret;
	if (ret < expected || ret > ALIGN(expected, 4)) {
		printf("sfc nor write length mismatch addr=%x len=%x ret=%x\n",
		       addr, expected, ret);
		return -EIO;
	}

	return 0;
}

static int sfc_nor_concat_page_write_wait(unsigned int to, unsigned int len,
					  unsigned char *buf)
{
	struct spi_nor_info *spi_nor_info;
	unsigned int page_offset;
	unsigned int actual_len;
	int writesize;
	int ret;
	u32 i;

	if (!len)
		return 0;

	spi_nor_info = flash->g_nor_info;
	writesize = spi_nor_info->page_size;
	page_offset = to & (spi_nor_info->page_size - 1);
	if (page_offset + len <= spi_nor_info->page_size) {
		ret = sfc_do_write(to, len, buf);
		ret = sfc_nor_concat_check_write_len(ret, len, to);
		if (ret)
			return ret;
		return sfc_nor_concat_wait_ready(SFC_NOR_WRITE_TIMEOUT_MS);
	}

	actual_len = spi_nor_info->page_size - page_offset;
	ret = sfc_do_write(to, actual_len, buf);
	ret = sfc_nor_concat_check_write_len(ret, actual_len, to);
	if (ret)
		return ret;
	ret = sfc_nor_concat_wait_ready(SFC_NOR_WRITE_TIMEOUT_MS);
	if (ret)
		return ret;

	for (i = actual_len; i < len; i += writesize) {
		actual_len = len - i;
		if (actual_len >= writesize)
			actual_len = writesize;

		ret = sfc_do_write(to + i, actual_len, buf + i);
		ret = sfc_nor_concat_check_write_len(ret, actual_len, to + i);
		if (ret)
			return ret;
		ret = sfc_nor_concat_wait_ready(SFC_NOR_WRITE_TIMEOUT_MS);
		if (ret)
			return ret;
	}

	return 0;
}

static int sfc_nor_concat_prepare_profile(void *priv)
{
	struct sfc_nor_runtime_chip *chip = priv;

	if (!flash || !chip || !chip->valid)
		return -EINVAL;

	flash->g_nor_info = &chip->info;
	flash->quad_succeed = chip->quad_succeed;
	flash->current_die_id = chip->current_die_id;
	flash->die_shift = chip->die_shift;
	flash->die_num = chip->die_num ? chip->die_num : 1;

	sfc_nor_create_cdt_table(flash, UPDATE_CDT);
	set_flash_timing(flash->sfc, chip->info.tCHSH, chip->info.tSLCH,
			 chip->info.tSHSL_RD, chip->info.tSHSL_WR);

	return sfc_nor_concat_wait_ready(SFC_NOR_WRITE_TIMEOUT_MS);
}

static int sfc_nor_concat_activate(uint32_t index)
{
	struct sfc_nor_runtime_chip *chip;
	int ret;

	if (index >= nor_concat_chip_count || !nor_concat_chips[index].valid)
		return -EINVAL;

	if (nor_concat_active_chip >= 0 &&
	    nor_concat_active_chip < nor_concat_chip_count) {
		chip = &nor_concat_chips[nor_concat_active_chip];
		chip->quad_succeed = flash->quad_succeed;
		chip->current_die_id = flash->current_die_id;
		chip->die_shift = flash->die_shift;
		chip->die_num = flash->die_num;
	}

	ret = jz_sfc_flash_concat_activate(index);
	if (ret)
		return ret;

	nor_concat_active_chip = index;
	return 0;
}

static int sfc_nor_concat_probe_chip_id(uint32_t index,
					const struct sfc_flash_concat_chip *desc)
{
	struct sfc_nor_runtime_chip *chip = &nor_concat_chips[index];
	uint32_t nor_id;
	int ret;

	if (!desc) {
		return -EINVAL;
	}

	memset(chip, 0, sizeof(*chip));
	chip->desc = desc;

	ret = jz_sfc_flash_concat_select_chip(index);
	if (ret)
		return -EIO;

	sfc_nor_reset();
	nor_id = sfc_nor_read_id();

	chip->jedec_id = nor_id;
	return 0;
}

#ifndef CONFIG_BURNER
static int sfc_nor_concat_init_static_chip(uint32_t index,
					   uint32_t linear_base)
{
	struct sfc_nor_runtime_chip *chip = &nor_concat_chips[index];
	const struct sfc_flash_concat_chip *desc = chip->desc;
	uint32_t nor_id = chip->jedec_id;
	int ret;

	if (sfc_nor_concat_resolve_static_info(nor_id, &chip->info)) {
		printf("sfc nor concat chip%d cannot resolve params id=%x\n",
		       index, nor_id);
		return -ENOENT;
	}

	if (!chip->info.chip_size || !chip->info.page_size ||
	    !chip->info.erase_size) {
		printf("sfc nor concat chip%d invalid geometry id=%x\n",
		       index, nor_id);
		return -EINVAL;
	}

	chip->jedec_id = nor_id;
	chip->segment.chip_index = index;
	chip->segment.linear_base = linear_base;
	chip->segment.chip_size = chip->info.chip_size;

	if (jz_sfc_flash_concat_select_chip(index))
		return -EIO;

	flash->g_nor_info = &chip->info;
	sfc_nor_create_cdt_table(flash, UPDATE_CDT);
	sfc_clk_set(flash->sfc, CONFIG_SFC_NOR_RATE);
	sfc_nor_do_special_func_internal(desc->flags &
					 SFC_FLASH_CONCAT_F_DISABLE_QUAD);
	chip->quad_succeed = flash->quad_succeed;
	chip->current_die_id = flash->current_die_id;
	chip->die_shift = flash->die_shift;
	chip->die_num = flash->die_num;
	chip->valid = 1;
	sfc_nor_child_mtd_init(chip);

	ret = jz_sfc_flash_concat_set_profile(index,
					      sfc_nor_concat_prepare_profile,
					      chip);
	if (ret)
		return ret;

	debug("sfc nor concat chip%d id=%x size=%x base=%x quad=%s\n",
	      index, nor_id, chip->info.chip_size, linear_base,
	      chip->quad_succeed ? "on" : "off");

	return 0;
}
#endif

static int sfc_nor_spi_info_valid(const struct spi_nor_info *info)
{
	return info && info->id && info->chip_size &&
		info->page_size && info->erase_size;
}

static const struct spi_nor_info *sfc_nor_concat_params_chip_info(
		const struct spiflash_info *concat_info,
		uint32_t chip_index, uint32_t *linear_base)
{
	const struct burner_params *concat_params;
	const struct nor_concat_extension *concat;
	const struct burner_concat_chip_info *chip;

	if (!concat_info)
		return NULL;

	concat_params = &concat_info->burner_params;
	if (chip_index == 0) {
		if (linear_base)
			*linear_base = 0;
		return &concat_params->spi_nor_info;
	}

	concat = &concat_info->concat;
	if (chip_index >= concat->chip_count)
		return NULL;

	chip = &concat->chip[chip_index - 1];
	if (linear_base)
		*linear_base = chip->linear_base;
	return &chip->spi_nor_info;
}

static int sfc_nor_concat_params_valid(
		const struct spiflash_info *concat_info)
{
	const struct burner_params *concat_params;
	const struct nor_concat_extension *concat;
	const struct spi_nor_info *info;
	uint32_t total;
	uint32_t i;

	if (!concat_info)
		return 0;
	concat_params = &concat_info->burner_params;
	if (concat_params->magic != NOR_MAGIC ||
	    concat_params->version != NOR_CONCAT_VERSION)
		return 0;

	concat = &concat_info->concat;
	if (concat->chip_count < 2 ||
	    concat->chip_count > NOR_CONCAT_CHIP_MAX ||
	    concat->chip_count > CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS)
		return 0;
	if (concat_params->norflash_partitions.num_partition_info > NOR_PART_NUM)
		return 0;

	info = sfc_nor_concat_params_chip_info(concat_info, 0, NULL);
	if (!sfc_nor_spi_info_valid(info))
		return 0;

	total = info->chip_size;
	for (i = 1; i < concat->chip_count; i++) {
		uint32_t linear_base;

		info = sfc_nor_concat_params_chip_info(concat_info, i,
						       &linear_base);
		if (linear_base != total)
			return 0;
		if (!sfc_nor_spi_info_valid(info))
			return 0;
		if (info->chip_size > (uint32_t)~0 - total)
			return 0;

		total += info->chip_size;
	}

	return total == concat->total_size;
}

static int sfc_nor_concat_load_partitions(
		const struct spiflash_info *concat_info)
{
	struct norflash_partitions *parts = flash->norflash_partitions;
	const struct burner_params *concat_params;
	const struct norflash_partitions *src_parts;
	uint32_t i;

	if (!parts || !sfc_nor_concat_params_valid(concat_info))
		return -EINVAL;

	concat_params = &concat_info->burner_params;
	src_parts = &concat_params->norflash_partitions;
	memset(parts, 0, sizeof(*parts));
	parts->num_partition_info = src_parts->num_partition_info;

	for (i = 0; i < src_parts->num_partition_info; i++) {
		const struct nor_partition *src = &src_parts->nor_partition[i];
		struct nor_partition *dst = &parts->nor_partition[i];
		uint32_t end;

		if (sfc_flash_concat_range_fits_u32_64(
					src->offset, src->size,
					concat_info->concat.total_size, &end))
			return -EINVAL;

		memcpy(dst->name, src->name, sizeof(dst->name));
		dst->offset = src->offset;
		dst->size = src->size;
		dst->mask_flags = src->mask_flags;
		dst->manager_mode = src->manager_mode;
	}

	return 0;
}

static int sfc_nor_concat_setup_from_params_common(
		const struct spiflash_info *concat_info, int burner)
{
	const struct sfc_flash_concat_context *ctx;
	const struct burner_params *concat_params;
	const struct nor_concat_extension *concat;
	uint32_t part_count;
	uint32_t i;
	int ret;

	if (!flash || !sfc_nor_concat_params_valid(concat_info))
		return -EINVAL;

	concat_params = &concat_info->burner_params;
	concat = &concat_info->concat;
	part_count = concat_params->norflash_partitions.num_partition_info;

	if (jz_sfc_flash_concat_init())
		return -EINVAL;

	ctx = jz_sfc_flash_concat_context();
	if (!ctx || !ctx->chips || ctx->chip_count < concat->chip_count)
		return -EINVAL;

	memset(nor_concat_chips, 0, sizeof(nor_concat_chips));
	memset(nor_concat_segments, 0, sizeof(nor_concat_segments));
	nor_concat_chip_count = 0;
	nor_concat_total_size = 0;
	nor_concat_enabled = 0;
	nor_concat_active_chip = -1;
	nor_concat_mtd = NULL;

#ifndef CONFIG_BURNER
	if (!burner) {
		params.magic = NOR_MAGIC;
		params.version = NOR_VERSION;
		params.uk_quad = 1;
		flash->norflash_partitions = &params.norflash_partitions;
	}
#endif

	for (i = 0; i < concat->chip_count; i++) {
		const struct spi_nor_info *arg_info;
		const struct sfc_flash_concat_chip *desc =
			jz_sfc_flash_concat_chip(i);
		struct sfc_nor_runtime_chip *chip = &nor_concat_chips[i];
		struct sfc_flash_concat_segment segment;
		uint32_t next_total = nor_concat_total_size;
		uint32_t linear_base;

		arg_info = sfc_nor_concat_params_chip_info(concat_info, i,
							   &linear_base);
		if (!sfc_nor_spi_info_valid(arg_info))
			return -EINVAL;

		ret = sfc_nor_concat_probe_chip_id(i, desc);
		if (ret)
			return ret;

		if (chip->jedec_id != arg_info->id) {
			printf("sfc nor concat chip%u params id mismatch probe=%x params=%x\n",
			       i, chip->jedec_id, arg_info->id);
			return -ENODEV;
		}

		memcpy(&chip->info, arg_info, sizeof(chip->info));
		if (!sfc_nor_spi_info_valid(&chip->info))
			return -EINVAL;

		chip->segment.chip_index = i;
		chip->segment.linear_base = linear_base;
		chip->segment.chip_size = chip->info.chip_size;
		ret = sfc_flash_concat_append_checked_segment(&segment,
				&chip->segment, i, &next_total);
		if (ret)
			return -EINVAL;

		if (jz_sfc_flash_concat_select_chip(i))
			return -EIO;

		flash->g_nor_info = &chip->info;
		sfc_nor_create_cdt_table(flash, UPDATE_CDT);
#ifdef CONFIG_BURNER
		if (burner && spi_args->sfc_frequency)
			sfc_clk_set(flash->sfc, spi_args->sfc_frequency);
		else
#endif
			sfc_clk_set(flash->sfc, CONFIG_SFC_NOR_RATE);

#ifdef CONFIG_BURNER
		if (burner)
			sfc_nor_clear_status(flash);
#endif
		sfc_nor_do_special_func_internal(desc ? desc->flags &
			SFC_FLASH_CONCAT_F_DISABLE_QUAD : 0);

		chip->quad_succeed = flash->quad_succeed;
		chip->current_die_id = flash->current_die_id;
		chip->die_shift = flash->die_shift;
		chip->die_num = flash->die_num;
		chip->valid = 1;
		sfc_nor_child_mtd_init(chip);

		ret = jz_sfc_flash_concat_set_profile(i,
				sfc_nor_concat_prepare_profile, chip);
		if (ret)
			return ret;

		nor_concat_segments[i] = segment;
		nor_concat_total_size = next_total;

		debug("sfc nor %s concat chip%u id=%x size=%x base=%x quad=%s\n",
		      burner ? "burner" : "params", i, chip->jedec_id,
		      chip->segment.chip_size, chip->segment.linear_base,
		      chip->quad_succeed ? "on" : "off");
	}

	if (nor_concat_total_size != concat->total_size)
		return -EINVAL;

	nor_concat_chip_count = concat->chip_count;
	ret = sfc_nor_install_mtdconcat_bridge();
	if (ret)
		return ret;
	nor_concat_enabled = 1;

	ret = sfc_nor_concat_load_partitions(concat_info);
	if (ret)
		return ret;

	ret = sfc_nor_concat_activate(0);
	if (ret)
		return ret;

	memcpy(&params.spi_nor_info, &nor_concat_chips[0].info,
	       sizeof(params.spi_nor_info));
	params.magic = NOR_MAGIC;
	params.version = NOR_VERSION;

#ifdef CONFIG_BURNER
	if (burner) {
		memcpy(&params.norflash_partitions, flash->norflash_partitions,
		       sizeof(params.norflash_partitions));
		params.fs_erase_size = spi_args->spi_erase_block_size;
		params.uk_quad = spi_args->sfc_quad_mode;
	} else
#endif
	{
		params.fs_erase_size = nor_concat_chips[0].info.erase_size;
	}

	debug("sfc nor %s concat enabled chips=%u total=%x parts=%u\n",
	      burner ? "burner" : "params", nor_concat_chip_count,
	      nor_concat_total_size, part_count);

	return 0;
}

#ifdef CONFIG_BURNER
static int sfc_nor_concat_chip_erase_all(void)
{
	uint32_t i;
	int ret;

	for (i = 0; i < nor_concat_chip_count; i++) {
		ret = sfc_nor_concat_activate(i);
		if (ret)
			return ret;

		ret = jz_sfc_chip_erase();
		if (ret)
			return ret;
	}

	return 0;
}

int mtd_sfcnor_probe_burner_concat(
		const struct spiflash_info *concat_info)
{
	int ret;

	if (!sfc_nor_concat_params_valid(concat_info))
		return -EINVAL;

	if (!flash && sfc_nor_flash_init())
		return -EIO;

	ret = sfc_nor_concat_setup_from_params_common(concat_info, 1);
	if (ret)
		return ret;

	if (spi_args->spi_erase != PART_ERASE)
		LOG_INFO("chip eraseing ... ");

	switch (spi_args->spi_erase) {
	case CHIP_ERASE:
		ret = sfc_nor_partition_erase();
		break;
	case FORCE_ERASE:
		ret = sfc_nor_concat_chip_erase_all();
		break;
	default:
		ret = 0;
		break;
	}

	if (spi_args->spi_erase != PART_ERASE)
		LOG_INFO("%s\n", ret == 0 ? "successful\n" : "failed\n");

	return ret;
}
#endif

#ifndef CONFIG_BURNER
int sfc_nor_concat_load_persistent_params(
		struct spiflash_info *concat_info)
{
	const struct burner_params *concat_params;

	if (!concat_info)
		return -EINVAL;

	/*
	 * Persistent concat parameters are written to the boot chip. Runtime
	 * probe can switch the active chip while reading IDs, so always select
	 * chip0 before reading the parameter area.
	 */
	if (jz_sfc_flash_concat_select_chip(0))
		return -EIO;

	if (sfc_nor_read_params(sfc_params_addr,
				(unsigned char *)concat_info,
				sizeof(*concat_info)) !=
	    sizeof(*concat_info))
		return -EIO;

	concat_params = &concat_info->burner_params;
	if (concat_params->magic != NOR_MAGIC ||
	    concat_params->version != NOR_CONCAT_VERSION)
		return 0;

	if (!sfc_nor_concat_params_valid(concat_info))
		return -EINVAL;

	return 1;
}

int sfc_nor_concat_setup_from_params(
		const struct spiflash_info *concat_info)
{
	return sfc_nor_concat_setup_from_params_common(concat_info, 0);
}

int sfc_nor_concat_setup_from_static_params(void)
{
	const struct sfc_flash_concat_context *ctx;
	uint32_t count;
	uint32_t i;
	uint32_t linear_base = 0;
	int ret;

	if (nor_concat_enabled)
		return 0;

	if (jz_sfc_flash_concat_init())
		return -EIO;

	ctx = jz_sfc_flash_concat_context();
	count = ctx ? ctx->chip_count : 0;
	if (!ctx || !ctx->chips || count <= 1)
		return 0;

	if (count > CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS)
		count = CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS;

	if (jz_sfc_flash_concat_select_chip(0))
		return -EIO;

	/*
	 * Static fallback for runtime NOR concat targets without burned
	 * NOR_CONCAT_VERSION params. Chip0 keeps the original NOR_VERSION
	 * params, and other chips are resolved from CONFIG_NOR_COMMON_PARAMS.
	 */
	if (sfc_nor_concat_cache_static_params())
		return -EIO;

	for (i = 0; i < count; i++) {
		int ret;

		ret = sfc_nor_concat_probe_chip_id(i,
						   jz_sfc_flash_concat_chip(i));
		if (ret)
			return ret;
	}

	for (i = 0; i < count; i++) {
		int ret;

		ret = sfc_nor_concat_init_static_chip(i, linear_base);
		if (ret)
			return ret;

		ret = sfc_flash_concat_append_checked_segment(
				&nor_concat_segments[i],
				&nor_concat_chips[i].segment,
				i, &linear_base);
		if (ret) {
			printf("sfc nor concat invalid segment chip%u\n", i);
			return -EINVAL;
		}
	}

	nor_concat_chip_count = count;
	nor_concat_total_size = linear_base;
	ret = sfc_nor_install_mtdconcat_bridge();
	if (ret)
		return ret;
	nor_concat_enabled = 1;

	if (sfc_nor_concat_activate(0))
		return -EIO;

	debug("sfc nor concat enabled chips=%u total=%x\n",
	      nor_concat_chip_count, nor_concat_total_size);

	return 0;
}
#endif

uint32_t sfc_nor_concat_capacity(void)
{
	return nor_concat_enabled ? nor_concat_total_size : 0;
}

int sfc_nor_concat_is_enabled(void)
{
	return nor_concat_enabled;
}

int sfc_nor_concat_read(unsigned int from, unsigned int len,
			unsigned char *buf)
{
	size_t retlen = 0;
	int ret;

	if (!nor_concat_mtd)
		return -ENODEV;

	ret = mtd_read(nor_concat_mtd, from, len, &retlen, buf);
	if (ret)
		return ret;

	return retlen == len ? 0 : -EIO;
}

int sfc_nor_concat_write(unsigned int to, unsigned int len,
			 unsigned char *buf)
{
	size_t retlen = 0;
	int ret;

	if (!len)
		return 0;
	if (!nor_concat_mtd)
		return -ENODEV;

	ret = mtd_write(nor_concat_mtd, to, len, &retlen, buf);
	if (ret)
		return ret;

	return retlen == len ? 0 : -EIO;
}

int sfc_nor_concat_erase(unsigned int addr, unsigned int len)
{
	struct erase_info erase;
	int erasesize;

	if (!len)
		return 0;
	if (!nor_concat_mtd)
		return -ENODEV;

	erasesize = nor_concat_mtd->erasesize;
	if (len % erasesize)
		len = len - (len % erasesize) + erasesize;

	memset(&erase, 0, sizeof(erase));
	erase.mtd = nor_concat_mtd;
	erase.addr = addr;
	erase.len = len;

	return mtd_erase(nor_concat_mtd, &erase);
}

static int sfc_nor_concat_child_read(struct jz_sfc_concat_mtd_child *child,
				     loff_t from, size_t len,
				     size_t *retlen, u_char *buf)
{
	struct sfc_nor_runtime_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	int ret;

	if (!chip || !chip->valid)
		return -EINVAL;

	ret = sfc_read((unsigned int)from, len, buf);
	if (ret < 0)
		return ret;
	if (ret < len || ret > ALIGN(len, 4))
		return -EIO;

	*retlen = len;
	return 0;
}

static int sfc_nor_concat_child_write(struct jz_sfc_concat_mtd_child *child,
				      loff_t to, size_t len,
				      size_t *retlen, const u_char *buf)
{
	struct sfc_nor_runtime_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	int ret;

	if (!chip || !chip->valid)
		return -EINVAL;

	ret = sfc_nor_concat_page_write_wait((unsigned int)to, len,
					     (unsigned char *)buf);
	if (ret < 0)
		return ret;

	*retlen = len;
	return 0;
}

static int sfc_nor_concat_child_erase(struct jz_sfc_concat_mtd_child *child,
				      struct erase_info *instr)
{
	struct sfc_nor_runtime_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	uint32_t addr;
	uint32_t end;
	uint32_t erase_size;
	int wait_ready;
	int ret;

	if (!chip || !chip->valid)
		return -EINVAL;

	erase_size = chip->info.erase_size;
	if (!erase_size || instr->addr % erase_size || instr->len % erase_size)
		return -EINVAL;
	wait_ready = chip->desc &&
		sfc_flash_concat_chip_cs_effective(chip->desc) ==
		SFC_FLASH_CONCAT_CS_GPIO;

	addr = (uint32_t)instr->addr;
	end = addr + (uint32_t)instr->len;
	instr->state = MTD_ERASING;
	while (addr < end) {
		ret = sfc_do_erase(addr);
		if (ret) {
			printf("erase error !\n");
			instr->state = MTD_ERASE_FAILED;
			return ret;
		}
		if (wait_ready) {
			ret = sfc_nor_concat_wait_ready(
					SFC_NOR_ERASE_TIMEOUT_MS);
			if (ret) {
				instr->state = MTD_ERASE_FAILED;
				return ret;
			}
		}
		addr += erase_size;
	}

	instr->state = MTD_ERASE_DONE;
	return 0;
}

static void sfc_nor_child_mtd_init(struct sfc_nor_runtime_chip *chip)
{
	struct mtd_info *mtd;

	jz_sfc_concat_mtd_child_init(&chip->child_mtd, "sfc_nor_child",
				     chip->segment.chip_index, chip,
				     &sfc_nor_child_mtd_ops);
	mtd = jz_sfc_concat_mtd_child_mtd(&chip->child_mtd);
	if (!mtd)
		return;

	mtd->type = MTD_NORFLASH;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->size = chip->segment.chip_size;
	mtd->erasesize = chip->info.erase_size;
	mtd->writesize = 1;
}

static int sfc_nor_install_mtdconcat_bridge(void)
{
	uint32_t i;

	for (i = 0; i < nor_concat_chip_count; i++)
		nor_concat_subdevs[i] =
			jz_sfc_concat_mtd_child_mtd(
				&nor_concat_chips[i].child_mtd);

	nor_concat_mtd = mtd_concat_create(nor_concat_subdevs,
			nor_concat_chip_count, "sfc_nor");
	return nor_concat_mtd ? 0 : -ENOMEM;
}
#endif
