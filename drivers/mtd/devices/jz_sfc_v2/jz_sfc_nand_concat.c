#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>
#include <nand.h>
#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/concat.h>
#include <linux/mtd/nand.h>
#include <asm/io.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>
#include "jz_sfc_common.h"
#include "jz_sfc_concat.h"
#include "./nand_device/nand_common.h"

#ifdef CONFIG_BURNER
#include <cloner/cloner.h>
#endif

#ifdef CONFIG_SFC_NAND_CONCAT

#define SFC_NAND_INIT_THOLD       50
#define SFC_NAND_INIT_TSETUP      50
#define SFC_NAND_INIT_TSHSL_R     100
#define SFC_NAND_INIT_TSHSL_W     100

struct jz_sfcnand_concat_chip {
	const struct sfc_flash_concat_chip *desc;
	struct sfc_flash flash;
	struct jz_sfcnand_flashinfo flash_info;
	struct jz_sfc_concat_mtd_child child_mtd;
	struct nand_chip nand_chip;
	struct sfc_flash_concat_segment segment;
	uint32_t packed_id;
	int valid;
};

static struct jz_sfcnand_concat_chip
	sfcnand_concat_chips[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
static struct sfc_flash_concat_segment
	sfcnand_concat_segments[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
static uint32_t sfcnand_concat_chip_count;
static uint32_t sfcnand_concat_total_size;
static int sfcnand_concat_enabled;
static struct nand_chip sfcnand_concat_nand_chip;
static struct mtd_info
	*sfcnand_concat_subdevs[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
static struct mtd_info *sfcnand_concat_mtd;

int jz_sfc_nand_concat_read_gpio_cache(struct sfc_flash *flash,
				       struct sfc_cdt_xfer *xfer,
				       unsigned short cmd_index,
				       struct jz_sfcnand_ops *ops)
{
	struct sfc_cdt_xfer to_cache_xfer;
	struct sfc_cdt_xfer cache_xfer;
	int ret;

	if (!(sfcnand_concat_enabled && jz_sfc_flash_concat_uses_gpio_cs()))
		return 1;
	if (!flash || !xfer || !ops)
		return -EINVAL;

	to_cache_xfer = *xfer;
	cache_xfer = *xfer;

	to_cache_xfer.dataen = DISABLE;
	memset(&to_cache_xfer.config, 0, sizeof(to_cache_xfer.config));

	if (sfc_sync_cdt_once(flash->sfc, &to_cache_xfer)) {
		printf("sfc_sync_cdt error ! %s %s %d\n",
		       __FILE__, __func__, __LINE__);
		return -EIO;
	}

	ret = ops->get_feature(flash, GET_READY_STATUS);
	if (ret) {
		printf("get ready state error!\n");
		return ret;
	}

	cache_xfer.cmd_index = cmd_index == NAND_QUAD_READ_TO_CACHE ?
		NAND_QUAD_READ_FROM_CACHE : NAND_STANDARD_READ_FROM_CACHE;

	if (sfc_sync_cdt_once(flash->sfc, &cache_xfer)) {
		printf("sfc_sync_cdt error ! %s %s %d\n",
		       __FILE__, __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

static struct mtd_info *sfcnand_concat_chip_mtd(
		struct jz_sfcnand_concat_chip *chip)
{
	return chip ? jz_sfc_concat_mtd_child_mtd(&chip->child_mtd) : NULL;
}

static int sfcnand_concat_child_read(struct jz_sfc_concat_mtd_child *child,
				     loff_t from, size_t len,
				     size_t *retlen, u_char *buf)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	return jz_sfcnand_read(mtd, from, len, retlen, buf);
}

static int sfcnand_concat_child_write(struct jz_sfc_concat_mtd_child *child,
				      loff_t to, size_t len,
				      size_t *retlen, const u_char *buf)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	return jz_sfcnand_write_raw(mtd, to, len, retlen, buf);
}

static int sfcnand_concat_child_erase(struct jz_sfc_concat_mtd_child *child,
				      struct erase_info *instr)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;
	int ret;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	ret = jz_sfc_nand_erase(mtd, instr);
	if (ret) {
		printf("WARNING: concat chip%u block %u erase fail !\n",
		       chip->segment.chip_index,
		       (uint32_t)instr->addr / mtd->erasesize);
		ret = jz_sfcnand_block_markbad(mtd, instr->addr);
		if (ret) {
			printf("mark bad block error, there will occur error,so exit !\n");
			instr->state = MTD_ERASE_FAILED;
			return -EIO;
		}
	}

	instr->state = MTD_ERASE_DONE;
	return 0;
}

static int sfcnand_concat_child_read_oob(
		struct jz_sfc_concat_mtd_child *child, loff_t from,
		struct mtd_oob_ops *ops)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	return jz_sfcnand_read_oob(mtd, from, ops);
}

static int sfcnand_concat_child_write_oob(
		struct jz_sfc_concat_mtd_child *child, loff_t to,
		struct mtd_oob_ops *ops)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	return jz_sfcnand_write_oob_raw(mtd, to, ops);
}

static int sfcnand_concat_child_block_isbad(
		struct jz_sfc_concat_mtd_child *child, loff_t ofs)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	return sfcnand_block_isbad(mtd, ofs);
}

static int sfcnand_concat_child_block_markbad(
		struct jz_sfc_concat_mtd_child *child, loff_t ofs)
{
	struct jz_sfcnand_concat_chip *chip =
		jz_sfc_concat_mtd_child_priv(child);
	struct mtd_info *mtd;

	mtd = jz_sfc_concat_mtd_child_mtd(child);
	if (!chip || !chip->valid || !mtd)
		return -EINVAL;

	return jz_sfcnand_block_markbad(mtd, ofs);
}

static const struct jz_sfc_concat_mtd_child_ops sfcnand_child_mtd_ops = {
	.read = sfcnand_concat_child_read,
	.write = sfcnand_concat_child_write,
	.erase = sfcnand_concat_child_erase,
	.read_oob = sfcnand_concat_child_read_oob,
	.write_oob = sfcnand_concat_child_write_oob,
	.block_isbad = sfcnand_concat_child_block_isbad,
	.block_markbad = sfcnand_concat_child_block_markbad,
};

static uint32_t sfcnand_concat_pack_id(struct jz_sfcnand_flashinfo *info)
{
	if (info->id_device > 0xff)
		return ((uint32_t)info->id_manufactory << 16) | info->id_device;

	return ((uint32_t)info->id_manufactory << 8) | info->id_device;
}

static struct jz_sfcnand_concat_chip *
sfcnand_concat_chip_by_index(uint32_t index)
{
	if (index >= sfcnand_concat_chip_count)
		return NULL;

	if (!sfcnand_concat_chips[index].valid)
		return NULL;

	return &sfcnand_concat_chips[index];
}

static int sfcnand_concat_prepare_profile(void *priv)
{
	struct jz_sfcnand_concat_chip *chip = priv;
	struct sfc_flash *flash;

	if (!chip || !chip->valid)
		return -EINVAL;

	flash = &chip->flash;
	jz_sfc_nand_set_current_flash(flash);
	jz_sfc_nand_create_cdt_table(flash, UPDATE_CDT);
	set_flash_timing(flash->sfc, chip->flash_info.param.tHOLD,
			 chip->flash_info.param.tSETUP,
			 chip->flash_info.param.tSHSL_R,
			 chip->flash_info.param.tSHSL_W);

	return 0;
}

static int sfcnand_concat_activate(uint32_t index)
{
	struct jz_sfcnand_concat_chip *chip;
	int ret;

	chip = sfcnand_concat_chip_by_index(index);
	if (!chip)
		return -EINVAL;

	ret = jz_sfc_flash_concat_activate(index);
	if (ret)
		return ret;

	return 0;
}

static int sfcnand_concat_page_aligned(uint32_t offset)
{
	struct sfc_flash_concat_span span;
	struct jz_sfcnand_concat_chip *chip;
	struct mtd_info *mtd;

	if (sfc_flash_concat_find_span(sfcnand_concat_segments,
				       sfcnand_concat_chip_count,
				       offset, 1, &span))
		return 0;

	chip = sfcnand_concat_chip_by_index(span.split.chip_index);
	mtd = sfcnand_concat_chip_mtd(chip);
	if (!mtd || !mtd->writesize)
		return 0;

	return sfc_flash_concat_is_aligned(span.split.chip_offset,
					   mtd->writesize);
}

static int sfcnand_concat_block_range(uint32_t offset,
				      struct sfc_flash_concat_split *split,
				      struct jz_sfcnand_concat_chip **chip,
				      uint32_t *block_start,
				      uint32_t *block_len)
{
	struct jz_sfcnand_concat_chip *cur;
	struct sfc_flash_concat_span span;
	struct mtd_info *mtd;
	uint32_t cur_block_len;
	uint32_t block_offset;
	int ret;

	ret = sfc_flash_concat_find_span(sfcnand_concat_segments,
					 sfcnand_concat_chip_count,
					 offset, 1, &span);
	if (ret)
		return ret;

	cur = sfcnand_concat_chip_by_index(span.split.chip_index);
	mtd = sfcnand_concat_chip_mtd(cur);
	if (!mtd || !mtd->erasesize)
		return -EINVAL;

	if (!span.chip_remain)
		return -EINVAL;
	block_offset = span.split.chip_offset % mtd->erasesize;
	cur_block_len = sfc_flash_concat_min_u32(
			mtd->erasesize - block_offset, span.chip_remain);
	if (!cur_block_len)
		return -EINVAL;

	if (split)
		*split = span.split;
	if (chip)
		*chip = cur;
	if (block_start)
		*block_start = span.chip_linear_base +
			span.split.chip_offset - block_offset;
	if (block_len)
		*block_len = cur_block_len;

	return 0;
}

static int sfcnand_concat_erase_range_aligned(uint32_t offset, uint32_t len)
{
	uint32_t end;
	uint32_t done = 0;

	if (!len)
		return 0;
	end = offset + len;
	if (end < offset || end > sfcnand_concat_total_size)
		return 0;

	while (done < len) {
		struct sfc_flash_concat_split split;
		struct jz_sfcnand_concat_chip *chip;
		struct mtd_info *mtd;
		uint32_t block_len;
		int ret;

		ret = sfcnand_concat_block_range(offset + done, &split,
						 &chip, NULL, &block_len);
		if (ret || !block_len)
			return 0;

		mtd = sfcnand_concat_chip_mtd(chip);
		if (!mtd || !mtd->erasesize)
			return 0;

		if (!sfc_flash_concat_is_aligned(split.chip_offset,
						 mtd->erasesize))
			return 0;
		if (!sfc_flash_concat_is_aligned(block_len,
						 mtd->erasesize))
			return 0;
		if (len - done < block_len)
			return 0;

		done += block_len;
	}

	return 1;
}

static int sfcnand_concat_check_skip_len(nand_info_t *nand, loff_t offset,
					 size_t length, size_t *used)
{
	size_t len_excl_bad = 0;
	int ret = 0;

	*used = 0;
	while (len_excl_bad < length) {
		struct sfc_flash_concat_split split;
		uint32_t block_start;
		uint32_t block_len;
		int bad;

		if (offset < 0 || offset > 0xffffffffULL)
			return -1;

		if (sfcnand_concat_block_range((uint32_t)offset, &split,
					       NULL, &block_start,
					       &block_len))
			return -1;
		if (!block_len)
			return -1;

		bad = mtd_block_isbad(nand, block_start);
		if (bad < 0)
			return bad;
		if (!bad)
			len_excl_bad += block_len;
		else
			ret = 1;

		offset += block_len;
		*used += block_len;
	}

	if (len_excl_bad > length)
		*used -= len_excl_bad - length;

	return ret;
}

static int sfcnand_concat_partition_range_valid(uint32_t offset,
						uint32_t size)
{
	return size && offset <= ~(uint32_t)0 - size;
}

static struct jz_sfcnand_partition *
sfcnand_concat_find_partition(uint32_t offset, uint32_t size)
{
	struct jz_sfcnand_partition *parts = jz_sfc_nand_burner_param.partition;
	uint32_t end;
	int32_t count = jz_sfc_nand_burner_param.partition_num;
	int32_t i;

	if (!sfcnand_concat_partition_range_valid(offset, size) ||
	    !parts || count <= 0 || count > PARTITION_NUM)
		return NULL;

	end = offset + size;
	for (i = 0; i < count; i++) {
		uint32_t part_end;

		if (!sfcnand_concat_partition_range_valid(parts[i].offset,
							  parts[i].size))
			continue;

		part_end = parts[i].offset + parts[i].size;
		if (offset >= parts[i].offset && end <= part_end)
			return &parts[i];
	}

	return NULL;
}

static int sfcnand_concat_range_is_writable_partition(uint32_t offset,
						      uint32_t size)
{
	struct jz_sfcnand_partition *part;

	part = sfcnand_concat_find_partition(offset, size);
	return part && !(part->mask_flags & PART_RO);
}

#ifndef CONFIG_BURNER
static int sfcnand_concat_request_readonly(uint32_t offset, uint32_t size,
					   uint32_t align, const char *op)
{
	struct jz_sfcnand_partition *part;

	if (!size)
		return 0;
	if (!align || (offset & (align - 1)) || (size & (align - 1)))
		return 1;
	if (sfcnand_concat_range_is_writable_partition(offset, size))
		return 0;

	part = sfcnand_concat_find_partition(offset, size);
	if (part)
		printf("\n%s partition is read-only and does not allow %s operation.\n",
		       part->name, op);
	else
		printf("\nsfc nand %s range offset=%x size=%x is not in one writable partition.\n",
		       op, offset, size);

	return 1;
}

static int sfcnand_concat_erase_request_readonly(uint32_t offset,
						 uint32_t size,
						 uint32_t erasesize)
{
	return sfcnand_concat_request_readonly(offset, size, erasesize,
					      "erase");
}

static int sfcnand_concat_write_request_readonly(uint32_t offset,
						 uint32_t size,
						 uint32_t writesize)
{
	return sfcnand_concat_request_readonly(offset, size, writesize,
					      "write");
}
#endif

int nand_concat_is_managed(nand_info_t *nand)
{
	return sfcnand_concat_enabled && nand == &nand_info[0];
}

int nand_concat_read_skip_bad(nand_info_t *nand, loff_t offset,
			      size_t *length, size_t *actual, loff_t lim,
			      u_char *buffer)
{
	size_t left_to_read;
	size_t used_for_read = 0;
	u_char *p_buffer = buffer;
	int need_skip;

	if (!length)
		return -EINVAL;
	if (actual)
		*actual = 0;

	if (offset < 0 || offset > 0xffffffffULL ||
	    *length > 0xffffffffUL) {
		*length = 0;
		return -EINVAL;
	}
	left_to_read = *length;
	need_skip = sfcnand_concat_check_skip_len(nand, offset, *length,
						  &used_for_read);
	if (actual)
		*actual = used_for_read;
	if (need_skip < 0) {
		printf("Attempt to read outside the flash area\n");
		*length = 0;
		return -EINVAL;
	}
	if (used_for_read > lim) {
		puts("Size of read exceeds partition or device limit\n");
		*length = 0;
		return -EFBIG;
	}
	if (!need_skip) {
		int ret;

		ret = mtd_read(nand, offset, *length, length, buffer);
		return ret && ret != -EUCLEAN ? ret : 0;
	}

	while (left_to_read > 0) {
		struct sfc_flash_concat_split split;
		uint32_t block_start;
		uint32_t block_len;
		size_t read_length;
		int ret;

		WATCHDOG_RESET();

		ret = sfcnand_concat_block_range((uint32_t)offset, &split,
						 NULL, &block_start,
						 &block_len);
		if (ret || !block_len) {
			*length -= left_to_read;
			return ret ? ret : -EINVAL;
		}

		ret = mtd_block_isbad(nand, block_start);
		if (ret < 0) {
			*length -= left_to_read;
			return ret;
		}
		if (ret > 0) {
			printf("Skipping bad block 0x%08x\n", block_start);
			offset += block_len;
			continue;
		}

		read_length = min_t(size_t, left_to_read, block_len);
		ret = mtd_read(nand, offset, read_length, &read_length,
			       p_buffer);
		if (ret < 0 && ret != -EUCLEAN) {
			printf("NAND read from offset %llx failed %d\n",
			       offset, ret);
			*length -= left_to_read;
			return ret;
		}

		left_to_read -= read_length;
		offset += read_length;
		p_buffer += read_length;
	}

	return 0;
}

int nand_concat_write_skip_bad(nand_info_t *nand, loff_t offset,
			       size_t *length, size_t *actual, loff_t lim,
			       u_char *buffer, int flags)
{
	size_t left_to_write;
	size_t used_for_write = 0;
	u_char *p_buffer = buffer;
	int need_skip;
	int ret;

	if (!length)
		return -EINVAL;
	if (actual)
		*actual = 0;

	if (flags) {
		printf("sfc nand concat does not support OOB/trim write mode\n");
		*length = 0;
		return -EOPNOTSUPP;
	}
	if (offset < 0 || offset > 0xffffffffULL ||
	    *length > 0xffffffffUL) {
		*length = 0;
		return -EINVAL;
	}
	if (*length && !sfcnand_concat_page_aligned((uint32_t)offset)) {
		printf("Attempt to write non page-aligned data\n");
		*length = 0;
		return -EINVAL;
	}

#ifndef CONFIG_BURNER
	ret = sfcnand_concat_write_request_readonly((uint32_t)offset,
						    (uint32_t)*length,
						    nand->writesize);
	if (ret) {
		*length = 0;
		return ret < 0 ? ret : -EROFS;
	}
#endif

	left_to_write = *length;
	need_skip = sfcnand_concat_check_skip_len(nand, offset, *length,
						  &used_for_write);
	if (actual)
		*actual = used_for_write;
	if (need_skip < 0) {
		printf("Attempt to write outside the flash area\n");
		*length = 0;
		return -EINVAL;
	}
	if (used_for_write > lim) {
		puts("Size of write exceeds partition or device limit\n");
		*length = 0;
		return -EFBIG;
	}
	if (!need_skip)
		return mtd_write(nand, offset, *length, length, buffer);

	while (left_to_write > 0) {
		struct sfc_flash_concat_split split;
		uint32_t block_start;
		uint32_t block_len;
		size_t write_size;

		WATCHDOG_RESET();

		ret = sfcnand_concat_block_range((uint32_t)offset, &split,
						 NULL, &block_start,
						 &block_len);
		if (ret || !block_len) {
			*length -= left_to_write;
			return ret ? ret : -EINVAL;
		}

		ret = mtd_block_isbad(nand, block_start);
		if (ret < 0) {
			*length -= left_to_write;
			return ret;
		}
		if (ret > 0) {
			printf("Skip bad block 0x%08x\n", block_start);
			offset += block_len;
			continue;
		}

		write_size = min_t(size_t, left_to_write, block_len);
		ret = mtd_write(nand, offset, write_size, &write_size,
				p_buffer);
		if (ret) {
			printf("NAND write to offset %llx failed %d\n",
			       offset, ret);
			*length -= left_to_write;
			return ret;
		}

		left_to_write -= write_size;
		offset += write_size;
		p_buffer += write_size;
	}

	return 0;
}

int nand_concat_erase_opts(nand_info_t *meminfo,
			   const nand_erase_options_t *opts)
{
	struct erase_info erase;
	loff_t addr;
	loff_t erased_length = 0;
	int percent_complete = -1;
	int result;

	if (!opts)
		return -EINVAL;
	if (!opts->length)
		return 0;
	if (opts->jffs2) {
		printf("sfc nand concat does not support cleanmarker erase\n");
		return -EOPNOTSUPP;
	}
	if (opts->offset < 0 || opts->length < 0 ||
	    opts->offset > 0xffffffffULL ||
	    opts->length > 0xffffffffULL ||
	    opts->offset + opts->length > 0xffffffffULL) {
		return -EINVAL;
	}
	if (!sfcnand_concat_erase_range_aligned((uint32_t)opts->offset,
						(uint32_t)opts->length)) {
		printf("Attempt to erase non block-aligned data\n");
		return -EINVAL;
	}

#ifndef CONFIG_BURNER
	result = sfcnand_concat_erase_request_readonly(
			(uint32_t)opts->offset, (uint32_t)opts->length,
			meminfo->erasesize);
	if (result)
		return result < 0 ? result : -EROFS;
#endif

	addr = opts->offset;
	while (erased_length < opts->length) {
		struct sfc_flash_concat_split split;
		uint32_t block_start;
		uint32_t block_len;

		if (opts->lim && addr >= opts->offset + opts->lim) {
			puts("Size of erase exceeds limit\n");
			return -EFBIG;
		}

		result = sfcnand_concat_block_range((uint32_t)addr, &split,
						    NULL, &block_start,
						    &block_len);
		if (result || !block_len)
			return result ? result : -EINVAL;

		if (!opts->scrub) {
			result = mtd_block_isbad(meminfo, block_start);
			if (result > 0) {
				if (!opts->quiet)
					printf("\rSkipping bad block at  0x%08x\n",
					       block_start);
				if (!opts->spread)
					erased_length += block_len;
				addr += block_len;
				continue;
			} else if (result < 0) {
				printf("\n%s: MTD get bad block failed: %d\n",
				       meminfo->name, result);
				return result;
			}
		}

		memset(&erase, 0, sizeof(erase));
		erase.mtd = meminfo;
		erase.addr = addr;
		erase.len = block_len;
		erase.scrub = opts->scrub;

		result = mtd_erase(meminfo, &erase);
		if (result) {
			printf("\n%s: MTD Erase failure: %d\n",
			       meminfo->name, result);
			return result;
		}

		addr += block_len;
		erased_length += block_len;

		if (!opts->quiet) {
			unsigned long long n = erased_length * 100ULL;
			int percent;

			do_div(n, opts->length);
			percent = (int)n;
			if (percent != percent_complete) {
				percent_complete = percent;
				printf("\rErasing at 0x%llx -- %3d%% complete.",
				       erase.addr, percent);
			}
		}
	}
	if (!opts->quiet)
		printf("\n");

	return 0;
}

static int sfcnand_mtdconcat_read(struct mtd_info *mtd, loff_t from,
				  size_t len, size_t *retlen, u_char *buf)
{
	if (!sfcnand_concat_mtd)
		return -ENODEV;

	return mtd_read(sfcnand_concat_mtd, from, len, retlen, buf);
}

static int sfcnand_mtdconcat_write(struct mtd_info *mtd, loff_t to,
				   size_t len, size_t *retlen,
				   const u_char *buf)
{
	if (!sfcnand_concat_mtd)
		return -ENODEV;

#ifndef CONFIG_BURNER
	if (jz_sfc_nand_is_readonly_partition((uint32_t)to, (uint32_t)len))
		return -EROFS;
#endif

	return mtd_write(sfcnand_concat_mtd, to, len, retlen, buf);
}

static int sfcnand_mtdconcat_erase(struct mtd_info *mtd,
				   struct erase_info *instr)
{
	struct erase_info concat_instr;
	int ret;

	if (!sfcnand_concat_mtd)
		return -ENODEV;

#ifdef CONFIG_BURNER
	if ((spi_args->spi_erase == CHIP_ERASE ||
	     spi_args->spi_erase == FACTORY_ERASE) &&
	    instr->addr < mtd->size) {
		ret = jz_sfc_nand_is_readonly_partition((uint32_t)instr->addr,
							(uint32_t)instr->len);
		if (ret)
			return 0;
	}
#else
	if (!instr->scrub) {
		ret = jz_sfc_nand_is_readonly_partition((uint32_t)instr->addr,
							(uint32_t)instr->len);
		if (ret)
			return -EROFS;
	}
#endif

	concat_instr = *instr;
	concat_instr.mtd = sfcnand_concat_mtd;
	concat_instr.callback = NULL;

	ret = mtd_erase(sfcnand_concat_mtd, &concat_instr);
	instr->state = concat_instr.state;
	instr->fail_addr = concat_instr.fail_addr;

	return ret;
}

static int sfcnand_mtdconcat_read_oob(struct mtd_info *mtd, loff_t from,
				      struct mtd_oob_ops *ops)
{
	if (!sfcnand_concat_mtd)
		return -ENODEV;

	return mtd_read_oob(sfcnand_concat_mtd, from, ops);
}

static int sfcnand_mtdconcat_write_oob(struct mtd_info *mtd, loff_t to,
				       struct mtd_oob_ops *ops)
{
#ifndef CONFIG_BURNER
	uint32_t len = ops->len ? ops->len : 1;
#endif

	if (!sfcnand_concat_mtd)
		return -ENODEV;

#ifndef CONFIG_BURNER
	if (jz_sfc_nand_is_readonly_partition((uint32_t)to, len))
		return -EROFS;
#endif

	return mtd_write_oob(sfcnand_concat_mtd, to, ops);
}

static int sfcnand_mtdconcat_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	if (!sfcnand_concat_mtd)
		return -ENODEV;

	return mtd_block_isbad(sfcnand_concat_mtd, ofs);
}

static int sfcnand_mtdconcat_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	if (!sfcnand_concat_mtd)
		return -ENODEV;

	return mtd_block_markbad(sfcnand_concat_mtd, ofs);
}

static void sfcnand_mtdconcat_sync(struct mtd_info *mtd)
{
	if (sfcnand_concat_mtd)
		mtd_sync(sfcnand_concat_mtd);
}

static int sfcnand_install_mtdconcat_bridge(struct mtd_info *mtd)
{
	uint32_t i;

	for (i = 0; i < sfcnand_concat_chip_count; i++)
		sfcnand_concat_subdevs[i] =
			sfcnand_concat_chip_mtd(&sfcnand_concat_chips[i]);

	sfcnand_concat_mtd = mtd_concat_create(sfcnand_concat_subdevs,
			sfcnand_concat_chip_count, "sfc_nand");
	if (!sfcnand_concat_mtd)
		return -ENOMEM;

	mtd->type = sfcnand_concat_mtd->type;
	mtd->flags = sfcnand_concat_mtd->flags;
	mtd->size = sfcnand_concat_mtd->size;
	mtd->erasesize = sfcnand_concat_mtd->erasesize;
	mtd->writesize = sfcnand_concat_mtd->writesize;
	mtd->oobsize = sfcnand_concat_mtd->oobsize;
	mtd->oobavail = sfcnand_concat_mtd->oobavail;
	mtd->bitflip_threshold = sfcnand_concat_mtd->bitflip_threshold;
	mtd->ecclayout = sfcnand_concat_mtd->ecclayout;
	mtd->ecc_strength = sfcnand_concat_mtd->ecc_strength;
	mtd->numeraseregions = sfcnand_concat_mtd->numeraseregions;
	mtd->eraseregions = sfcnand_concat_mtd->eraseregions;
	mtd->ecc_stats = sfcnand_concat_mtd->ecc_stats;
	mtd->subpage_sft = sfcnand_concat_mtd->subpage_sft;
	mtd->_erase = sfcnand_mtdconcat_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = sfcnand_mtdconcat_read;
	mtd->_write = sfcnand_mtdconcat_write;
	mtd->_read_oob = sfcnand_mtdconcat_read_oob;
	mtd->_write_oob = sfcnand_mtdconcat_write_oob;
	mtd->_sync = sfcnand_mtdconcat_sync;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_block_isbad = sfcnand_mtdconcat_block_isbad;
	mtd->_block_markbad = sfcnand_mtdconcat_block_markbad;

	return 0;
}

static int32_t sfcnand_concat_probe_chip(uint32_t index,
		const struct sfc_flash_concat_chip *desc, struct sfc *sfc,
		uint32_t linear_base)
{
	struct jz_sfcnand_concat_chip *chip = &sfcnand_concat_chips[index];
	struct mtd_info *child_mtd;
	int32_t ret;

	memset(chip, 0, sizeof(*chip));
	chip->desc = desc;
	jz_sfc_concat_mtd_child_init(&chip->child_mtd, "sfc_nand_child",
				     index, chip, &sfcnand_child_mtd_ops);
	child_mtd = sfcnand_concat_chip_mtd(chip);
	if (!child_mtd)
		return -ENOMEM;

	chip->flash.sfc = sfc;
	chip->flash.flash_info = &chip->flash_info;
	chip->flash.mtd = child_mtd;

	if (jz_sfc_flash_concat_select_chip(index))
		return -EIO;
	jz_sfc_nand_set_current_flash(&chip->flash);

	jz_sfc_nand_create_cdt_table(&chip->flash, DEFAULT_CDT);
	ret = sfc_nand_reset();
	if (ret) {
		printf("ERR: sfc nand concat chip%u reset error!\n", index);
		return ret;
	}

	ret = jz_sfc_nand_try_id(&chip->flash, &chip->flash_info);
	if (ret) {
		printf("ERR: sfc nand concat chip%u try id error!\n", index);
		return ret;
	}

	chip->packed_id = sfcnand_concat_pack_id(&chip->flash_info);

	if (desc->flags & SFC_FLASH_CONCAT_F_DISABLE_QUAD)
		chip->flash_info.param.need_quad = 0;

	if (!chip->flash_info.param.flashsize ||
	    !chip->flash_info.param.pagesize ||
	    !chip->flash_info.param.blocksize) {
		printf("sfc nand concat chip%u invalid geometry id=%x\n",
		       index, chip->packed_id);
		return -EINVAL;
	}

	jz_sfc_nand_create_cdt_table(&chip->flash, UPDATE_CDT);
	sfc_clk_set(chip->flash.sfc, CONFIG_SFC_NAND_RATE);
	set_flash_timing(chip->flash.sfc, chip->flash_info.param.tHOLD,
			 chip->flash_info.param.tSETUP,
			 chip->flash_info.param.tSHSL_R,
			 chip->flash_info.param.tSHSL_W);

	ret = sfc_nand_dev_init(&chip->flash);
	if (ret) {
		printf("ERR: sfc nand concat chip%u device init failed!\n",
		       index);
		return ret;
	}

	chip->segment.chip_index = index;
	chip->segment.linear_base = linear_base;
	chip->segment.chip_size = chip->flash_info.param.flashsize;
	child_mtd->size = chip->flash_info.param.flashsize;
	jz_sfc_nand_setup_mtd_geometry(child_mtd, &chip->nand_chip,
				       &chip->flash_info);
	chip->valid = 1;

	ret = jz_sfc_flash_concat_set_profile(index,
					      sfcnand_concat_prepare_profile,
					      chip);
	if (ret)
		return ret;

	debug("sfc nand concat chip%u id=%x size=%x base=%x page=%x block=%x quad=%s\n",
	      index, chip->packed_id, chip->segment.chip_size, linear_base,
	      child_mtd->writesize, child_mtd->erasesize,
	      chip->flash_info.param.need_quad ? "on" : "off");

	return 0;
}

#ifdef CONFIG_BURNER
static int sfcnand_concat_load_partition_entries(
		const struct jz_sfcnand_partition *src,
		int partition_count, uint64_t total_size)
{
	struct jz_sfcnand_partition *parts;
	uint32_t i;

	if (!src || partition_count <= 0 || partition_count > PARTITION_NUM)
		return -EINVAL;

	parts = calloc(partition_count, sizeof(*parts));
	if (!parts)
		return -ENOMEM;

	for (i = 0; i < (uint32_t)partition_count; i++) {
		uint32_t end;

		if (sfc_flash_concat_range_fits_u32_64(
						src[i].offset, src[i].size,
						total_size, &end)) {
			free(parts);
			return -EINVAL;
		}
		if (!sfcnand_concat_erase_range_aligned(
					src[i].offset,
					src[i].size)) {
			printf("sfc nand concat partition %s offset=%llx size=%llx is not per-chip block aligned\n",
			       src[i].name,
			       (unsigned long long)src[i].offset,
			       (unsigned long long)src[i].size);
			free(parts);
			return -EINVAL;
		}

		memcpy(parts[i].name, src[i].name, sizeof(parts[i].name));
		parts[i].name[sizeof(parts[i].name) - 1] = '\0';
		parts[i].offset = src[i].offset;
		parts[i].size = src[i].size;
		parts[i].mask_flags = src[i].mask_flags;
		parts[i].manager_mode = src[i].manager_mode;
	}

	jz_sfc_nand_burner_param.magic_num = SPINAND_MAGIC_NUM;
	jz_sfc_nand_burner_param.partition_num = partition_count;
	jz_sfc_nand_burner_param.partition = parts;

	for (i = 0; i < sfcnand_concat_chip_count; i++) {
		if (!sfcnand_concat_chips[i].valid)
			continue;
		sfcnand_concat_chips[i].flash_info.partition.num_partition =
			partition_count;
		sfcnand_concat_chips[i].flash_info.partition.partition = parts;
	}

	return 0;
}

int32_t sfcnand_burner_probe_runtime_concat(
		const struct jz_sfcnand_burner_param *param)
{
	struct mtd_info *mtd = &nand_info[0];
#ifdef CONFIG_SFC_FLASH_CONCAT
	const struct sfc_flash_concat_context *ctx =
		jz_sfc_flash_concat_context();
#endif
	int32_t ret;

#ifdef CONFIG_SFC_FLASH_CONCAT
	if (!ctx || !ctx->chips || ctx->chip_count <= 1)
		return 1;
#else
	return 1;
#endif

	ret = jz_sfc_nand_concat_init();
	if (ret) {
		printf("ERR: jz_sfc_nand_concat_init error ret=%d!\n", ret);
		return ret;
	}

	ret = sfcnand_concat_load_partition_entries(
			(const struct jz_sfcnand_partition *)&param->partition,
			param->partition_num, sfcnand_concat_total_size);
	if (ret) {
		printf("ERR: sfc nand runtime concat partition invalid ret=%d!\n", ret);
		return ret;
	}

	ret = sfcnand_burner_finish_probe(mtd);
	if (ret)
		return ret;

	mtd_sfcnand_partition_analysis(mtd->erasesize,
				       jz_sfc_nand_burner_param.partition_num,
				       jz_sfc_nand_burner_param.partition);
	debug("sfc nand burner concat enabled chips=%u total=%x parts=%d\n",
	      sfcnand_concat_chip_count, sfcnand_concat_total_size,
	      jz_sfc_nand_burner_param.partition_num);

	return 0;
}
#endif

int32_t jz_sfc_nand_concat_init(void)
{
	const struct sfc_flash_concat_context *ctx;
	struct mtd_info *mtd = &nand_info[0];
	uint32_t sfc_rate = 100000000;
	uint32_t count;
	uint32_t i;
	uint32_t linear_base = 0;
	int32_t ret;
	struct sfc *sfc;
	struct sfc_flash *flash;

	if (sfcnand_concat_enabled)
		return 0;

	ret = jz_sfc_flash_concat_init();
	if (ret)
		return -EINVAL;

	ctx = jz_sfc_flash_concat_context();
	count = ctx ? ctx->chip_count : 0;
	if (!ctx || !ctx->chips || count <= 1)
		return 1;

	if (count > CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS)
		count = CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS;

#ifdef CONFIG_SFC_NAND_INIT_RATE
	sfc_rate = CONFIG_SFC_NAND_INIT_RATE;
#endif
	sfc = sfc_res_init(sfc_rate);
	if (!sfc)
		return -ENOMEM;

	ret = spinand_moudle_init();
	if (ret)
		return -EINVAL;

	set_flash_timing(sfc, SFC_NAND_INIT_THOLD, SFC_NAND_INIT_TSETUP,
			 SFC_NAND_INIT_TSHSL_R, SFC_NAND_INIT_TSHSL_W);

	for (i = 0; i < count; i++) {
		const struct sfc_flash_concat_chip *desc =
			jz_sfc_flash_concat_chip(i);

		ret = sfcnand_concat_probe_chip(i, desc, sfc, linear_base);
		if (ret)
			return ret;

		ret = sfc_flash_concat_append_checked_segment(
				&sfcnand_concat_segments[i],
				&sfcnand_concat_chips[i].segment,
				i, &linear_base);
		if (ret) {
			printf("sfc nand concat invalid segment chip%u\n", i);
			return -EINVAL;
		}
	}

	sfcnand_concat_chip_count = count;
	sfcnand_concat_total_size = linear_base;

	ret = sfcnand_concat_activate(0);
	if (ret)
		return ret;

	flash = jz_sfc_nand_current_flash();
	if (!flash || !flash->flash_info)
		return -EINVAL;

	flash->mtd = mtd;
	memset(mtd, 0, sizeof(*mtd));
	mtd->size = sfcnand_concat_total_size;
	jz_sfc_nand_setup_mtd_geometry(mtd, &sfcnand_concat_nand_chip,
				       flash->flash_info);

#ifdef CONFIG_BURNER
	((struct jz_sfcnand_flashinfo *)flash->flash_info)->
		partition.num_partition = 0;
	((struct jz_sfcnand_flashinfo *)flash->flash_info)->
		partition.partition = NULL;
#else
	jz_sfc_nand_get_partition_from_spinand(flash);
	((struct jz_sfcnand_flashinfo *)flash->flash_info)->
		partition.num_partition =
		jz_sfc_nand_burner_param.partition_num;
	((struct jz_sfcnand_flashinfo *)flash->flash_info)->
		partition.partition =
		jz_sfc_nand_burner_param.partition;
#endif

	ret = sfcnand_install_mtdconcat_bridge(mtd);
	if (ret)
		return ret;
	nand_register(0);
	sfcnand_concat_enabled = 1;

	debug("sfc nand concat enabled chips=%u total=%x\n",
	      sfcnand_concat_chip_count, sfcnand_concat_total_size);

	return 0;
}

#endif
