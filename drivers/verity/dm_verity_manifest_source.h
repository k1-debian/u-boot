#ifndef __DM_VERITY_MANIFEST_SOURCE_H__
#define __DM_VERITY_MANIFEST_SOURCE_H__

#include <dm_verity_boot.h>
#include <dm_verity_runtime.h>

struct dm_verity_manifest_runtime {
	struct dm_verity_manifest_source_desc source;
};

void dm_verity_manifest_runtime_init(
	struct dm_verity_manifest_runtime *runtime,
	const struct dm_verity_manifest_source_desc *source);

const struct dm_verity_manifest_ops *dm_verity_get_manifest_ops(void);

#endif
