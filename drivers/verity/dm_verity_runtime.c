#include <common.h>
#include <asm/errno.h>
#include <dm_verity_runtime.h>
#include <malloc.h>
#include <sha256.h>

#define DM_VERITY_MANIFEST_MAGIC	0x56525459U
#define DM_VERITY_MANIFEST_VERSION	1U
#define DM_VERITY_MANIFEST_RESERVED_LEN	33
#define DM_VERITY_TREE_READ_CHUNK	65536U

struct dm_verity_manifest_storage_v1 {
	unsigned int magic;
	unsigned short version;
	unsigned short header_size;
	unsigned int flags;
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
	char reserved[DM_VERITY_MANIFEST_RESERVED_LEN];
} __attribute__((packed));

static int dm_verity_strnlen_local(const char *str, unsigned int max_len)
{
	unsigned int i;

	for (i = 0; i < max_len; i++) {
		if (!str[i])
			return i;
	}

	return max_len;
}

static int dm_verity_copy_field(char *dst, unsigned int dst_size,
			const char *src, unsigned int src_size)
{
	unsigned int len;

	if (!dst || !src || !dst_size)
		return -EINVAL;

	len = dm_verity_strnlen_local(src, src_size);
	if (len >= dst_size)
		return -ENOSPC;

	memcpy(dst, src, len);
	dst[len] = '\0';
	return 0;
}

static void dm_verity_digest_to_hex(const unsigned char *digest,
			    unsigned int digest_len,
			    char *hex, unsigned int hex_size)
{
	static const char lut[] = "0123456789abcdef";
	unsigned int i;

	if (!digest || !hex || hex_size < (digest_len * 2 + 1))
		return;

	for (i = 0; i < digest_len; i++) {
		hex[i * 2] = lut[(digest[i] >> 4) & 0xf];
		hex[i * 2 + 1] = lut[digest[i] & 0xf];
	}
	hex[digest_len * 2] = '\0';
}

static int dm_verity_parse_manifest_storage(
	const struct dm_verity_manifest_storage_v1 *storage,
	struct dm_verity_ctx *ctx)
{
	int ret;

	if (!storage || !ctx)
		return -EINVAL;

	if (storage->magic != DM_VERITY_MANIFEST_MAGIC)
		return -EINVAL;
	if (storage->version != DM_VERITY_MANIFEST_VERSION)
		return -EPROTONOSUPPORT;
	if (storage->header_size != sizeof(*storage))
		return -EINVAL;

	memset(ctx, 0, sizeof(*ctx));
	ctx->manifest.version = storage->version;
	ctx->manifest.key_id = storage->key_id;
	ctx->manifest.rollback_version = storage->rollback_version;
	ctx->manifest.data_block_size = storage->data_block_size;
	ctx->manifest.hash_block_size = storage->hash_block_size;
	ctx->manifest.data_blocks = storage->data_blocks;
	ctx->manifest.hash_start_block = storage->hash_start_block;
	ctx->manifest.data_sectors = storage->data_sectors;
	ctx->manifest.tree_offset = storage->tree_offset;
	ctx->manifest.tree_size = storage->tree_size;

	ret = dm_verity_copy_field(ctx->manifest.part_name,
				   sizeof(ctx->manifest.part_name),
				   storage->part_name,
				   sizeof(storage->part_name));
	if (ret)
		return ret;
	ret = dm_verity_copy_field(ctx->manifest.hash_name,
				   sizeof(ctx->manifest.hash_name),
				   storage->hash_name,
				   sizeof(storage->hash_name));
	if (ret)
		return ret;
	ret = dm_verity_copy_field(ctx->manifest.root_hash,
				   sizeof(ctx->manifest.root_hash),
				   storage->root_hash,
				   sizeof(storage->root_hash));
	if (ret)
		return ret;
	ret = dm_verity_copy_field(ctx->manifest.salt,
				   sizeof(ctx->manifest.salt),
				   storage->salt,
				   sizeof(storage->salt));
	if (ret)
		return ret;
	ret = dm_verity_copy_field(ctx->manifest.tree_sha256,
				   sizeof(ctx->manifest.tree_sha256),
				   storage->tree_sha256,
				   sizeof(storage->tree_sha256));
	if (ret)
		return ret;

	if (!ctx->manifest.part_name[0] ||
	    !ctx->manifest.hash_name[0] ||
	    !ctx->manifest.root_hash[0] ||
	    !ctx->manifest.salt[0] ||
	    !ctx->manifest.tree_sha256[0] ||
	    !ctx->manifest.data_block_size ||
	    !ctx->manifest.hash_block_size ||
	    !ctx->manifest.data_blocks ||
	    !ctx->manifest.hash_start_block ||
	    !ctx->manifest.data_sectors ||
	    !ctx->manifest.tree_offset ||
	    !ctx->manifest.tree_size)
		return -EINVAL;

	return 0;
}

static int dm_verity_sha256_region_software(
	const struct dm_verity_data_ops *ops, void *priv,
	const char *part_name, unsigned int offset, unsigned int size,
	unsigned char digest[SHA256_SUM_LEN])
{
	sha256_context ctx;
	unsigned char *chunk;
	unsigned int done = 0;
	unsigned int this_size;
	int ret;

	if (!ops || !ops->read_region || !part_name || !digest || !size)
		return -EINVAL;

	chunk = malloc(DM_VERITY_TREE_READ_CHUNK);
	if (!chunk)
		return -ENOMEM;

	sha256_starts(&ctx);
	while (done < size) {
		this_size = size - done;
		if (this_size > DM_VERITY_TREE_READ_CHUNK)
			this_size = DM_VERITY_TREE_READ_CHUNK;

		ret = ops->read_region(priv, part_name, offset + done, chunk,
				      this_size);
		if (ret)
			goto out;

		sha256_update(&ctx, chunk, this_size);
		done += this_size;
	}

	sha256_finish(&ctx, digest);
	ret = 0;
out:
	free(chunk);
	return ret;
}

int dm_verity_load(struct dm_verity_ctx *ctx,
	   const struct dm_verity_manifest_ops *ops,
	   void *priv)
{
	struct dm_verity_manifest_storage_v1 storage;
	unsigned int actual_size = 0;
	int ret;

	if (!ctx || !ops || !ops->read_manifest || !ops->read_manifest_sig)
		return -EINVAL;

	memset(&storage, 0, sizeof(storage));
	ret = ops->read_manifest(priv, &storage, sizeof(storage), &actual_size);
	if (ret)
		return ret;
	if (actual_size != sizeof(storage))
		return -EMSGSIZE;

	ret = dm_verity_parse_manifest_storage(&storage, ctx);
	if (ret)
		return ret;

	memcpy(ctx->manifest_blob, &storage, sizeof(storage));
	ctx->manifest_len = sizeof(storage);

	ret = ops->read_manifest_sig(priv, ctx->manifest_sig,
			     sizeof(ctx->manifest_sig),
			     &ctx->manifest_sig_len);
	if (ret)
		return ret;
	if (!ctx->manifest_sig_len)
		return -ENODATA;

	ctx->loaded = 1;
	return 0;
}

int dm_verity_verify_manifest_sig(struct dm_verity_ctx *ctx,
			 const struct security_pubkey *pubkey)
{
	unsigned char digest[SHA256_SUM_LEN];
	sha256_context sha_ctx;
	int ret;

	if (!ctx || !ctx->loaded || !pubkey)
		return -EINVAL;
	if (!ctx->manifest_len || !ctx->manifest_sig_len)
		return -EINVAL;

	if (ctx->manifest.key_id && ctx->manifest.key_id != pubkey->key_id)
		return -EACCES;

	sha256_starts(&sha_ctx);
	sha256_update(&sha_ctx, ctx->manifest_blob, ctx->manifest_len);
	sha256_finish(&sha_ctx, digest);

	ret = security_pubkey_verify_rsa2048_sha256(pubkey,
					    ctx->manifest_sig,
					    ctx->manifest_sig_len,
					    digest,
					    sizeof(digest));
	if (ret)
		return ret;

	ctx->manifest_verified = 1;
	return 0;
}

int dm_verity_verify_tree(struct dm_verity_ctx *ctx,
		  const struct dm_verity_data_ops *ops,
		  void *priv)
{
	unsigned char digest[SHA256_SUM_LEN];
	char digest_hex[DM_VERITY_HEX_BUF_LEN];
	int ret;

	if (!ctx || !ctx->loaded || !ops)
		return -EINVAL;

	if (ops->hash_region) {
		ret = ops->hash_region(priv, ctx->manifest.part_name,
				      ctx->manifest.tree_offset,
				      ctx->manifest.tree_size,
				      digest, sizeof(digest));
		if (!ret)
			goto compare_digest;
		if (ret != -ENOSYS)
			return ret;
	}

	ret = dm_verity_sha256_region_software(ops, priv,
					      ctx->manifest.part_name,
					      ctx->manifest.tree_offset,
					      ctx->manifest.tree_size,
					      digest);
	if (ret)
		return ret;

compare_digest:
	dm_verity_digest_to_hex(digest, sizeof(digest), digest_hex,
			       sizeof(digest_hex));
	if (strcmp(digest_hex, ctx->manifest.tree_sha256))
		return -EACCES;

	ctx->tree_verified = 1;
	return 0;
}

int dm_verity_export_params(const struct dm_verity_ctx *ctx,
		    struct dm_verity_params *out)
{
	if (!ctx || !ctx->loaded || !out)
		return -EINVAL;

	memset(out, 0, sizeof(*out));
	out->key_id = ctx->manifest.key_id;
	out->data_block_size = ctx->manifest.data_block_size;
	out->hash_block_size = ctx->manifest.hash_block_size;
	out->data_blocks = ctx->manifest.data_blocks;
	out->hash_start_block = ctx->manifest.hash_start_block;
	out->data_sectors = ctx->manifest.data_sectors;
	out->tree_offset = ctx->manifest.tree_offset;
	out->tree_size = ctx->manifest.tree_size;
	memcpy(out->part_name, ctx->manifest.part_name, sizeof(out->part_name));
	memcpy(out->hash_name, ctx->manifest.hash_name, sizeof(out->hash_name));
	memcpy(out->root_hash, ctx->manifest.root_hash, sizeof(out->root_hash));
	memcpy(out->salt, ctx->manifest.salt, sizeof(out->salt));
	memcpy(out->tree_sha256, ctx->manifest.tree_sha256,
	       sizeof(out->tree_sha256));
	return 0;
}

int dm_verity_runtime_run(const struct dm_verity_runtime_req *req,
			 struct dm_verity_runtime_result *out)
{
	struct dm_verity_ctx runtime;
	int ret;

	if (!req || !out || !req->pubkey || !req->manifest_ops || !req->data_ops)
		return -EINVAL;

	memset(&runtime, 0, sizeof(runtime));
	memset(out, 0, sizeof(*out));

	ret = dm_verity_load(&runtime, req->manifest_ops, req->manifest_priv);
	if (ret)
		return ret;

	ret = dm_verity_verify_manifest_sig(&runtime, req->pubkey);
	if (ret)
		return ret;

	ret = dm_verity_verify_tree(&runtime, req->data_ops, req->data_priv);
	if (ret)
		return ret;

	ret = dm_verity_export_params(&runtime, &out->params);
	if (ret)
		return ret;

	out->rollback_version = runtime.manifest.rollback_version;
	return 0;
}
