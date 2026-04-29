#ifndef __DM_VERITY_MEDIA_H__
#define __DM_VERITY_MEDIA_H__

#include <dm_verity_boot.h>
#include <mmc.h>
#include <part.h>

int dm_verity_get_mmc_partition_info_by_name(int mmc_dev, int max_part,
					     const char *part_name,
					     struct mmc **out_mmc,
					     disk_partition_t *out_part);

int dm_verity_read_mmc_partition_resolved(int mmc_dev, struct mmc *mmc,
					  const disk_partition_t *part_info,
					  unsigned int base_offset,
					  unsigned int offset,
					  void *buf, unsigned int size);

int dm_verity_read_partition_source(
	const struct dm_verity_partition_source_desc *source,
	unsigned int offset, void *buf, unsigned int size,
	unsigned int *actual_size);

int dm_verity_read_data_source_region(
	const struct dm_verity_data_source_desc *source,
	const char *part_name, unsigned int offset,
	void *buf, unsigned int size);

#endif
