#ifndef __JZ_SFC_CONCAT_H__
#define __JZ_SFC_CONCAT_H__

#include <sfc_concat.h>

#if !defined(CONFIG_SPL_BUILD) && !defined(USE_HOSTCC)
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <asm/arch/sfc.h>
#include <asm/arch/spinand.h>
#include <asm/arch/spinor.h>
#endif

#ifndef CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS
#define CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS 3
#endif

#define SFC_FLASH_CONCAT_CDT_LINK_BIT	(1U << 31)
#define JZ_SFC_INVALID_NATIVE_CS_GPIO	SFC_FLASH_CONCAT_GPIO_INVALID
#define JZ_SFC_INVALID_NATIVE_CS_FUNC	0xffffffffU
#define JZ_SFC_CONCAT_NO_ACTIVE_CHIP	0xffffffffU

struct sfc_flash_concat_segment {
	uint32_t chip_index;
	uint32_t linear_base;
	uint32_t chip_size;
};

struct sfc_flash_concat_split {
	uint32_t chip_index;
	uint32_t chip_offset;
	uint32_t chunk_len;
};

struct sfc_flash_concat_span {
	struct sfc_flash_concat_split split;
	uint32_t linear_offset;
	uint32_t chip_linear_base;
	uint32_t chip_size;
	uint32_t chip_remain;
};

struct sfc_flash_concat_context {
	const struct sfc_flash_concat_chip *chips;
	uint32_t chip_count;
	uint32_t active_chip;
};

static inline uint32_t sfc_flash_concat_min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}

static inline int sfc_flash_concat_append_segment(
		struct sfc_flash_concat_segment *segment,
		uint32_t chip_index, uint32_t chip_size,
		uint32_t *linear_base)
{
	if (!segment || !linear_base || !chip_size)
		return -1;
	if (chip_size > (uint32_t)~0 - *linear_base)
		return -1;

	segment->chip_index = chip_index;
	segment->linear_base = *linear_base;
	segment->chip_size = chip_size;
	*linear_base += chip_size;

	return 0;
}

static inline int sfc_flash_concat_append_checked_segment(
		struct sfc_flash_concat_segment *dst,
		const struct sfc_flash_concat_segment *src,
		uint32_t chip_index, uint32_t *linear_base)
{
	if (!dst || !src || !linear_base)
		return -1;
	if (src->chip_index != chip_index || src->linear_base != *linear_base)
		return -1;

	return sfc_flash_concat_append_segment(dst, chip_index,
					       src->chip_size, linear_base);
}

static inline int sfc_flash_concat_find_span(
		const struct sfc_flash_concat_segment *segments,
		uint32_t segment_count, uint32_t offset, uint32_t len,
		struct sfc_flash_concat_span *span)
{
	uint32_t i;

	if (!segments || !span || !len)
		return -1;

	for (i = 0; i < segment_count; i++) {
		const struct sfc_flash_concat_segment *seg = &segments[i];
		uint32_t end = seg->linear_base + seg->chip_size;
		uint32_t chip_offset;
		uint32_t chip_remain;
		uint32_t chunk;

		if (!seg->chip_size || end < seg->linear_base)
			continue;

		if (offset < seg->linear_base || offset >= end)
			continue;

		chip_offset = offset - seg->linear_base;
		chip_remain = seg->chip_size - chip_offset;
		chunk = sfc_flash_concat_min_u32(chip_remain, len);

		span->split.chip_index = seg->chip_index;
		span->split.chip_offset = chip_offset;
		span->split.chunk_len = chunk;
		span->linear_offset = offset;
		span->chip_linear_base = seg->linear_base;
		span->chip_size = seg->chip_size;
		span->chip_remain = chip_remain;
		return 0;
	}

	return -1;
}

static inline int sfc_flash_concat_range_fits_u32_64(uint64_t offset,
		uint64_t len, uint64_t total, uint32_t *end32)
{
	uint64_t end;

	if (!end32 || !len)
		return -1;
	end = offset + len;
	if (end < offset)
		return -1;
	if (end > total || offset > 0xffffffffULL || len > 0xffffffffULL ||
	    end > 0xffffffffULL)
		return -1;

	*end32 = (uint32_t)end;
	return 0;
}

static inline int sfc_flash_concat_is_aligned(uint32_t value, uint32_t align)
{
	if (!align)
		return 0;

	return (value % align) == 0;
}

typedef int (*jz_sfc_flash_concat_prepare_profile_t)(void *priv);

#if defined(CONFIG_SFC_FLASH_CONCAT) && !defined(CONFIG_SPL_BUILD) && \
	!defined(USE_HOSTCC)
int sfc_sync_cdt_once(struct sfc *sfc, struct sfc_cdt_xfer *xfer);
#endif

#if !defined(CONFIG_SPL_BUILD) && !defined(USE_HOSTCC)
struct jz_sfc_concat_mtd_child;

struct jz_sfc_concat_mtd_child_ops {
	int (*read)(struct jz_sfc_concat_mtd_child *child, loff_t from,
		    size_t len, size_t *retlen, u_char *buf);
	int (*write)(struct jz_sfc_concat_mtd_child *child, loff_t to,
		     size_t len, size_t *retlen, const u_char *buf);
	int (*erase)(struct jz_sfc_concat_mtd_child *child,
		     struct erase_info *instr);
	int (*read_oob)(struct jz_sfc_concat_mtd_child *child, loff_t from,
			struct mtd_oob_ops *ops);
	int (*write_oob)(struct jz_sfc_concat_mtd_child *child, loff_t to,
			 struct mtd_oob_ops *ops);
	int (*block_isbad)(struct jz_sfc_concat_mtd_child *child,
			   loff_t ofs);
	int (*block_markbad)(struct jz_sfc_concat_mtd_child *child,
			     loff_t ofs);
};

struct jz_sfc_concat_mtd_child {
	struct mtd_info mtd;
	uint32_t chip_index;
	void *priv;
	const struct jz_sfc_concat_mtd_child_ops *ops;
};

void jz_sfc_concat_mtd_child_init(struct jz_sfc_concat_mtd_child *child,
				  const char *name, uint32_t chip_index,
				  void *priv,
				  const struct jz_sfc_concat_mtd_child_ops *ops);
struct mtd_info *
jz_sfc_concat_mtd_child_mtd(struct jz_sfc_concat_mtd_child *child);
void *jz_sfc_concat_mtd_child_priv(struct jz_sfc_concat_mtd_child *child);
#endif

static inline uint32_t jz_sfc_gpio_to_port(uint32_t gpio)
{
	return gpio / 32;
}

static inline uint32_t jz_sfc_gpio_to_mask(uint32_t gpio)
{
	return 1U << (gpio % 32);
}

static inline uint32_t jz_sfc_gpio_value(uint32_t active_low,
					 uint32_t active)
{
	return active ? !active_low : active_low;
}

const struct sfc_flash_concat_chip *board_sfc_flash_concat_chips(uint32_t *count);
static inline int
jz_sfc_concat_context_init(struct sfc_flash_concat_context *ctx,
			   const struct sfc_flash_concat_chip *chips,
			   uint32_t count)
{
	uint32_t i;

	if (!ctx || !chips || !count)
		return -1;

	for (i = 0; i < count; i++) {
		if (chips[i].chip_index != i)
			return -1;
		if (!sfc_flash_concat_chip_cs_valid(&chips[i]))
			return -1;
	}

	ctx->chips = chips;
	ctx->chip_count = count;
	ctx->active_chip = JZ_SFC_CONCAT_NO_ACTIVE_CHIP;

	return 0;
}

static inline const struct sfc_flash_concat_chip *
jz_sfc_concat_context_chip(const struct sfc_flash_concat_context *ctx,
			   uint32_t index)
{
	if (!ctx || !ctx->chips || index >= ctx->chip_count)
		return NULL;

	return &ctx->chips[index];
}

const struct sfc_flash_concat_context *jz_sfc_flash_concat_context(void);
const struct sfc_flash_concat_chip *jz_sfc_flash_concat_chip(uint32_t index);
int jz_sfc_flash_concat_set_runtime_topology(
		const struct sfc_cs_topology *topology);
int jz_sfc_flash_concat_init(void);
int jz_sfc_flash_concat_select_boot_chip(void);
int jz_sfc_flash_concat_select_chip(uint32_t index);
int jz_sfc_flash_concat_set_profile(uint32_t index,
		jz_sfc_flash_concat_prepare_profile_t prepare, void *priv);
int jz_sfc_flash_concat_activate(uint32_t index);
void jz_sfc_flash_concat_xfer_begin(void);
void jz_sfc_flash_concat_xfer_end(void);
int jz_sfc_flash_concat_uses_gpio_cs(void);

#if !defined(CONFIG_SPL_BUILD) && !defined(USE_HOSTCC)
int nand_concat_is_managed(nand_info_t *nand);
int nand_concat_read_skip_bad(nand_info_t *nand, loff_t offset,
			      size_t *length, size_t *actual, loff_t lim,
			      u_char *buffer);
int nand_concat_write_skip_bad(nand_info_t *nand, loff_t offset,
			       size_t *length, size_t *actual, loff_t lim,
			       u_char *buffer, int flags);
int nand_concat_erase_opts(nand_info_t *meminfo,
			   const nand_erase_options_t *opts);
#ifdef CONFIG_SFC_NOR_CONCAT
int sfc_nor_concat_select_boot_chip(void);
int sfc_nor_concat_load_persistent_params(
		struct spiflash_info *concat_info);
int sfc_nor_concat_setup_from_params(
		const struct spiflash_info *concat_info);
int sfc_nor_concat_setup_from_static_params(void);
int sfc_nor_concat_is_enabled(void);
int sfc_nor_concat_read(unsigned int from, unsigned int len,
			unsigned char *buf);
int sfc_nor_concat_write(unsigned int to, unsigned int len,
			 unsigned char *buf);
int sfc_nor_concat_erase(unsigned int addr, unsigned int len);
uint32_t sfc_nor_concat_capacity(void);
#ifdef CONFIG_BURNER
int mtd_sfcnor_probe_burner_concat(
		const struct spiflash_info *concat_info);
int sfc_nor_concat_inject_burner_params(uint32_t offset, uint32_t len,
					unsigned char *buf);
#endif
#endif

#ifdef CONFIG_SFC_NAND_CONCAT
struct sfc_flash *jz_sfc_nand_current_flash(void);
void jz_sfc_nand_set_current_flash(struct sfc_flash *current);
int jz_sfc_nand_concat_read_gpio_cache(struct sfc_flash *flash,
				       struct sfc_cdt_xfer *xfer,
				       unsigned short cmd_index,
				       struct jz_sfcnand_ops *ops);
int jz_sfc_nand_erase(struct mtd_info *mtd, struct erase_info *instr);
int jz_sfcnand_read(struct mtd_info *mtd, loff_t from, size_t len,
		    size_t *retlen, u_char *buf);
int jz_sfcnand_write_raw(struct mtd_info *mtd, loff_t to, size_t len,
			 size_t *retlen, const u_char *buf);
int jz_sfcnand_read_oob(struct mtd_info *mtd, loff_t from,
			struct mtd_oob_ops *ops);
int jz_sfcnand_write_oob_raw(struct mtd_info *mtd, loff_t addr,
			     struct mtd_oob_ops *ops);
int sfcnand_block_isbad(struct mtd_info *mtd, loff_t ofs);
int jz_sfcnand_block_markbad(struct mtd_info *mtd, loff_t ofs);
int jz_sfc_nand_is_readonly_partition(uint32_t offset, uint32_t size);
void jz_sfc_nand_get_partition_from_spinand(struct sfc_flash *flash);
int32_t sfc_nand_reset(void);
int32_t jz_sfc_nand_try_id(struct sfc_flash *flash,
			   struct jz_sfcnand_flashinfo *nand_info);
int32_t sfc_nand_dev_init(struct sfc_flash *flash);
int32_t spinand_moudle_init(void);
void jz_sfc_nand_create_cdt_table(struct sfc_flash *flash, uint32_t flag);
void jz_sfc_nand_setup_mtd_geometry(struct mtd_info *mtd,
				    struct nand_chip *chip,
				    struct jz_sfcnand_flashinfo *info);
int32_t jz_sfc_nand_concat_init(void);
extern struct jz_sfcnand_burner_param jz_sfc_nand_burner_param;
#ifdef CONFIG_BURNER
void mtd_sfcnand_partition_analysis(uint32_t blk_sz, uint32_t partcount,
		struct jz_sfcnand_partition *jz_mtd_spinand_partition);
int sfcnand_burner_finish_probe(struct mtd_info *mtd);
int32_t sfcnand_burner_probe_runtime_concat(
		const struct jz_sfcnand_burner_param *param);
#endif
#endif
#endif

#endif
