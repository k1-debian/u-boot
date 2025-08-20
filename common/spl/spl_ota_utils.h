#ifndef __SPL_OTA_UTILS_H__
#define __SPL_OTA_UTILS_H__
#include <common.h>

struct ota_ops {
	void (*flash_init)(void);
	int (*flash_read)(unsigned int addr, unsigned int len, unsigned int buf);
	struct jz_sfcnand_partition_param *(*flash_get_partitions)(void);
	unsigned int (*flash_get_part_offset_by_name)(struct jz_sfcnand_partition_param *partitions, char *name);
	void (*flash_load_kernel)(long offset, char *name);
};

#endif /* __SPL_OTA_UTILS_H__ */
