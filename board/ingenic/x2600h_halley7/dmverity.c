#include <common.h>
#include "dmverity.h"

#if !defined(CONFIG_SPL_BUILD) && defined(CONFIG_BOARD_DM_VERITY_HELPER)

#include <asm/errno.h>

#ifndef CONFIG_BOARD_DM_VERITY_BOOTARGS_BASE
#error "CONFIG_BOARD_DM_VERITY_BOOTARGS_BASE is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_KERNEL_BOOTCMD
#error "CONFIG_BOARD_DM_VERITY_KERNEL_BOOTCMD is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_DATA_DEV_MAJOR
#error "CONFIG_BOARD_DM_VERITY_DATA_DEV_MAJOR is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_DATA_DEV_MINOR
#error "CONFIG_BOARD_DM_VERITY_DATA_DEV_MINOR is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_HASH_DEV_MAJOR
#error "CONFIG_BOARD_DM_VERITY_HASH_DEV_MAJOR is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_HASH_DEV_MINOR
#error "CONFIG_BOARD_DM_VERITY_HASH_DEV_MINOR is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_MANIFEST_MMC_DEV
#error "CONFIG_BOARD_DM_VERITY_MANIFEST_MMC_DEV is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_MANIFEST_MMC_MAX_PART
#error "CONFIG_BOARD_DM_VERITY_MANIFEST_MMC_MAX_PART is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_MANIFEST_PART_NAME
#error "CONFIG_BOARD_DM_VERITY_MANIFEST_PART_NAME is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_DATA_MMC_DEV
#error "CONFIG_BOARD_DM_VERITY_DATA_MMC_DEV is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_DATA_MMC_MAX_PART
#error "CONFIG_BOARD_DM_VERITY_DATA_MMC_MAX_PART is required"
#endif

static void dm_verity_fill_mmc_partition_source(
	struct dm_verity_partition_source_desc *source,
	const char *part_name, unsigned int offset, unsigned int length)
{
	memset(source, 0, sizeof(*source));
	source->media = DM_VERITY_SOURCE_MEDIA_MMC;
	source->offset = offset;
	source->length = length;
	source->location.mmc.dev = CONFIG_BOARD_DM_VERITY_MANIFEST_MMC_DEV;
	source->location.mmc.max_part = CONFIG_BOARD_DM_VERITY_MANIFEST_MMC_MAX_PART;
	source->location.mmc.part_name = part_name;
}

#if defined(CONFIG_BOARD_DM_VERITY_PUBKEY_SOURCE_KEYBOX) && \
	CONFIG_BOARD_DM_VERITY_PUBKEY_SOURCE_KEYBOX

#ifndef CONFIG_BOARD_DM_VERITY_KEYBOX_OBJECT_NAME
#error "CONFIG_BOARD_DM_VERITY_KEYBOX_OBJECT_NAME is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_KEYBOX_OBJECT_USAGE
#error "CONFIG_BOARD_DM_VERITY_KEYBOX_OBJECT_USAGE is required"
#endif

static void dm_verity_fill_pubkey_keybox_source(
	struct dm_verity_pubkey_source_desc *source)
{
	memset(source, 0, sizeof(*source));
	source->type = DM_VERITY_PUBKEY_SOURCE_KEYBOX;
	source->partition.offset = CONFIG_BOARD_DM_VERITY_KEYBOX_OFFSET;
	source->partition.length = CONFIG_BOARD_DM_VERITY_KEYBOX_LENGTH;

#if defined(CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_MMC) && \
	CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_MMC
#ifndef CONFIG_BOARD_DM_VERITY_KEYBOX_MMC_DEV
#error "CONFIG_BOARD_DM_VERITY_KEYBOX_MMC_DEV is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_KEYBOX_MMC_MAX_PART
#error "CONFIG_BOARD_DM_VERITY_KEYBOX_MMC_MAX_PART is required"
#endif
#ifndef CONFIG_BOARD_DM_VERITY_KEYBOX_PART_NAME
#error "CONFIG_BOARD_DM_VERITY_KEYBOX_PART_NAME is required"
#endif
	source->partition.media = DM_VERITY_SOURCE_MEDIA_MMC;
	source->partition.location.mmc.dev = CONFIG_BOARD_DM_VERITY_KEYBOX_MMC_DEV;
	source->partition.location.mmc.max_part =
		CONFIG_BOARD_DM_VERITY_KEYBOX_MMC_MAX_PART;
	source->partition.location.mmc.part_name =
		CONFIG_BOARD_DM_VERITY_KEYBOX_PART_NAME;
#elif defined(CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_NOR) && \
	CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_NOR
	source->partition.media = DM_VERITY_SOURCE_MEDIA_NOR;
#elif defined(CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_NAND) && \
	CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_NAND
	source->partition.media = DM_VERITY_SOURCE_MEDIA_NAND;
#else
#error "CONFIG_BOARD_DM_VERITY_KEYBOX_SOURCE_* is not configured"
#endif

	source->selector.name = CONFIG_BOARD_DM_VERITY_KEYBOX_OBJECT_NAME;
	source->selector.class_id = DM_VERITY_PUBKEY_CLASS_PUBKEY;
	source->selector.usage = CONFIG_BOARD_DM_VERITY_KEYBOX_OBJECT_USAGE;
}
#endif

static void dm_verity_fill_data_source(
	struct dm_verity_data_source_desc *source)
{
	memset(source, 0, sizeof(*source));
	source->media = DM_VERITY_SOURCE_MEDIA_MMC;
	source->location.mmc.dev = CONFIG_BOARD_DM_VERITY_DATA_MMC_DEV;
	source->location.mmc.max_part = CONFIG_BOARD_DM_VERITY_DATA_MMC_MAX_PART;
}

int board_dm_verity_helper_get_config(struct dm_verity_boot_config *out)
{
	if (!out)
		return -EINVAL;

#if !defined(CONFIG_BOARD_DM_VERITY_ENABLE) || !CONFIG_BOARD_DM_VERITY_ENABLE
	return -ENOSYS;
#else
	memset(out, 0, sizeof(*out));

	out->bootargs_base = CONFIG_BOARD_DM_VERITY_BOOTARGS_BASE;
	out->kernel_bootcmd = CONFIG_BOARD_DM_VERITY_KERNEL_BOOTCMD;
	out->data_dev_major = CONFIG_BOARD_DM_VERITY_DATA_DEV_MAJOR;
	out->data_dev_minor = CONFIG_BOARD_DM_VERITY_DATA_DEV_MINOR;
	out->hash_dev_major = CONFIG_BOARD_DM_VERITY_HASH_DEV_MAJOR;
	out->hash_dev_minor = CONFIG_BOARD_DM_VERITY_HASH_DEV_MINOR;

#if defined(CONFIG_BOARD_DM_VERITY_PUBKEY_SOURCE_KEYBOX) && \
	CONFIG_BOARD_DM_VERITY_PUBKEY_SOURCE_KEYBOX
	dm_verity_fill_pubkey_keybox_source(&out->pubkey_source);
#elif defined(CONFIG_BOARD_DM_VERITY_PUBKEY_SOURCE_DIRECT) && \
	CONFIG_BOARD_DM_VERITY_PUBKEY_SOURCE_DIRECT
	return -ENOSYS;
#else
	return -ENOSYS;
#endif

#if defined(CONFIG_BOARD_DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL) && \
	CONFIG_BOARD_DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL
	out->manifest_source.type = DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL;
#elif defined(CONFIG_BOARD_DM_VERITY_MANIFEST_SOURCE_MMC_RAW) && \
	CONFIG_BOARD_DM_VERITY_MANIFEST_SOURCE_MMC_RAW
	out->manifest_source.type = DM_VERITY_MANIFEST_SOURCE_MMC_RAW;
	out->manifest_source.sig_offset =
		CONFIG_BOARD_DM_VERITY_MANIFEST_SIG_OFFSET;
	out->manifest_source.sig_length =
		CONFIG_BOARD_DM_VERITY_MANIFEST_SIG_LENGTH;
#else
	return -ENOSYS;
#endif

	dm_verity_fill_mmc_partition_source(
		&out->manifest_source.partition,
		CONFIG_BOARD_DM_VERITY_MANIFEST_PART_NAME,
		CONFIG_BOARD_DM_VERITY_MANIFEST_OFFSET,
		CONFIG_BOARD_DM_VERITY_MANIFEST_LENGTH);

#if defined(CONFIG_BOARD_DM_VERITY_DATA_SOURCE_MMC) && \
	CONFIG_BOARD_DM_VERITY_DATA_SOURCE_MMC
	dm_verity_fill_data_source(&out->data_source);
#else
	return -ENOSYS;
#endif

	return 0;
#endif
}

#endif
