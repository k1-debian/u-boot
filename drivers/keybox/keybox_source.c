#include <common.h>
#include <asm/errno.h>
#include "keybox_source_internal.h"

static int keybox_source_backend_read(void *priv, unsigned int offset, void *buf,
				      unsigned int size,
				      unsigned int *actual_size)
{
	const struct keybox_source_runtime *runtime = priv;

	if (!runtime || !buf || !size)
		return -EINVAL;

	switch (runtime->source.media) {
	case KEYBOX_SOURCE_MEDIA_MMC:
		return keybox_source_mmc_read(&runtime->source, offset, buf, size,
					      actual_size);
	case KEYBOX_SOURCE_MEDIA_NOR:
		return keybox_source_nor_read(&runtime->source, offset, buf, size,
					      actual_size);
	case KEYBOX_SOURCE_MEDIA_NAND:
		return keybox_source_nand_read(&runtime->source, offset, buf, size,
					       actual_size);
	default:
		return -EINVAL;
	}
}

static int keybox_source_backend_get_size(void *priv, unsigned int *size)
{
	const struct keybox_source_runtime *runtime = priv;

	if (!runtime || !size)
		return -EINVAL;

	if (runtime->source.length) {
		*size = runtime->source.length;
		return 0;
	}

	switch (runtime->source.media) {
	case KEYBOX_SOURCE_MEDIA_MMC:
		return keybox_source_mmc_get_size(&runtime->source, size);
	case KEYBOX_SOURCE_MEDIA_NOR:
		return keybox_source_nor_get_size(&runtime->source, size);
	case KEYBOX_SOURCE_MEDIA_NAND:
		return keybox_source_nand_get_size(&runtime->source, size);
	default:
		return -EINVAL;
	}
}

const struct keybox_backend_ops keybox_source_backend_ops = {
	.read = keybox_source_backend_read,
	.get_size = keybox_source_backend_get_size,
};
