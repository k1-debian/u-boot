#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/sfc.h>
#include "jz_sfc_concat.h"

#ifndef CONFIG_SFC_NATIVE_CS_INACTIVE_LEVEL
#define CONFIG_SFC_NATIVE_CS_INACTIVE_LEVEL	1
#endif

static struct sfc_flash_concat_context jz_sfc_concat_ctx = {
	.active_chip = JZ_SFC_CONCAT_NO_ACTIVE_CHIP,
};
static uint32_t jz_sfc_concat_prepared_chip =
	JZ_SFC_CONCAT_NO_ACTIVE_CHIP;
struct jz_sfc_flash_concat_profile {
	jz_sfc_flash_concat_prepare_profile_t prepare;
	void *priv;
};

static struct jz_sfc_flash_concat_profile
	jz_sfc_flash_concat_profiles[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
#ifndef CONFIG_SPL_BUILD
static struct sfc_flash_concat_chip
	jz_sfc_runtime_concat_chips[CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS];
#endif

#ifdef CONFIG_SFC_FLASH_CONCAT_BOARD_CHIPS
static const struct sfc_flash_concat_chip jz_sfc_concat_chips[] = {
	CONFIG_SFC_FLASH_CONCAT_BOARD_CHIPS,
};
#endif

const struct sfc_flash_concat_chip *__weak
board_sfc_flash_concat_chips(uint32_t *count)
{
#ifdef CONFIG_SFC_FLASH_CONCAT_BOARD_CHIPS
	if (count)
		*count = ARRAY_SIZE(jz_sfc_concat_chips);

	return jz_sfc_concat_chips;
#else
	if (count)
		*count = 0;

	return NULL;
#endif
}

static uint32_t jz_sfc_native_cs_gpio_from_chips(
		const struct sfc_flash_concat_chip *chips, uint32_t count)
{
	uint32_t i;

	for (i = 0; chips && i < count; i++) {
		if (chips[i].chip_index == 0 &&
		    sfc_flash_concat_chip_cs_effective(&chips[i]) ==
		    SFC_FLASH_CONCAT_CS_NATIVE &&
		    chips[i].gpio_cs != JZ_SFC_INVALID_NATIVE_CS_GPIO)
			return chips[i].gpio_cs;
	}

	return JZ_SFC_INVALID_NATIVE_CS_GPIO;
}

static uint32_t jz_sfc_native_cs_func_from_chips(
		const struct sfc_flash_concat_chip *chips, uint32_t count)
{
	uint32_t i;

	for (i = 0; chips && i < count; i++) {
		if (chips[i].chip_index == 0 &&
		    sfc_flash_concat_chip_cs_effective(&chips[i]) ==
		    SFC_FLASH_CONCAT_CS_NATIVE &&
		    (chips[i].flags & SFC_FLASH_CONCAT_F_NATIVE_FUNC_VALID)) {
			uint32_t func =
				SFC_FLASH_CONCAT_NATIVE_FUNC_VALUE(chips[i].flags);

			return func <= 3 ? func : JZ_SFC_INVALID_NATIVE_CS_FUNC;
		}
	}

	return JZ_SFC_INVALID_NATIVE_CS_FUNC;
}

const struct sfc_flash_concat_context *jz_sfc_flash_concat_context(void)
{
	return &jz_sfc_concat_ctx;
}

const struct sfc_flash_concat_chip *jz_sfc_flash_concat_chip(uint32_t index)
{
	return jz_sfc_concat_context_chip(&jz_sfc_concat_ctx, index);
}

static uint32_t jz_sfc_native_cs_gpio_value(void)
{
	const struct sfc_flash_concat_chip *chips;
	uint32_t count;

	if (jz_sfc_concat_ctx.chips && jz_sfc_concat_ctx.chip_count)
		return jz_sfc_native_cs_gpio_from_chips(jz_sfc_concat_ctx.chips,
							jz_sfc_concat_ctx.chip_count);

	chips = board_sfc_flash_concat_chips(&count);
	return jz_sfc_native_cs_gpio_from_chips(chips, count);
}

static uint32_t jz_sfc_native_cs_func_value(void)
{
	const struct sfc_flash_concat_chip *chips;
	uint32_t count;

	if (jz_sfc_concat_ctx.chips && jz_sfc_concat_ctx.chip_count)
		return jz_sfc_native_cs_func_from_chips(jz_sfc_concat_ctx.chips,
							jz_sfc_concat_ctx.chip_count);

	chips = board_sfc_flash_concat_chips(&count);
	return jz_sfc_native_cs_func_from_chips(chips, count);
}

static void jz_sfc_native_cs_func(void)
{
	uint32_t gpio;
	uint32_t func;

	gpio = jz_sfc_native_cs_gpio_value();
	func = jz_sfc_native_cs_func_value();
	if (gpio == JZ_SFC_INVALID_NATIVE_CS_GPIO ||
	    func == JZ_SFC_INVALID_NATIVE_CS_FUNC)
		return;

	gpio_set_func(jz_sfc_gpio_to_port(gpio), func,
		      jz_sfc_gpio_to_mask(gpio));
}

static void jz_sfc_native_cs_inactive(void)
{
	uint32_t gpio;

	gpio = jz_sfc_native_cs_gpio_value();
	if (gpio == JZ_SFC_INVALID_NATIVE_CS_GPIO)
		return;

	gpio_set_func(jz_sfc_gpio_to_port(gpio),
		      CONFIG_SFC_NATIVE_CS_INACTIVE_LEVEL ?
		      GPIO_OUTPUT1 : GPIO_OUTPUT0,
		      jz_sfc_gpio_to_mask(gpio));
}

static void jz_sfc_set_gpio_cs(const struct sfc_flash_concat_chip *chip,
			       uint32_t active)
{
	uint32_t value;

	if (sfc_flash_concat_chip_cs_effective(chip) !=
	    SFC_FLASH_CONCAT_CS_GPIO)
		return;

	value = jz_sfc_gpio_value(chip->gpio_active_low, active);
	gpio_direction_output(chip->gpio_cs, value);
}

static void jz_sfc_set_native_cs(uint32_t native_cs)
{
	uint32_t tmp;

	tmp = readl(SFC_BASE + SFC_GLB1);
	tmp &= ~GLB1_CHIP_SEL_MSK;
	tmp |= (native_cs ? GLB1_CHIP_SEL_1 : GLB1_CHIP_SEL_0)
		<< GLB1_CHIP_SEL_OFFSET;
	writel(tmp, SFC_BASE + SFC_GLB1);
}

static void jz_sfc_flash_concat_deactivate_chips(
		const struct sfc_flash_concat_chip *chips, uint32_t count)
{
	uint32_t i;

	for (i = 0; chips && i < count; i++)
		jz_sfc_set_gpio_cs(&chips[i], 0);
}

static int jz_sfc_flash_concat_has_gpio_cs(
		const struct sfc_flash_concat_chip *chips, uint32_t count)
{
	uint32_t i;

	for (i = 0; chips && i < count; i++) {
		if (sfc_flash_concat_chip_cs_effective(&chips[i]) ==
		    SFC_FLASH_CONCAT_CS_GPIO)
			return 1;
	}

	return 0;
}

static int jz_sfc_flash_concat_set_chips(
		const struct sfc_flash_concat_chip *chips, uint32_t count)
{
	uint32_t gpio;
	uint32_t func;

	if (jz_sfc_flash_concat_has_gpio_cs(chips, count)) {
		gpio = jz_sfc_native_cs_gpio_from_chips(chips, count);
		func = jz_sfc_native_cs_func_from_chips(chips, count);
		if (gpio == JZ_SFC_INVALID_NATIVE_CS_GPIO ||
		    func == JZ_SFC_INVALID_NATIVE_CS_FUNC)
			return -1;
	}

	jz_sfc_flash_concat_deactivate_chips(jz_sfc_concat_ctx.chips,
					     jz_sfc_concat_ctx.chip_count);
	memset(&jz_sfc_concat_ctx, 0, sizeof(jz_sfc_concat_ctx));
	memset(jz_sfc_flash_concat_profiles, 0,
	       sizeof(jz_sfc_flash_concat_profiles));
	jz_sfc_concat_ctx.active_chip = JZ_SFC_CONCAT_NO_ACTIVE_CHIP;
	jz_sfc_concat_prepared_chip = JZ_SFC_CONCAT_NO_ACTIVE_CHIP;

	if (jz_sfc_concat_context_init(&jz_sfc_concat_ctx, chips, count))
		return -1;

	jz_sfc_flash_concat_deactivate_chips(jz_sfc_concat_ctx.chips,
					     jz_sfc_concat_ctx.chip_count);
	jz_sfc_set_native_cs(0);
	jz_sfc_native_cs_inactive();

	return 0;
}

#ifndef CONFIG_SPL_BUILD
int jz_sfc_flash_concat_set_runtime_topology(
		const struct sfc_cs_topology *topology)
{
	uint32_t count;

	if (!sfc_cs_topology_valid(topology))
		return -1;

	count = topology->chip_count;
	if (count > CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS)
		return -1;

	memcpy(jz_sfc_runtime_concat_chips, topology->chips,
	       sizeof(jz_sfc_runtime_concat_chips[0]) * count);

	return jz_sfc_flash_concat_set_chips(jz_sfc_runtime_concat_chips,
					     count);
}
#endif

int jz_sfc_flash_concat_init(void)
{
	const struct sfc_flash_concat_chip *chips;
	uint32_t count;

	if (jz_sfc_concat_ctx.chips && jz_sfc_concat_ctx.chip_count)
		return 0;

	chips = board_sfc_flash_concat_chips(&count);
	if (!chips || !count)
		return -1;

	if (jz_sfc_flash_concat_set_chips(chips, count))
		return -1;

	return 0;
}

int jz_sfc_flash_concat_select_boot_chip(void)
{
	if (jz_sfc_flash_concat_init())
		return -1;

	return jz_sfc_flash_concat_select_chip(0);
}

int jz_sfc_flash_concat_select_chip(uint32_t index)
{
	const struct sfc_flash_concat_chip *chips;
	const struct sfc_flash_concat_chip *chip;
	uint32_t i;

	chip = jz_sfc_concat_context_chip(&jz_sfc_concat_ctx, index);
	if (!chip)
		return -1;

	chips = jz_sfc_concat_ctx.chips;
	if (!chips)
		return -1;

	for (i = 0; i < jz_sfc_concat_ctx.chip_count; i++)
		jz_sfc_set_gpio_cs(&chips[i], 0);

	if (sfc_flash_concat_chip_cs_effective(chip) ==
	    SFC_FLASH_CONCAT_CS_GPIO) {
		jz_sfc_set_native_cs(0);
		jz_sfc_native_cs_inactive();
	} else {
		jz_sfc_native_cs_func();
		jz_sfc_set_native_cs(0);
	}

	jz_sfc_concat_ctx.active_chip = index;

	return 0;
}

int jz_sfc_flash_concat_set_profile(uint32_t index,
		jz_sfc_flash_concat_prepare_profile_t prepare, void *priv)
{
	if (!jz_sfc_concat_ctx.chips || !jz_sfc_concat_ctx.chip_count)
		return -ENODEV;
	if (index >= jz_sfc_concat_ctx.chip_count ||
	    index >= CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS)
		return -EINVAL;
	if (!prepare)
		return -EINVAL;

	jz_sfc_flash_concat_profiles[index].prepare = prepare;
	jz_sfc_flash_concat_profiles[index].priv = priv;
	if (jz_sfc_concat_prepared_chip == index)
		jz_sfc_concat_prepared_chip = JZ_SFC_CONCAT_NO_ACTIVE_CHIP;

	return 0;
}

int jz_sfc_flash_concat_activate(uint32_t index)
{
	const struct sfc_flash_concat_chip *chip;
	struct jz_sfc_flash_concat_profile *profile;
	int ret;

	chip = jz_sfc_concat_context_chip(&jz_sfc_concat_ctx, index);
	if (!chip)
		return -ENODEV;
	if (index >= CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS)
		return -EINVAL;

	profile = &jz_sfc_flash_concat_profiles[index];
	if (!profile->prepare)
		return -ENODEV;
	if (jz_sfc_concat_ctx.active_chip == index &&
	    jz_sfc_concat_prepared_chip == index)
		return 0;

	ret = jz_sfc_flash_concat_select_chip(index);
	if (ret)
		return ret;

	ret = profile->prepare(profile->priv);
	if (ret)
		return ret;

	jz_sfc_concat_prepared_chip = index;

	return 0;
}

void jz_sfc_flash_concat_xfer_begin(void)
{
	const struct sfc_flash_concat_chip *chip;

	chip = jz_sfc_concat_context_chip(&jz_sfc_concat_ctx,
					  jz_sfc_concat_ctx.active_chip);
	if (!chip || sfc_flash_concat_chip_cs_effective(chip) !=
	    SFC_FLASH_CONCAT_CS_GPIO)
		return;

	jz_sfc_set_gpio_cs(chip, 1);
}

void jz_sfc_flash_concat_xfer_end(void)
{
	const struct sfc_flash_concat_chip *chip;

	chip = jz_sfc_concat_context_chip(&jz_sfc_concat_ctx,
					  jz_sfc_concat_ctx.active_chip);
	if (!chip || sfc_flash_concat_chip_cs_effective(chip) !=
	    SFC_FLASH_CONCAT_CS_GPIO)
		return;

	jz_sfc_set_gpio_cs(chip, 0);

}

int jz_sfc_flash_concat_uses_gpio_cs(void)
{
	const struct sfc_flash_concat_chip *chip;

	chip = jz_sfc_concat_context_chip(&jz_sfc_concat_ctx,
					  jz_sfc_concat_ctx.active_chip);
	return chip &&
		sfc_flash_concat_chip_cs_effective(chip) ==
		SFC_FLASH_CONCAT_CS_GPIO;
}

#ifndef CONFIG_SPL_BUILD
static struct jz_sfc_concat_mtd_child *
jz_sfc_concat_mtd_child_from_mtd(struct mtd_info *mtd)
{
	if (!mtd)
		return NULL;

	return container_of(mtd, struct jz_sfc_concat_mtd_child, mtd);
}

static int jz_sfc_concat_mtd_child_prepare(
		struct jz_sfc_concat_mtd_child *child)
{
	if (!child || !child->ops)
		return -EINVAL;

	return jz_sfc_flash_concat_activate(child->chip_index);
}

static int jz_sfc_concat_mtd_child_read(struct mtd_info *mtd, loff_t from,
					size_t len, size_t *retlen,
					u_char *buf)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->read)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->read(child, from, len, retlen, buf);
}

static int jz_sfc_concat_mtd_child_write(struct mtd_info *mtd, loff_t to,
					 size_t len, size_t *retlen,
					 const u_char *buf)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->write)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->write(child, to, len, retlen, buf);
}

static int jz_sfc_concat_mtd_child_erase(struct mtd_info *mtd,
					 struct erase_info *instr)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->erase)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->erase(child, instr);
}

static int jz_sfc_concat_mtd_child_read_oob(struct mtd_info *mtd,
					    loff_t from,
					    struct mtd_oob_ops *ops)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->read_oob)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->read_oob(child, from, ops);
}

static int jz_sfc_concat_mtd_child_write_oob(struct mtd_info *mtd,
					     loff_t to,
					     struct mtd_oob_ops *ops)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->write_oob)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->write_oob(child, to, ops);
}

static int jz_sfc_concat_mtd_child_block_isbad(struct mtd_info *mtd,
					       loff_t ofs)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->block_isbad)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->block_isbad(child, ofs);
}

static int jz_sfc_concat_mtd_child_block_markbad(struct mtd_info *mtd,
						 loff_t ofs)
{
	struct jz_sfc_concat_mtd_child *child =
		jz_sfc_concat_mtd_child_from_mtd(mtd);
	int ret;

	if (!child || !child->ops || !child->ops->block_markbad)
		return -EOPNOTSUPP;

	ret = jz_sfc_concat_mtd_child_prepare(child);
	if (ret)
		return ret;

	return child->ops->block_markbad(child, ofs);
}

void jz_sfc_concat_mtd_child_init(struct jz_sfc_concat_mtd_child *child,
				  const char *name, uint32_t chip_index,
				  void *priv,
				  const struct jz_sfc_concat_mtd_child_ops *ops)
{
	struct mtd_info *mtd;

	if (!child)
		return;

	memset(child, 0, sizeof(*child));
	child->chip_index = chip_index;
	child->priv = priv;
	child->ops = ops;

	mtd = &child->mtd;
	mtd->name = name;
	if (!ops)
		return;

	if (ops->erase)
		mtd->_erase = jz_sfc_concat_mtd_child_erase;
	if (ops->read)
		mtd->_read = jz_sfc_concat_mtd_child_read;
	if (ops->write)
		mtd->_write = jz_sfc_concat_mtd_child_write;
	if (ops->read_oob)
		mtd->_read_oob = jz_sfc_concat_mtd_child_read_oob;
	if (ops->write_oob)
		mtd->_write_oob = jz_sfc_concat_mtd_child_write_oob;
	if (ops->block_isbad)
		mtd->_block_isbad = jz_sfc_concat_mtd_child_block_isbad;
	if (ops->block_markbad)
		mtd->_block_markbad = jz_sfc_concat_mtd_child_block_markbad;
}

struct mtd_info *
jz_sfc_concat_mtd_child_mtd(struct jz_sfc_concat_mtd_child *child)
{
	return child ? &child->mtd : NULL;
}

void *jz_sfc_concat_mtd_child_priv(struct jz_sfc_concat_mtd_child *child)
{
	return child ? child->priv : NULL;
}
#endif
