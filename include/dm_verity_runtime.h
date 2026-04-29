#ifndef __DM_VERITY_RUNTIME_H__
#define __DM_VERITY_RUNTIME_H__

#include <security_pubkey.h>

#define DM_VERITY_PART_NAME_LEN		32U
#define DM_VERITY_HASH_NAME_LEN		16U
#define DM_VERITY_HEX_DIGEST_LEN	64U
#define DM_VERITY_HEX_BUF_LEN		(DM_VERITY_HEX_DIGEST_LEN + 1)
#define DM_VERITY_MANIFEST_BLOB_MAX_LEN	384U
#define DM_VERITY_MANIFEST_SIG_MAX_LEN	512U

struct dm_verity_manifest {
	unsigned int version;
	unsigned int key_id;
	unsigned int rollback_version;
	unsigned int data_block_size;
	unsigned int hash_block_size;
	unsigned int data_blocks;
	unsigned int hash_start_block;
	unsigned int data_sectors;
	unsigned int tree_offset;
	unsigned int tree_size;
	char part_name[DM_VERITY_PART_NAME_LEN];
	char hash_name[DM_VERITY_HASH_NAME_LEN];
	char root_hash[DM_VERITY_HEX_BUF_LEN];
	char salt[DM_VERITY_HEX_BUF_LEN];
	char tree_sha256[DM_VERITY_HEX_BUF_LEN];
};

struct dm_verity_params {
	unsigned int key_id;
	unsigned int data_block_size;
	unsigned int hash_block_size;
	unsigned int data_blocks;
	unsigned int hash_start_block;
	unsigned int data_sectors;
	unsigned int tree_offset;
	unsigned int tree_size;
	char part_name[DM_VERITY_PART_NAME_LEN];
	char hash_name[DM_VERITY_HASH_NAME_LEN];
	char root_hash[DM_VERITY_HEX_BUF_LEN];
	char salt[DM_VERITY_HEX_BUF_LEN];
	char tree_sha256[DM_VERITY_HEX_BUF_LEN];
};

struct dm_verity_manifest_ops {
	int (*read_manifest)(void *priv, void *buf, unsigned int size,
			    unsigned int *actual_size);
	int (*read_manifest_sig)(void *priv, void *buf, unsigned int size,
			    unsigned int *actual_size);
};

struct dm_verity_data_ops {
	int (*read_region)(void *priv, const char *part_name,
		   unsigned int offset, void *buf, unsigned int size);
	int (*hash_region)(void *priv, const char *part_name,
		   unsigned int offset, unsigned int size,
		   unsigned char *digest, unsigned int digest_len);
};

struct dm_verity_ctx {
	struct dm_verity_manifest manifest;
	unsigned char manifest_blob[DM_VERITY_MANIFEST_BLOB_MAX_LEN];
	unsigned int manifest_len;
	unsigned char manifest_sig[DM_VERITY_MANIFEST_SIG_MAX_LEN];
	unsigned int manifest_sig_len;
	int loaded;
	int manifest_verified;
	int tree_verified;
};

struct dm_verity_runtime_req {
	const struct security_pubkey *pubkey;
	const struct dm_verity_manifest_ops *manifest_ops;
	void *manifest_priv;
	const struct dm_verity_data_ops *data_ops;
	void *data_priv;
};

struct dm_verity_runtime_result {
	struct dm_verity_params params;
	unsigned int rollback_version;
};

int dm_verity_load(struct dm_verity_ctx *ctx,
	   const struct dm_verity_manifest_ops *ops,
	   void *priv);
int dm_verity_verify_manifest_sig(struct dm_verity_ctx *ctx,
			 const struct security_pubkey *pubkey);
int dm_verity_verify_tree(struct dm_verity_ctx *ctx,
		  const struct dm_verity_data_ops *ops,
		  void *priv);
int dm_verity_export_params(const struct dm_verity_ctx *ctx,
		    struct dm_verity_params *out);
int dm_verity_runtime_run(const struct dm_verity_runtime_req *req,
			 struct dm_verity_runtime_result *out);

#endif
