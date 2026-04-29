#include <common.h>
#include <asm/errno.h>
#include "keybox_source_internal.h"

#ifdef CONFIG_MMC
#include <mmc.h>
#include <part.h>

static int keybox_get_mmc_partition_info_by_name(int mmc_dev, int max_part,
						 const char *part_name,
						 struct mmc **out_mmc,
						 disk_partition_t *out_part)
{
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
}

static int keybox_read_mmc_partition_resolved(int mmc_dev, struct mmc *mmc,
					      const disk_partition_t *part_info,
					      unsigned int base_offset,
					      unsigned int offset, void *buf,
					      unsigned int size)
{
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
}

int keybox_source_mmc_read(const struct keybox_source_desc *source,
			   unsigned int offset, void *buf,
			   unsigned int size, unsigned int *actual_size)
{
	struct mmc *mmc = NULL;
	disk_partition_t part_info;
	int ret;

	if (!source || !buf || !size)
		return -EINVAL;

	ret = keybox_get_mmc_partition_info_by_name(source->location.mmc.dev,
						    source->location.mmc.max_part,
						    source->location.mmc.part_name,
						    &mmc, &part_info);
	if (ret)
		return ret;

	if (source->length && offset + size > source->length)
		return -EFBIG;

	ret = keybox_read_mmc_partition_resolved(source->location.mmc.dev, mmc,
						 &part_info, source->offset,
						 offset, buf, size);
	if (ret)
		return ret;

	if (actual_size)
		*actual_size = size;

	return 0;
}

int keybox_source_mmc_get_size(const struct keybox_source_desc *source,
			       unsigned int *size)
{
	struct mmc *mmc = NULL;
	disk_partition_t part_info;
	unsigned long long part_bytes;
	int ret;

	if (!source || !size)
		return -EINVAL;

	ret = keybox_get_mmc_partition_info_by_name(source->location.mmc.dev,
						    source->location.mmc.max_part,
						    source->location.mmc.part_name,
						    &mmc, &part_info);
	if (ret)
		return ret;

	part_bytes = (unsigned long long)part_info.size * part_info.blksz;
	if (part_bytes <= source->offset || part_bytes > 0xffffffffULL)
		return -EFBIG;

	*size = (unsigned int)(part_bytes - source->offset);
	return 0;
}
#else
int keybox_source_mmc_read(const struct keybox_source_desc *source,
			   unsigned int offset, void *buf,
			   unsigned int size, unsigned int *actual_size)
{
	(void)source;
	(void)offset;
	(void)buf;
	(void)size;
	(void)actual_size;
	printf("keybox: MMC source requires CONFIG_MMC\n");
	return -ENOSYS;
}

int keybox_source_mmc_get_size(const struct keybox_source_desc *source,
			       unsigned int *size)
{
	(void)source;
	(void)size;
	printf("keybox: MMC source requires CONFIG_MMC\n");
	return -ENOSYS;
}
#endif
