#ifndef __SFC_CONCAT_H__
#define __SFC_CONCAT_H__

#ifdef USE_HOSTCC
#include <stddef.h>
#include <stdint.h>
#else
#include <linux/types.h>
#endif

#define SFC_CONCAT_ABI_VERSION		2U

#define SFC_FLASH_CONCAT_CS_NATIVE	0U
#define SFC_FLASH_CONCAT_CS_GPIO	1U
#define SFC_FLASH_CONCAT_GPIO_INVALID	0xffffffffU

#define SFC_FLASH_CONCAT_F_DISABLE_QUAD		(1U << 0)
#define SFC_FLASH_CONCAT_F_NATIVE_FUNC_VALID	(1U << 1)
#define SFC_FLASH_CONCAT_NATIVE_FUNC_SHIFT	8U
#define SFC_FLASH_CONCAT_NATIVE_FUNC_MASK \
	(0x0fU << SFC_FLASH_CONCAT_NATIVE_FUNC_SHIFT)
#define SFC_FLASH_CONCAT_F_VALID_MASK \
	(SFC_FLASH_CONCAT_F_DISABLE_QUAD | \
	 SFC_FLASH_CONCAT_F_NATIVE_FUNC_VALID | \
	 SFC_FLASH_CONCAT_NATIVE_FUNC_MASK)
#define SFC_FLASH_CONCAT_NATIVE_FUNC_VALUE(flags) \
	(((flags) & SFC_FLASH_CONCAT_NATIVE_FUNC_MASK) >> \
	 SFC_FLASH_CONCAT_NATIVE_FUNC_SHIFT)
#define SFC_FLASH_CONCAT_NATIVE_FUNC(func) \
	(SFC_FLASH_CONCAT_F_NATIVE_FUNC_VALID | \
	 (((uint32_t)(func) & 0x0fU) << SFC_FLASH_CONCAT_NATIVE_FUNC_SHIFT))

#ifndef SFC_CS_TOPOLOGY_MAX_CHIPS
#ifdef CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS
#define SFC_CS_TOPOLOGY_MAX_CHIPS	CONFIG_SFC_FLASH_CONCAT_MAX_CHIPS
#else
#define SFC_CS_TOPOLOGY_MAX_CHIPS	3
#endif
#endif

#define SFC_CS_TOPOLOGY_MAGIC		0x53435354U /* "SCST" */
#define SFC_CS_TOPOLOGY_VERSION		SFC_CONCAT_ABI_VERSION

struct sfc_flash_concat_chip {
	uint32_t chip_index;
	uint32_t cs_type;
	uint32_t gpio_cs;
	uint32_t gpio_active_low;
	uint32_t flags;
};

struct sfc_cs_topology {
	uint32_t magic;
	uint32_t version;
	uint32_t chip_count;
	struct sfc_flash_concat_chip chips[SFC_CS_TOPOLOGY_MAX_CHIPS];
};

static inline uint32_t sfc_flash_concat_chip_cs_default(uint32_t chip_index)
{
	return chip_index == 0 ? SFC_FLASH_CONCAT_CS_NATIVE :
		SFC_FLASH_CONCAT_CS_GPIO;
}

static inline int sfc_flash_concat_chip_cs_valid(
		const struct sfc_flash_concat_chip *chip)
{
	uint32_t cs_type;

	if (!chip)
		return 0;
	if (chip->flags & ~SFC_FLASH_CONCAT_F_VALID_MASK)
		return 0;
	if ((chip->flags & SFC_FLASH_CONCAT_NATIVE_FUNC_MASK) &&
	    !(chip->flags & SFC_FLASH_CONCAT_F_NATIVE_FUNC_VALID))
		return 0;
	if ((chip->flags & SFC_FLASH_CONCAT_F_NATIVE_FUNC_VALID) &&
	    SFC_FLASH_CONCAT_NATIVE_FUNC_VALUE(chip->flags) > 3)
		return 0;
	if (chip->cs_type > SFC_FLASH_CONCAT_CS_GPIO)
		return 0;

	cs_type = chip->cs_type == SFC_FLASH_CONCAT_CS_GPIO ?
		SFC_FLASH_CONCAT_CS_GPIO :
		sfc_flash_concat_chip_cs_default(chip->chip_index);

	switch (cs_type) {
	case SFC_FLASH_CONCAT_CS_NATIVE:
		return chip->gpio_active_low <= 1;
	case SFC_FLASH_CONCAT_CS_GPIO:
		return chip->gpio_cs != SFC_FLASH_CONCAT_GPIO_INVALID &&
			chip->gpio_active_low <= 1;
	default:
		return 0;
	}
}

static inline int sfc_cs_topology_valid(const struct sfc_cs_topology *topo)
{
	uint32_t i;

	if (!topo)
		return 0;
	if (topo->magic != SFC_CS_TOPOLOGY_MAGIC)
		return 0;
	if (topo->version != SFC_CS_TOPOLOGY_VERSION)
		return 0;
	if (!topo->chip_count || topo->chip_count > SFC_CS_TOPOLOGY_MAX_CHIPS)
		return 0;
	for (i = 0; i < topo->chip_count; i++) {
		const struct sfc_flash_concat_chip *chip = &topo->chips[i];

		if (chip->chip_index != i)
			return 0;
		if (!sfc_flash_concat_chip_cs_valid(chip))
			return 0;
	}

	return 1;
}

static inline uint32_t sfc_flash_concat_chip_cs_effective(
		const struct sfc_flash_concat_chip *chip)
{
	if (!chip)
		return SFC_FLASH_CONCAT_CS_NATIVE;
	if (chip->cs_type == SFC_FLASH_CONCAT_CS_GPIO)
		return SFC_FLASH_CONCAT_CS_GPIO;

	return sfc_flash_concat_chip_cs_default(chip->chip_index);
}

#endif
