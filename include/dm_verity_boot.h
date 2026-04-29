#ifndef __DM_VERITY_BOOT_H__
#define __DM_VERITY_BOOT_H__

#include <dm_verity_runtime.h>

#define DM_VERITY_BOOTARGS_MAX_LEN	1024U

#define DM_VERITY_PUBKEY_SOURCE_NONE	0U
#define DM_VERITY_PUBKEY_SOURCE_KEYBOX	1U
#define DM_VERITY_PUBKEY_SOURCE_DIRECT	2U

#define DM_VERITY_PUBKEY_CLASS_UNKNOWN	0U
#define DM_VERITY_PUBKEY_CLASS_PUBKEY	1U
#define DM_VERITY_PUBKEY_CLASS_SECRET	2U
#define DM_VERITY_PUBKEY_CLASS_CERT	3U
#define DM_VERITY_PUBKEY_CLASS_BLOB	4U

#define DM_VERITY_PUBKEY_USAGE_GENERIC	0U
#define DM_VERITY_PUBKEY_USAGE_DM_VERITY	1U
#define DM_VERITY_PUBKEY_USAGE_LUKS_CONFIG	2U
#define DM_VERITY_PUBKEY_USAGE_LUKS_PART_KEY	3U
#define DM_VERITY_PUBKEY_USAGE_LUKS_LOG_KEY	4U
#define DM_VERITY_PUBKEY_USAGE_UPDATE_VERIFY	5U

#define DM_VERITY_SOURCE_MEDIA_NONE	0U
#define DM_VERITY_SOURCE_MEDIA_MMC	1U
#define DM_VERITY_SOURCE_MEDIA_NOR	2U
#define DM_VERITY_SOURCE_MEDIA_NAND	3U

#define DM_VERITY_MANIFEST_SOURCE_NONE	0U
#define DM_VERITY_MANIFEST_SOURCE_MMC_RAW	1U
#define DM_VERITY_MANIFEST_SOURCE_ROOTFS_TAIL	2U

struct dm_verity_partition_source_desc {
	unsigned int media;
	unsigned int offset;
	unsigned int length;
	union {
		struct {
			int dev;
			int max_part;
			const char *part_name;
		} mmc;
	} location;
};

struct dm_verity_pubkey_selector {
	const char *name;
	unsigned int class_id;
	unsigned int usage;
	unsigned int key_id;
	unsigned int algo;
};

struct dm_verity_pubkey_source_desc {
	unsigned int type;
	struct dm_verity_partition_source_desc partition;
	struct dm_verity_pubkey_selector selector;
	struct security_pubkey direct;
};

struct dm_verity_manifest_source_desc {
	unsigned int type;
	struct dm_verity_partition_source_desc partition;
	unsigned int sig_offset;
	unsigned int sig_length;
};

struct dm_verity_data_source_desc {
	unsigned int media;
	union {
		struct {
			int dev;
			int max_part;
		} mmc;
	} location;
};

struct dm_verity_boot_config {
	struct dm_verity_pubkey_source_desc pubkey_source;
	struct dm_verity_manifest_source_desc manifest_source;
	struct dm_verity_data_source_desc data_source;
	unsigned int data_dev_major;
	unsigned int data_dev_minor;
	unsigned int hash_dev_major;
	unsigned int hash_dev_minor;
	const char *bootargs_base;
	const char *kernel_bootcmd;
};

struct dm_verity_boot_state {
	struct dm_verity_runtime_result runtime;
	char bootargs[DM_VERITY_BOOTARGS_MAX_LEN];
};

int board_dm_verity_get_config(struct dm_verity_boot_config *out);
int dm_verity_boot_prepare(const struct dm_verity_boot_config *cfg,
	   struct dm_verity_boot_state *out);
int dm_verity_boot_run(const struct dm_verity_boot_config *cfg);

#endif
