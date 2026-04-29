#include <common.h>
#include <asm/errno.h>
#include <dm_verity_boot.h>
#include <mmc.h>
#include <part.h>

#include "dm_verity_media.h"

int dm_verity_get_mmc_partition_info_by_name(int mmc_dev, int max_part,
					     const char *part_name,
					     struct mmc **out_mmc,
					     disk_partition_t *out_part)
{
#ifdef CONFIG_MMC
	struct mmc *mmc;
	disk_partition_t part_info;
	int part;

	if (!part_name || !out_part)
		return -EINVAL;

	mmc = find_mmc_device(mmc_dev);
	if (!mmc)
		return -ENODEV;

	if (mmc_init(mmc))
		return -EIO;

	init_part(&mmc->block_dev);

	memset(&part_info, 0, sizeof(part_info));
	for (part = 1; part <= max_part; part++) {
		if (get_partition_info(&mmc->block_dev, part, &part_info))
			continue;
		if (strcmp((const char *)part_info.name, part_name))
			continue;

		if (out_mmc)
			*out_mmc = mmc;
		memcpy(out_part, &part_info, sizeof(*out_part));
		return 0;
	}

	return -ENOENT;
#else
	(void)mmc_dev;
	(void)max_part;
	(void)part_name;
	(void)out_mmc;
	(void)out_part;
	return -ENOSYS;
#endif
}

int dm_verity_read_mmc_partition_resolved(int mmc_dev, struct mmc *mmc,
					  const disk_partition_t *part_info,
					  unsigned int base_offset,
					  unsigned int offset,
					  void *buf, unsigned int size)
{
#ifdef CONFIG_MMC
	unsigned char block_buf[512];
	unsigned char *dst = buf;
	lbaint_t lba;
	lbaint_t blkcnt;
	unsigned int copied = 0;
	unsigned int in_block;
	unsigned int blksz;
	unsigned int chunk;
	unsigned long long part_bytes;
	unsigned long long start_offset;

	if (!mmc || !part_info || !buf || !size)
		return -EINVAL;
	if (!mmc->block_dev.blksz)
		return -EINVAL;

	blksz = mmc->block_dev.blksz;
	if (blksz > sizeof(block_buf))
		return -E2BIG;

	part_bytes = (unsigned long long)part_info->size * part_info->blksz;
	start_offset = (unsigned long long)base_offset + offset;
	if (start_offset + size > part_bytes)
		return -EFBIG;

	lba = part_info->start + (start_offset / blksz);
	in_block = start_offset % blksz;
	while (copied < size) {
		if (!in_block && size - copied >= blksz) {
			blkcnt = (size - copied) / blksz;
			if (mmc->block_dev.block_read(mmc_dev, lba, blkcnt,
						      dst + copied) != blkcnt)
				return -EIO;

			copied += blkcnt * blksz;
			lba += blkcnt;
			continue;
		}

		if (mmc->block_dev.block_read(mmc_dev, lba, 1, block_buf) != 1)
			return -EIO;

		chunk = blksz - in_block;
		if (chunk > size - copied)
			chunk = size - copied;

		memcpy(dst + copied, block_buf + in_block, chunk);
		copied += chunk;
		lba++;
		in_block = 0;
	}

	return 0;
#else
	(void)mmc_dev;
	(void)mmc;
	(void)part_info;
	(void)base_offset;
	(void)offset;
	(void)buf;
	(void)size;
	return -ENOSYS;
#endif
}

int dm_verity_read_partition_source(
	const struct dm_verity_partition_source_desc *source,
	unsigned int offset, void *buf, unsigned int size,
	unsigned int *actual_size)
{
	if (!source || !buf || !size)
		return -EINVAL;
	if (source->length && offset + size > source->length)
		return -EFBIG;

	switch (source->media) {
	case DM_VERITY_SOURCE_MEDIA_MMC: {
		struct mmc *mmc = NULL;
		disk_partition_t part_info;
		int ret;

		ret = dm_verity_get_mmc_partition_info_by_name(
			source->location.mmc.dev,
			source->location.mmc.max_part,
			source->location.mmc.part_name,
			&mmc, &part_info);
		if (ret)
			return ret;

		ret = dm_verity_read_mmc_partition_resolved(
			source->location.mmc.dev, mmc, &part_info,
			source->offset, offset, buf, size);
		if (ret)
			return ret;
		if (actual_size)
			*actual_size = size;
		return 0;
	}
	case DM_VERITY_SOURCE_MEDIA_NOR:
		printf("dmverity: TODO: NOR partition source is not implemented\n");
		return -ENOSYS;
	case DM_VERITY_SOURCE_MEDIA_NAND:
		printf("dmverity: TODO: NAND partition source is not implemented\n");
		return -ENOSYS;
	default:
		return -EINVAL;
	}
}

int dm_verity_read_data_source_region(
	const struct dm_verity_data_source_desc *source,
	const char *part_name, unsigned int offset,
	void *buf, unsigned int size)
{
	struct mmc *mmc = NULL;
	disk_partition_t part_info;
	int ret;

	if (!source || !part_name || !buf || !size)
		return -EINVAL;

	switch (source->media) {
	case DM_VERITY_SOURCE_MEDIA_MMC:
		ret = dm_verity_get_mmc_partition_info_by_name(
			source->location.mmc.dev,
			source->location.mmc.max_part,
			part_name, &mmc, &part_info);
		if (ret)
			return ret;
		return dm_verity_read_mmc_partition_resolved(
			source->location.mmc.dev,
			mmc, &part_info, 0, offset, buf, size);
	case DM_VERITY_SOURCE_MEDIA_NOR:
		printf("dmverity: TODO: NOR data source is not implemented\n");
		return -ENOSYS;
	case DM_VERITY_SOURCE_MEDIA_NAND:
		printf("dmverity: TODO: NAND data source is not implemented\n");
		return -ENOSYS;
	default:
		return -EINVAL;
	}
}
