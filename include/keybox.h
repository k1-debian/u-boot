#ifndef __KEYBOX_H__
#define __KEYBOX_H__

#include <security_pubkey.h>

#define KEYBOX_MAX_OBJECTS		32U
#define KEYBOX_NAME_MAX_LEN		64U

#define KEYBOX_OBJECT_CLASS_UNKNOWN	0U
#define KEYBOX_OBJECT_CLASS_PUBKEY	1U
#define KEYBOX_OBJECT_CLASS_SECRET	2U
#define KEYBOX_OBJECT_CLASS_CERT	3U
#define KEYBOX_OBJECT_CLASS_BLOB	4U

#define KEYBOX_OBJECT_USAGE_GENERIC	0U
#define KEYBOX_OBJECT_USAGE_DM_VERITY	1U
#define KEYBOX_OBJECT_USAGE_LUKS_CONFIG	2U
#define KEYBOX_OBJECT_USAGE_LUKS_PART_KEY	3U
#define KEYBOX_OBJECT_USAGE_LUKS_LOG_KEY	4U
#define KEYBOX_OBJECT_USAGE_UPDATE_VERIFY	5U

#define KEYBOX_OBJECT_FLAG_NONE		0U
#define KEYBOX_OBJECT_FLAG_ENCRYPTED	0x00000001U
#define KEYBOX_OBJECT_FLAG_TRUST_CHECK_REQUIRED	0x00000002U
#define KEYBOX_OBJECT_FLAG_WRITE_PROTECTED	0x00000004U

#define KEYBOX_SOURCE_MEDIA_NONE	0U
#define KEYBOX_SOURCE_MEDIA_MMC	1U
#define KEYBOX_SOURCE_MEDIA_NOR	2U
#define KEYBOX_SOURCE_MEDIA_NAND	3U

struct keybox_selector {
	const char *name;
	unsigned int class_id;
	unsigned int usage;
	unsigned int key_id;
	unsigned int algo;
};

struct keybox_object {
	char name[KEYBOX_NAME_MAX_LEN];
	unsigned int class_id;
	unsigned int usage;
	unsigned int key_id;
	unsigned int algo;
	unsigned int flags;
	void *data;
	unsigned int data_len;
};

struct keybox_backend_ops {
	int (*read)(void *priv, unsigned int offset, void *buf,
		    unsigned int size, unsigned int *actual_size);
	int (*get_size)(void *priv, unsigned int *size);
};

struct keybox_secure_ops {
	int (*unwrap)(void *priv, const struct keybox_backend_ops *backend_ops,
		      const void *header, unsigned int header_size,
		      void **out_buf, unsigned int *out_size);
	int (*verify_container)(void *priv, const void *container,
			       unsigned int size);
	int (*decrypt_object)(void *priv, struct keybox_object *obj);
	int (*check_object_trust)(void *priv, const struct keybox_object *obj);
};

struct keybox_source_desc {
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

struct keybox_ctx {
	const struct keybox_backend_ops *backend_ops;
	const struct keybox_secure_ops *secure_ops;
	void *priv;
	unsigned char *container;
	unsigned int container_size;
	struct keybox_object objects[KEYBOX_MAX_OBJECTS];
	unsigned int object_count;
	unsigned int decrypted_mask;
	unsigned int trust_checked_mask;
	int owns_priv;
	int opened;
	int verified;
};

int keybox_open(struct keybox_ctx *ctx,
	       const struct keybox_backend_ops *backend_ops,
	       const struct keybox_secure_ops *secure_ops,
	       void *priv);
int keybox_open_source(struct keybox_ctx *ctx,
	       const struct keybox_source_desc *source);
void keybox_close(struct keybox_ctx *ctx);
int keybox_verify(struct keybox_ctx *ctx);
int keybox_find_object(struct keybox_ctx *ctx,
	      const struct keybox_selector *selector,
	      struct keybox_object *out);
int keybox_find_object_by_name(struct keybox_ctx *ctx, const char *name,
		       struct keybox_object *out);
int keybox_object_to_pubkey(const struct keybox_object *obj,
		    struct security_pubkey *out);
int keybox_load_pubkey_from_source(struct keybox_ctx *ctx,
	      const struct keybox_source_desc *source,
	      const struct keybox_selector *selector,
	      struct security_pubkey *out);

#endif
