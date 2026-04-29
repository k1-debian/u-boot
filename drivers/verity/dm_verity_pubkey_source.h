#ifndef __DM_VERITY_PUBKEY_SOURCE_H__
#define __DM_VERITY_PUBKEY_SOURCE_H__

#include <dm_verity_boot.h>

int dm_verity_load_pubkey_from_source(
	const struct dm_verity_pubkey_source_desc *source,
	struct security_pubkey *out);

#endif
