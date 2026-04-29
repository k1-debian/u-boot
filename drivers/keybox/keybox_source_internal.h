#ifndef __KEYBOX_SOURCE_INTERNAL_H__
#define __KEYBOX_SOURCE_INTERNAL_H__

#include <keybox.h>

struct keybox_source_runtime {
	struct keybox_source_desc source;
};

extern const struct keybox_backend_ops keybox_source_backend_ops;

int keybox_source_mmc_read(const struct keybox_source_desc *source,
			   unsigned int offset, void *buf,
			   unsigned int size, unsigned int *actual_size);
int keybox_source_mmc_get_size(const struct keybox_source_desc *source,
			       unsigned int *size);
int keybox_source_nor_read(const struct keybox_source_desc *source,
			   unsigned int offset, void *buf,
			   unsigned int size, unsigned int *actual_size);
int keybox_source_nor_get_size(const struct keybox_source_desc *source,
			       unsigned int *size);
int keybox_source_nand_read(const struct keybox_source_desc *source,
			    unsigned int offset, void *buf,
			    unsigned int size, unsigned int *actual_size);
int keybox_source_nand_get_size(const struct keybox_source_desc *source,
				unsigned int *size);

#endif
