#include <common.h>
#include <asm/errno.h>
#include "keybox_source_internal.h"

int keybox_source_nor_read(const struct keybox_source_desc *source,
			   unsigned int offset, void *buf,
			   unsigned int size, unsigned int *actual_size)
{
	(void)source;
	(void)offset;
	(void)buf;
	(void)size;
	(void)actual_size;
	printf("keybox: TODO: NOR source is not implemented\n");
	return -ENOSYS;
}

int keybox_source_nor_get_size(const struct keybox_source_desc *source,
			       unsigned int *size)
{
	(void)source;
	(void)size;
	printf("keybox: TODO: NOR source size is not implemented\n");
	return -ENOSYS;
}
