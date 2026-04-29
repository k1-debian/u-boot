#include <common.h>
#include <asm/errno.h>
#include <dm_verity_boot.h>
#include <dm_verity_runtime.h>
#include <mmc.h>
#include <part.h>

#include "dm_verity_manifest_source.h"
#include "dm_verity_media.h"

#define DM_VERITY_MANIFEST_SLOT_MAGIC	0x56534c54U
#define DM_VERITY_MANIFEST_SLOT_VERSION	1U
#define DM_VERITY_ROOTFS_TAIL_MAGIC	0x5654524cU
#define DM_VERITY_ROOTFS_TAIL_VERSION	1U
#define DM_VERITY_MANIFEST_SLOT_RESERVED_LEN	28

struct dm_verity_manifest_slot_v1 {
	unsigned int magic;
	unsigned short version;
	unsigned short header_size;
	unsigned int flags;
	unsigned int total_size;
	unsigned int manifest_offset;
	unsigned int manifest_size;
	unsigned int sig_offset;
	unsigned int sig_size;
	char reserved[DM_VERITY_MANIFEST_SLOT_RESERVED_LEN];
} __attribute__((packed));

struct dm_verity_rootfs_tail_v1 {
	unsigned int magic;
	unsigned short version;
	unsigned short header_size;
	unsigned int flags;
	unsigned int image_size;
	unsigned int slot_offset;
	unsigned int slot_size;
	char reserved[488];
} __attribute__((packed));

static int dm_verity_load_rootfs_tail_slot(
	const struct dm_verity_manifest_source_desc *source,
	struct mmc **out_mmc, disk_partition_t *out_part,
	struct dm_verity_manifest_slot_v1 *out_slot,
	struct dm_verity_rootfs_tail_v1 *out_tail)
{
	struct mmc *mmc = NULL;
	disk_partition_t part_info;
	struct dm_verity_manifest_slot_v1 slot;
	struct dm_verity_rootfs_tail_v1 tail;
	unsigned long long part_bytes;
	unsigned int tail_offset;
	int ret;

	if (!source || !out_slot)
		return -EINVAL;
	if (source->type != DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL)
		return -EINVAL;
	if (source->partition.media != DM_VERITY_SOURCE_MEDIA_MMC)
		return -ENOSYS;

	memset(&part_info, 0, sizeof(part_info));
	memset(&slot, 0, sizeof(slot));
	memset(&tail, 0, sizeof(tail));

	ret = dm_verity_get_mmc_partition_info_by_name(
		source->partition.location.mmc.dev,
		source->partition.location.mmc.max_part,
		source->partition.location.mmc.part_name,
		&mmc, &part_info);
	if (ret)
		return ret;

	part_bytes = (unsigned long long)part_info.size * part_info.blksz;
	if (part_bytes < sizeof(tail) || part_bytes > 0xffffffffULL)
		return -EFBIG;

	tail_offset = (unsigned int)(part_bytes - sizeof(tail));
	ret = dm_verity_read_mmc_partition_resolved(
		source->partition.location.mmc.dev, mmc, &part_info,
		source->partition.offset, tail_offset, &tail, sizeof(tail));
	if (ret)
		return ret;

	if (tail.magic != DM_VERITY_ROOTFS_TAIL_MAGIC)
		return -EINVAL;
	if (tail.version != DM_VERITY_ROOTFS_TAIL_VERSION)
		return -EPROTONOSUPPORT;
	if (tail.header_size != sizeof(tail))
		return -EINVAL;
	if ((unsigned long long)tail.image_size != part_bytes)
		return -EINVAL;
	if (tail.slot_size < sizeof(slot))
		return -EINVAL;
	if ((unsigned long long)tail.slot_offset + tail.slot_size > part_bytes)
		return -EINVAL;
	if ((unsigned long long)tail.slot_offset + tail.slot_size > tail_offset)
		return -EINVAL;

	ret = dm_verity_read_mmc_partition_resolved(
		source->partition.location.mmc.dev, mmc, &part_info,
		source->partition.offset, tail.slot_offset, &slot, sizeof(slot));
	if (ret)
		return ret;

	if (slot.magic != DM_VERITY_MANIFEST_SLOT_MAGIC)
		return -EINVAL;
	if (slot.version != DM_VERITY_MANIFEST_SLOT_VERSION)
		return -EPROTONOSUPPORT;
	if (slot.header_size != sizeof(slot))
		return -EINVAL;
	if (slot.total_size < sizeof(slot))
		return -EINVAL;
	if (slot.total_size > tail.slot_size)
		return -EINVAL;
	if (slot.manifest_offset + slot.manifest_size > slot.total_size)
		return -EINVAL;
	if (slot.sig_size && slot.sig_offset + slot.sig_size > slot.total_size)
		return -EINVAL;

	if (out_mmc)
		*out_mmc = mmc;
	if (out_part)
		memcpy(out_part, &part_info, sizeof(*out_part));
	if (out_tail)
		memcpy(out_tail, &tail, sizeof(*out_tail));
	memcpy(out_slot, &slot, sizeof(*out_slot));
	return 0;
}

static int dm_verity_manifest_read(void *priv, void *buf,
				   unsigned int size,
				   unsigned int *actual_size)
{
	const struct dm_verity_manifest_runtime *runtime = priv;
	int ret;

	if (!runtime || !buf || !size)
		return -EINVAL;

	switch (runtime->source.type) {
	case DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL: {
		struct mmc *mmc = NULL;
		disk_partition_t part_info;
		struct dm_verity_manifest_slot_v1 slot;
		struct dm_verity_rootfs_tail_v1 tail;

		memset(&part_info, 0, sizeof(part_info));
		memset(&slot, 0, sizeof(slot));
		memset(&tail, 0, sizeof(tail));
		ret = dm_verity_load_rootfs_tail_slot(&runtime->source,
						      &mmc, &part_info,
						      &slot, &tail);
		if (ret)
			return ret;
		if (slot.manifest_size > size)
			return -EMSGSIZE;

		ret = dm_verity_read_mmc_partition_resolved(
			runtime->source.partition.location.mmc.dev,
			mmc, &part_info, runtime->source.partition.offset,
			tail.slot_offset + slot.manifest_offset,
			buf, slot.manifest_size);
		if (ret)
			return ret;
		if (actual_size)
			*actual_size = slot.manifest_size;
		return 0;
	}
	case DM_VERITY_MANIFEST_SOURCE_MMC_RAW:
		return dm_verity_read_partition_source(&runtime->source.partition,
						       0, buf, size,
						       actual_size);
	default:
		return -EINVAL;
	}
}

static int dm_verity_manifest_read_sig(void *priv, void *buf,
				       unsigned int size,
				       unsigned int *actual_size)
{
	const struct dm_verity_manifest_runtime *runtime = priv;
	int ret;

	if (!runtime || !buf || !size)
		return -EINVAL;

	switch (runtime->source.type) {
	case DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL: {
		struct mmc *mmc = NULL;
		disk_partition_t part_info;
		struct dm_verity_manifest_slot_v1 slot;
		struct dm_verity_rootfs_tail_v1 tail;

		memset(&part_info, 0, sizeof(part_info));
		memset(&slot, 0, sizeof(slot));
		memset(&tail, 0, sizeof(tail));
		ret = dm_verity_load_rootfs_tail_slot(&runtime->source,
						      &mmc, &part_info,
						      &slot, &tail);
		if (ret)
			return ret;
		if (!slot.sig_size)
			return -ENODATA;
		if (slot.sig_size > size)
			return -EMSGSIZE;

		ret = dm_verity_read_mmc_partition_resolved(
			runtime->source.partition.location.mmc.dev,
			mmc, &part_info, runtime->source.partition.offset,
			tail.slot_offset + slot.sig_offset,
			buf, slot.sig_size);
		if (ret)
			return ret;
		if (actual_size)
			*actual_size = slot.sig_size;
		return 0;
	}
	case DM_VERITY_MANIFEST_SOURCE_MMC_RAW:
		if (!runtime->source.sig_length)
			return -ENODATA;
		if (runtime->source.sig_length > size)
			return -EMSGSIZE;
		return dm_verity_read_partition_source(&runtime->source.partition,
						       runtime->source.sig_offset,
						       buf,
						       runtime->source.sig_length,
						       actual_size);
	default:
		return -EINVAL;
	}
}

static const struct dm_verity_manifest_ops dm_verity_manifest_ops = {
	.read_manifest = dm_verity_manifest_read,
	.read_manifest_sig = dm_verity_manifest_read_sig,
};

void dm_verity_manifest_runtime_init(
	struct dm_verity_manifest_runtime *runtime,
	const struct dm_verity_manifest_source_desc *source)
{
	if (!runtime)
		return;

	memset(runtime, 0, sizeof(*runtime));
	if (source)
		memcpy(&runtime->source, source, sizeof(runtime->source));
}

const struct dm_verity_manifest_ops *dm_verity_get_manifest_ops(void)
{
	return &dm_verity_manifest_ops;
}
