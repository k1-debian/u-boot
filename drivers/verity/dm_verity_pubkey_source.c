#include <common.h>
#include <asm/errno.h>
#include <dm_verity_boot.h>
#include <keybox.h>

#include "dm_verity_pubkey_source.h"

static int dm_verity_pubkey_map_keybox_media(unsigned int media,
					     unsigned int *out_media)
{
	if (!out_media)
		return -EINVAL;

	switch (media) {
	case DM_VERITY_SOURCE_MEDIA_MMC:
		*out_media = KEYBOX_SOURCE_MEDIA_MMC;
		return 0;
	case DM_VERITY_SOURCE_MEDIA_NOR:
		*out_media = KEYBOX_SOURCE_MEDIA_NOR;
		return 0;
	case DM_VERITY_SOURCE_MEDIA_NAND:
		*out_media = KEYBOX_SOURCE_MEDIA_NAND;
		return 0;
	default:
		return -EINVAL;
	}
}

static int dm_verity_pubkey_build_keybox_source(
	const struct dm_verity_partition_source_desc *partition,
	struct keybox_source_desc *out)
{
	int ret;

	if (!partition || !out)
		return -EINVAL;

	memset(out, 0, sizeof(*out));

	ret = dm_verity_pubkey_map_keybox_media(partition->media, &out->media);
	if (ret)
		return ret;

	out->offset = partition->offset;
	out->length = partition->length;

	if (partition->media == DM_VERITY_SOURCE_MEDIA_MMC) {
		out->location.mmc.dev = partition->location.mmc.dev;
		out->location.mmc.max_part = partition->location.mmc.max_part;
		out->location.mmc.part_name = partition->location.mmc.part_name;
	}

	return 0;
}

static void dm_verity_pubkey_build_keybox_selector(
	const struct dm_verity_pubkey_selector *selector,
	struct keybox_selector *out)
{
	memset(out, 0, sizeof(*out));

	if (!selector)
		return;

	out->name = selector->name;
	out->class_id = selector->class_id;
	out->usage = selector->usage;
	out->key_id = selector->key_id;
	out->algo = selector->algo;
}

static int dm_verity_load_keybox_pubkey(
	const struct dm_verity_pubkey_source_desc *source,
	struct security_pubkey *out)
{
	struct keybox_ctx ctx;
	struct keybox_source_desc keybox_source;
	struct keybox_selector selector;
	struct security_pubkey loaded;
	int ret;

	memset(&ctx, 0, sizeof(ctx));
	memset(&keybox_source, 0, sizeof(keybox_source));
	memset(&selector, 0, sizeof(selector));
	memset(&loaded, 0, sizeof(loaded));

	ret = dm_verity_pubkey_build_keybox_source(&source->partition,
						   &keybox_source);
	if (ret)
		return ret;

	dm_verity_pubkey_build_keybox_selector(&source->selector, &selector);

	ret = keybox_load_pubkey_from_source(&ctx, &keybox_source,
					     &selector, &loaded);
	if (!ret)
		ret = security_pubkey_dup(&loaded, out);
	keybox_close(&ctx);
	return ret;
}

int dm_verity_load_pubkey_from_source(
	const struct dm_verity_pubkey_source_desc *source,
	struct security_pubkey *out)
{
	if (!source || !out)
		return -EINVAL;

	memset(out, 0, sizeof(*out));

	switch (source->type) {
	case DM_VERITY_PUBKEY_SOURCE_KEYBOX:
		return dm_verity_load_keybox_pubkey(source, out);
	case DM_VERITY_PUBKEY_SOURCE_DIRECT:
		if (!source->direct.data || !source->direct.data_len)
			return -EINVAL;
		memcpy(out, &source->direct, sizeof(*out));
		return 0;
	default:
		return -EINVAL;
	}
}
