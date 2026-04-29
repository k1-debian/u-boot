#include <common.h>
#include <config.h>
#include <asm/errno.h>
#include <keybox.h>
#include <malloc.h>
#include <mmc.h>
#include <u-boot/crc.h>
#include "keybox_source_internal.h"

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT)
#include <asm/arch/cpm.h>
#include <asm/io.h>
#include "../scboot/jz_sec_v4/secall.h"
#endif

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_CKEYAES)
extern void flush_cache_all(void);
#endif

#ifndef CONFIG_KEYBOX_OUTER_SCBOOT_ENABLE
#define CONFIG_KEYBOX_OUTER_SCBOOT_ENABLE	0
#endif
#ifndef CONFIG_KEYBOX_AES_USERKEY0_ENABLE
#define CONFIG_KEYBOX_AES_USERKEY0_ENABLE	0
#endif
#ifndef CONFIG_KEYBOX_NKU_CHECK_ENABLE
#define CONFIG_KEYBOX_NKU_CHECK_ENABLE	0
#endif

#define KEYBOX_MAGIC			0x4b425831U
#define KEYBOX_VERSION			1U
#define KEYBOX_FLAG_UNSIGNED_TEST	0x00000001U
#define KEYBOX_SECSTORE_MAGIC		0x17253948U
#define KEYBOX_SECSTORE_REENCRYPT_MAGIC	0x86734716U
#define KEYBOX_SECSTORE_WRITE_PROTECT_MAGIC	0x8ad3820fU
#define KEYBOX_ALGO_RSA2048_SHA256_PKCS1	1U
#define KEYBOX_SECSTORE_BLOCK_SIZE	4096U
#define KEYBOX_SECSTORE_MAX_ITEM	32U
#define KEYBOX_CONTAINER_MAX_LEN	(KEYBOX_SECSTORE_BLOCK_SIZE * KEYBOX_SECSTORE_MAX_ITEM)
#define KEYBOX_RESERVED_LEN	24
#define KEYBOX_ENTRY_RESERVED_LEN	8
#define KEYBOX_MAX_ENTRIES	16U
#define KEYBOX_SECSTORE_NAME_LEN	64U
#define KEYBOX_SECSTORE_MAX_STORE_LEN	0x0c00U
#define KEYBOX_SECSTORE_MAP_DATA_LEN	(KEYBOX_SECSTORE_BLOCK_SIZE - sizeof(unsigned int) * 2)
#define KEYBOX_HEADER_READ_LEN	KEYBOX_SECSTORE_BLOCK_SIZE
#define KEYBOX_SECURE_SCBOOT_MAGIC	0x54424353U
#define KEYBOX_SCBOOT_MAGIC_SIZE	MMC_MAX_BLOCK_LEN
#define KEYBOX_SCBOOT_KEY_SIZE	1536U
#define KEYBOX_SCBOOT_HEADER_SIZE	(KEYBOX_SCBOOT_MAGIC_SIZE + KEYBOX_SCBOOT_KEY_SIZE)
#define KEYBOX_SECSTORE_IMAGE_MAX_LEN	(KEYBOX_SECSTORE_BLOCK_SIZE * KEYBOX_SECSTORE_MAX_ITEM)

struct keybox_storage_v1 {
	unsigned int magic;
	unsigned short version;
	unsigned short header_size;
	unsigned int flags;
	unsigned int container_size;
	unsigned int key_count;
	unsigned int entries_offset;
	unsigned int entries_size;
	unsigned int auth_offset;
	unsigned int auth_size;
	char reserved[KEYBOX_RESERVED_LEN];
} __attribute__((packed));

struct keybox_entry_storage_v1 {
	unsigned int usage;
	unsigned int key_id;
	unsigned int algo;
	unsigned int flags;
	unsigned int data_offset;
	unsigned int data_size;
	char reserved[KEYBOX_ENTRY_RESERVED_LEN];
} __attribute__((packed));

struct keybox_secstore_map_info {
	unsigned char data[KEYBOX_SECSTORE_MAP_DATA_LEN];
	unsigned int magic;
	unsigned int crc;
} __attribute__((packed));

struct keybox_secstore_object {
	unsigned int magic;
	int id;
	unsigned char name[KEYBOX_SECSTORE_NAME_LEN];
	unsigned int re_encrypt;
	unsigned int version;
	unsigned int write_protect;
	unsigned int reserved[3];
	unsigned int actual_len;
	unsigned char data[KEYBOX_SECSTORE_MAX_STORE_LEN];
	unsigned int crc;
} __attribute__((packed));

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_CKEYAES) && \
	CONFIG_KEYBOX_AES_USERKEY0_ENABLE
#define KEYBOX_AES_CRYPT	0x1
#define KEYBOX_AES_BY_UKEY	(0x1 << 6)
int do_aes_dma(void *in_paaddr, void *out_paaddr, int len, int aeskey,
	       int aescrypt);
#endif

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT)
int secure_scboot(void *input, void *output);
extern void pdma_wait(void);
#endif

static int keybox_strnlen_local(const char *str, unsigned int max_len)
{
	unsigned int i;

	for (i = 0; i < max_len; i++) {
		if (!str[i])
			return i;
	}

	return max_len;
}

static int keybox_crc32_ok(const void *buf, unsigned int size,
			   unsigned int stored_crc)
{
	if (!stored_crc)
		return 1;

	return crc32(0, buf, size) == stored_crc;
}

static int keybox_secstore_name_valid(const unsigned char *name)
{
	unsigned int i;

	for (i = 0; i < KEYBOX_SECSTORE_NAME_LEN; i++) {
		if (!name[i])
			return i > 0;
	}

	return 0;
}

static int keybox_secstore_map_valid(const struct keybox_secstore_map_info *map)
{
	if (!map)
		return 0;

	if (map->magic != KEYBOX_SECSTORE_MAGIC)
		return 0;

	return keybox_crc32_ok(map, sizeof(*map) - sizeof(map->crc), map->crc);
}

static int keybox_secstore_object_valid(
	const struct keybox_secstore_object *obj)
{
	if (!obj)
		return 0;

	if (obj->magic != KEYBOX_SECSTORE_MAGIC)
		return 0;

	if (!keybox_secstore_name_valid(obj->name))
		return 0;

	if (!obj->actual_len || obj->actual_len > sizeof(obj->data))
		return 0;

	if (!keybox_crc32_ok(obj, sizeof(*obj) - sizeof(obj->crc), obj->crc))
		return 0;

	return 1;
}

static unsigned int keybox_detect_algo(const void *data, unsigned int data_len)
{
	unsigned int magic;
	const char *text = data;

	if (!data || !data_len)
		return SECURITY_PUBKEY_ALGO_UNKNOWN;

	if (data_len >= sizeof(unsigned int)) {
		magic = *(const unsigned int *)data;
		if (magic == 0x52504b31U)
			return SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1;
	}

	if (data_len >= 1 && *(const unsigned char *)data == 0x30)
		return SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1;

	if (data_len >= 26 && !memcmp(text, "-----BEGIN PUBLIC KEY-----", 26))
		return SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1;

	if (data_len >= 30 && !memcmp(text, "-----BEGIN RSA PUBLIC KEY-----", 30))
		return SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1;

	return SECURITY_PUBKEY_ALGO_UNKNOWN;
}

static unsigned int keybox_detect_class_from_name(const char *name)
{
	if (!name || !name[0])
		return KEYBOX_OBJECT_CLASS_UNKNOWN;

	if (strstr(name, ".der") || strstr(name, ".pem") ||
	    strstr(name, "pub") || strstr(name, "public"))
		return KEYBOX_OBJECT_CLASS_PUBKEY;

	if (strstr(name, "cert"))
		return KEYBOX_OBJECT_CLASS_CERT;

	if (strstr(name, "KEY") || strstr(name, "key"))
		return KEYBOX_OBJECT_CLASS_SECRET;

	return KEYBOX_OBJECT_CLASS_BLOB;
}

static unsigned int keybox_class_from_usage_algo(unsigned int usage,
					 unsigned int algo)
{
	if (algo == KEYBOX_ALGO_RSA2048_SHA256_PKCS1 ||
	    algo == SECURITY_PUBKEY_ALGO_RSA2048_SHA256_PKCS1)
		return KEYBOX_OBJECT_CLASS_PUBKEY;

	switch (usage) {
	case KEYBOX_OBJECT_USAGE_DM_VERITY:
		return KEYBOX_OBJECT_CLASS_PUBKEY;
	case KEYBOX_OBJECT_USAGE_LUKS_CONFIG:
	case KEYBOX_OBJECT_USAGE_LUKS_PART_KEY:
	case KEYBOX_OBJECT_USAGE_LUKS_LOG_KEY:
	case KEYBOX_OBJECT_USAGE_UPDATE_VERIFY:
		return KEYBOX_OBJECT_CLASS_SECRET;
	default:
		return KEYBOX_OBJECT_CLASS_UNKNOWN;
	}
}

static int keybox_add_object(struct keybox_ctx *ctx,
		      const struct keybox_object *obj)
{
	if (!ctx || !obj)
		return -EINVAL;

	if (ctx->object_count >= KEYBOX_MAX_OBJECTS)
		return -ENOSPC;

	memcpy(&ctx->objects[ctx->object_count], obj, sizeof(*obj));
	ctx->object_count++;
	return 0;
}

static int keybox_parse_secstore(struct keybox_ctx *ctx,
			 const unsigned char *buf, unsigned int size)
{
	const struct keybox_secstore_map_info *map;
	const struct keybox_secstore_object *obj;
	struct keybox_object out;
	unsigned int max_slot;
	unsigned int slot;
	int map_valid;
	int ret;

	if (!ctx || !buf || size < KEYBOX_SECSTORE_BLOCK_SIZE)
		return -EMSGSIZE;

	map = (const struct keybox_secstore_map_info *)buf;
	map_valid = keybox_secstore_map_valid(map);
	max_slot = size / KEYBOX_SECSTORE_BLOCK_SIZE;
	if (max_slot > KEYBOX_SECSTORE_MAX_ITEM)
		max_slot = KEYBOX_SECSTORE_MAX_ITEM;

	for (slot = 0; slot < max_slot; slot++) {
		if (slot == 0 && map_valid)
			continue;

		obj = (const struct keybox_secstore_object *)
			(buf + slot * KEYBOX_SECSTORE_BLOCK_SIZE);
		if (!keybox_secstore_object_valid(obj))
			continue;

		memset(&out, 0, sizeof(out));
		memcpy(out.name, obj->name,
		       keybox_strnlen_local((const char *)obj->name,
					   sizeof(obj->name)));
		out.name[KEYBOX_NAME_MAX_LEN - 1] = '\0';
		out.class_id = keybox_detect_class_from_name(out.name);
		out.usage = KEYBOX_OBJECT_USAGE_GENERIC;
		out.key_id = obj->id > 0 ? (unsigned int)obj->id : 0;
		out.algo = keybox_detect_algo(obj->data, obj->actual_len);
		out.flags = KEYBOX_OBJECT_FLAG_NONE;
		if (obj->re_encrypt == KEYBOX_SECSTORE_REENCRYPT_MAGIC)
			out.flags |= KEYBOX_OBJECT_FLAG_ENCRYPTED |
				     KEYBOX_OBJECT_FLAG_TRUST_CHECK_REQUIRED;
		if (obj->write_protect == KEYBOX_SECSTORE_WRITE_PROTECT_MAGIC)
			out.flags |= KEYBOX_OBJECT_FLAG_WRITE_PROTECTED;
		out.data = (void *)obj->data;
		out.data_len = obj->actual_len;

		ret = keybox_add_object(ctx, &out);
		if (ret)
			return ret;
	}

	return ctx->object_count ? 0 : -EINVAL;
}

static int keybox_parse_kbx1(struct keybox_ctx *ctx,
		     const unsigned char *buf, unsigned int size)
{
	const struct keybox_storage_v1 *header;
	const struct keybox_entry_storage_v1 *entries;
	struct keybox_object out;
	unsigned int i;
	unsigned int entries_size;
	unsigned int data_end;
	int ret;

	if (!ctx || !buf || size < sizeof(*header))
		return -EINVAL;

	header = (const struct keybox_storage_v1 *)buf;
	if (header->version != KEYBOX_VERSION)
		return -EPROTONOSUPPORT;
	if (header->header_size != sizeof(*header))
		return -EINVAL;
	if (!header->container_size || header->container_size > size)
		return -EFBIG;
	if (!header->key_count || header->key_count > KEYBOX_MAX_ENTRIES)
		return -EINVAL;

	entries_size = header->key_count * sizeof(*entries);
	if (header->entries_size != entries_size)
		return -EINVAL;
	if (header->entries_offset + entries_size > header->container_size)
		return -EINVAL;

	entries = (const struct keybox_entry_storage_v1 *)(buf + header->entries_offset);
	for (i = 0; i < header->key_count; i++) {
		if (!entries[i].usage || !entries[i].data_size)
			return -EINVAL;

		data_end = entries[i].data_offset + entries[i].data_size;
		if (data_end > header->container_size)
			return -EINVAL;

		memset(&out, 0, sizeof(out));
		out.class_id = keybox_class_from_usage_algo(entries[i].usage,
						    entries[i].algo);
		out.usage = entries[i].usage;
		out.key_id = entries[i].key_id;
		out.algo = entries[i].algo;
		out.flags = KEYBOX_OBJECT_FLAG_NONE;
		if (entries[i].flags & 0x00000001U)
			out.flags |= KEYBOX_OBJECT_FLAG_ENCRYPTED;
		if (entries[i].flags & 0x00000002U)
			out.flags |= KEYBOX_OBJECT_FLAG_TRUST_CHECK_REQUIRED;
		out.data = ctx->container + entries[i].data_offset;
		out.data_len = entries[i].data_size;

		ret = keybox_add_object(ctx, &out);
		if (ret)
			return ret;
	}

	return 0;
}

static int keybox_parse_container(struct keybox_ctx *ctx,
			  const unsigned char *buf, unsigned int size)
{
	const struct keybox_storage_v1 *header;

	if (!ctx || !buf || size < sizeof(unsigned int))
		return -EINVAL;

	header = (const struct keybox_storage_v1 *)buf;
	if (header->magic == KEYBOX_MAGIC)
		return keybox_parse_kbx1(ctx, buf, size);

	return keybox_parse_secstore(ctx, buf, size);
}

static int keybox_read_plain_container(struct keybox_ctx *ctx,
			      const unsigned char *header,
			      unsigned int header_size)
{
	const struct keybox_storage_v1 *storage;
	unsigned int total_size = header_size;
	unsigned int actual_size = 0;
	int ret;

	storage = (const struct keybox_storage_v1 *)header;
	if (storage->magic == KEYBOX_MAGIC) {
		total_size = storage->container_size;
		if (!total_size || total_size > KEYBOX_CONTAINER_MAX_LEN)
			return -EFBIG;
	} else if (ctx->backend_ops->get_size) {
		ret = ctx->backend_ops->get_size(ctx->priv, &total_size);
		if (ret)
			return ret;
		if (total_size > KEYBOX_CONTAINER_MAX_LEN)
			total_size = KEYBOX_CONTAINER_MAX_LEN;
		if (total_size < header_size)
			total_size = header_size;
	} else {
		total_size = header_size;
	}

	ctx->container = malloc(total_size);
	if (!ctx->container)
		return -ENOMEM;

	ret = ctx->backend_ops->read(ctx->priv, 0, ctx->container, total_size,
				     &actual_size);
	if (ret) {
		free(ctx->container);
		ctx->container = NULL;
		return ret;
	}
	if (actual_size != total_size) {
		free(ctx->container);
		ctx->container = NULL;
		return -EMSGSIZE;
	}

	ctx->container_size = total_size;
	return 0;
}

int keybox_open(struct keybox_ctx *ctx,
	       const struct keybox_backend_ops *backend_ops,
	       const struct keybox_secure_ops *secure_ops,
	       void *priv)
{
	unsigned char header[KEYBOX_HEADER_READ_LEN];
	unsigned int actual_size = 0;
	void *plain_buf = NULL;
	unsigned int plain_size = 0;
	int ret;

	if (!ctx || !backend_ops || !backend_ops->read)
		return -EINVAL;

	if (ctx->opened)
		return 0;

	memset(ctx, 0, sizeof(*ctx));
	ctx->backend_ops = backend_ops;
	ctx->secure_ops = secure_ops;
	ctx->priv = priv;

	memset(header, 0, sizeof(header));
	ret = backend_ops->read(priv, 0, header, sizeof(header), &actual_size);
	if (ret)
		return ret;
	if (actual_size != sizeof(header))
		return -EMSGSIZE;

	if (secure_ops && secure_ops->unwrap) {
		ret = secure_ops->unwrap(priv, backend_ops, header, sizeof(header),
					&plain_buf, &plain_size);
		if (!ret) {
			ctx->container = plain_buf;
			ctx->container_size = plain_size;
		} else if (ret != -ENOSYS && ret != -ENOENT) {
			return ret;
		}
	}

	if (!ctx->container) {
		ret = keybox_read_plain_container(ctx, header, sizeof(header));
		if (ret)
			return ret;
	}

	ret = keybox_parse_container(ctx, ctx->container, ctx->container_size);
	if (ret) {
		free(ctx->container);
		ctx->container = NULL;
		ctx->container_size = 0;
		return ret;
	}

	ctx->opened = 1;
	return 0;
}

void keybox_close(struct keybox_ctx *ctx)
{
	if (!ctx)
		return;

	if (ctx->container)
		free(ctx->container);
	if (ctx->owns_priv && ctx->priv)
		free(ctx->priv);

	memset(ctx, 0, sizeof(*ctx));
}

int keybox_verify(struct keybox_ctx *ctx)
{
	int ret;

	if (!ctx || !ctx->opened || !ctx->container)
		return -EINVAL;

	if (ctx->verified)
		return 0;

	if (ctx->secure_ops && ctx->secure_ops->verify_container) {
		ret = ctx->secure_ops->verify_container(ctx->priv, ctx->container,
						       ctx->container_size);
		if (ret)
			return ret;
	}

	ctx->verified = 1;
	return 0;
}

static int keybox_selector_match(const struct keybox_selector *selector,
			 const struct keybox_object *obj)
{
	if (!selector || !obj)
		return 0;

	if (selector->name && selector->name[0]) {
		if (!obj->name[0])
			return 0;
		if (strcmp(selector->name, obj->name))
			return 0;
	}

	if (selector->class_id && selector->class_id != obj->class_id)
		return 0;
	if (selector->usage && selector->usage != obj->usage)
		return 0;
	if (selector->key_id && selector->key_id != obj->key_id)
		return 0;
	if (selector->algo && selector->algo != obj->algo)
		return 0;

	return 1;
}

int keybox_find_object(struct keybox_ctx *ctx,
	      const struct keybox_selector *selector,
	      struct keybox_object *out)
{
	struct keybox_object *match = NULL;
	unsigned int i;
	int ret;

	if (!ctx || !selector || !out)
		return -EINVAL;
	if (!ctx->opened)
		return -EINVAL;
	if (!ctx->verified)
		return -EACCES;

	for (i = 0; i < ctx->object_count; i++) {
		if (!keybox_selector_match(selector, &ctx->objects[i]))
			continue;
		if (match)
			return -ENOTUNIQ;
		match = &ctx->objects[i];
	}

	if (!match)
		return -ENOENT;

	if ((match->flags & KEYBOX_OBJECT_FLAG_ENCRYPTED) &&
	    !(ctx->decrypted_mask & (1U << (match - ctx->objects)))) {
		if (!ctx->secure_ops || !ctx->secure_ops->decrypt_object)
			return -ENOSYS;
		ret = ctx->secure_ops->decrypt_object(ctx->priv, match);
		if (ret)
			return ret;
		ctx->decrypted_mask |= (1U << (match - ctx->objects));
	}

	if ((match->flags & KEYBOX_OBJECT_FLAG_TRUST_CHECK_REQUIRED) &&
	    !(ctx->trust_checked_mask & (1U << (match - ctx->objects)))) {
		if (!ctx->secure_ops || !ctx->secure_ops->check_object_trust)
			return -ENOSYS;
		ret = ctx->secure_ops->check_object_trust(ctx->priv, match);
		if (ret)
			return ret;
		ctx->trust_checked_mask |= (1U << (match - ctx->objects));
	}

	memcpy(out, match, sizeof(*out));
	return 0;
}

int keybox_find_object_by_name(struct keybox_ctx *ctx, const char *name,
		       struct keybox_object *out)
{
	struct keybox_selector selector;

	memset(&selector, 0, sizeof(selector));
	selector.name = name;
	return keybox_find_object(ctx, &selector, out);
}

int keybox_object_to_pubkey(const struct keybox_object *obj,
		    struct security_pubkey *out)
{
	if (!obj || !out || !obj->data || !obj->data_len)
		return -EINVAL;

	memset(out, 0, sizeof(*out));
	out->key_id = obj->key_id;
	out->algo = obj->algo ? obj->algo : SECURITY_PUBKEY_ALGO_UNKNOWN;
	out->data = obj->data;
	out->data_len = obj->data_len;
	return 0;
}

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT)
static void keybox_sc_prepare_runtime_hw(void)
{
	unsigned int *pdma_ins = (unsigned int *)pdma_wait;
	volatile unsigned int *pdma_bank0 =
		(volatile unsigned int *)TCSM_BANK0;
	unsigned int i;

	REG32(CPM_BASE + CPM_CLKGR0) = 0;
	REG32(CPM_BASE + CPM_CLKGR1) = 0;

	reset_mcu();
	for (i = 0; i < 6; i++)
		pdma_bank0[i] = pdma_ins[i];

	boot_up_mcu();
	mdelay(500);
}

static int keybox_sc_runtime_init(void)
{
	static int initialized;

	if (initialized)
		return 0;

	keybox_sc_prepare_runtime_hw();
	initialized = 1;
	return 0;
}
#endif

static int keybox_parse_secure_image_len(const unsigned char *buf,
				       unsigned int size,
				       unsigned int *payload_len)
{
	const unsigned int *words = (const unsigned int *)buf;
	unsigned int code_len;

	if (!buf || !payload_len)
		return -EINVAL;
	if (size < KEYBOX_SCBOOT_HEADER_SIZE)
		return -EMSGSIZE;
	if (words[0] != KEYBOX_SECURE_SCBOOT_MAGIC)
		return -EINVAL;

	code_len = words[KEYBOX_SCBOOT_MAGIC_SIZE / sizeof(unsigned int)];
	if (!code_len || code_len > KEYBOX_SECSTORE_IMAGE_MAX_LEN)
		return -EFBIG;

	*payload_len = code_len;
	return 0;
}

static int keybox_secure_unwrap_scboot(
	void *priv, const struct keybox_backend_ops *backend_ops,
	const void *header, unsigned int header_size,
	void **out_buf, unsigned int *out_size)
{
#if !(defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT) && \
	CONFIG_KEYBOX_OUTER_SCBOOT_ENABLE)
	(void)priv;
	(void)backend_ops;
	(void)header;
	(void)header_size;
	(void)out_buf;
	(void)out_size;
	return -ENOSYS;
#else
	unsigned char *secure_buf = NULL;
	unsigned char *plain_buf = NULL;
	unsigned int actual_size = 0;
	unsigned int payload_len = 0;
	unsigned int secure_len;
	int ret;

	if (!backend_ops || !backend_ops->read || !header || !out_buf || !out_size)
		return -EINVAL;

	ret = keybox_parse_secure_image_len(header, header_size, &payload_len);
	if (ret)
		return -ENOENT;

	secure_len = KEYBOX_SCBOOT_HEADER_SIZE + payload_len;
	secure_buf = malloc(secure_len);
	plain_buf = malloc(payload_len);
	if (!secure_buf || !plain_buf) {
		ret = -ENOMEM;
		goto out;
	}

	ret = backend_ops->read(priv, 0, secure_buf, secure_len, &actual_size);
	if (ret)
		goto out;
	if (actual_size != secure_len) {
		ret = -EMSGSIZE;
		goto out;
	}

	ret = secure_scboot(secure_buf, plain_buf);
	if (ret) {
		ret = -EACCES;
		goto out;
	}

	*out_buf = plain_buf;
	*out_size = payload_len;
	plain_buf = NULL;
	ret = 0;
out:
	if (plain_buf)
		free(plain_buf);
	if (secure_buf)
		free(secure_buf);
	return ret;
#endif
}

static int keybox_secure_verify_container(void *priv, const void *container,
				  unsigned int size)
{
	const struct keybox_storage_v1 *header = container;

	(void)priv;
	if (!container || size < sizeof(*header))
		return -EINVAL;
	if (header->magic != KEYBOX_MAGIC)
		return 0;
	if (header->container_size != size)
		return -EINVAL;
	if (!header->auth_size)
		return 0;
	if (header->auth_offset + header->auth_size > size)
		return -EINVAL;
	if (header->flags & KEYBOX_FLAG_UNSIGNED_TEST)
		return 0;
	return -ENOSYS;
}

static int keybox_secure_decrypt_userkey0(void *priv, struct keybox_object *obj)
{
#if !(defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_CKEYAES) && \
	CONFIG_KEYBOX_AES_USERKEY0_ENABLE)
	(void)priv;
	(void)obj;
	return -ENOSYS;
#else
	unsigned long data_pa;
	int ret;

	(void)priv;
	if (!obj || !obj->data || !obj->data_len || (obj->data_len & 0xf))
		return -EINVAL;

	data_pa = virt_to_phys(obj->data);
	ret = do_aes_dma((void *)data_pa, (void *)data_pa, obj->data_len,
			 KEYBOX_AES_BY_UKEY, KEYBOX_AES_CRYPT);
	if (ret)
		return -EACCES;

	flush_cache_all();
	return 0;
#endif
}

#if defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT) && \
	CONFIG_KEYBOX_NKU_CHECK_ENABLE
static int keybox_sc_check_rsa_pubkey_nku(
	const struct security_rsa_public_key *rsa_key)
{
	volatile struct sc_args *args;
	volatile unsigned int *nku_words;
	unsigned int ret;
	unsigned int i;

	if (!rsa_key || rsa_key->len != SECURITY_RSA2048_WORDS)
		return -EINVAL;

	args = GET_SC_ARGS();
	nku_words = (volatile unsigned int *)MCU_TCSM_NKU;

	ret = keybox_sc_runtime_init();
	if (ret)
		return ret;
	ret = secall(args, SC_FUNC_INIT, 0, 1);
	if (ret != SC_ERR_SUCC)
		return -EIO;

	nku_words[0] = rsa_key->len * 32;
	nku_words[1] = rsa_key->len * 32;
	for (i = 0; i < rsa_key->len; i++)
		nku_words[i + 2] = rsa_key->modulus[rsa_key->len - 1 - i];

	for (i = 0; i < rsa_key->len; i++)
		nku_words[i + 2 + rsa_key->len] = 0;
	nku_words[2 + rsa_key->len * 2 - 1] = 65537U;

	args->arg[0] = MCU_TCSM_PADDR(nku_words);
	ret = secall(args, SC_FUNC_CHECKNKU, 0, 1);
	if (ret != SC_ERR_SUCC)
		return -EACCES;

	return 0;
}
#endif

static int keybox_secure_check_nku(void *priv,
			  const struct keybox_object *obj)
{
	struct security_pubkey pubkey;
	struct security_rsa_public_key rsa_key;
	int ret;

	(void)priv;
#if !(defined(CONFIG_JZ_SCBOOT) && defined(CONFIG_JZ_SECURE_SUPPORT) && \
	CONFIG_KEYBOX_NKU_CHECK_ENABLE)
	(void)obj;
	return -ENOSYS;
#else
	if (!obj)
		return -EINVAL;

	memset(&pubkey, 0, sizeof(pubkey));
	memset(&rsa_key, 0, sizeof(rsa_key));
	ret = keybox_object_to_pubkey(obj, &pubkey);
	if (ret)
		return ret;
	ret = security_pubkey_parse_rsa(&pubkey, &rsa_key);
	if (ret)
		return ret;
	return keybox_sc_check_rsa_pubkey_nku(&rsa_key);
#endif
}

static const struct keybox_secure_ops keybox_source_secure_ops = {
	.unwrap = keybox_secure_unwrap_scboot,
	.verify_container = keybox_secure_verify_container,
	.decrypt_object = keybox_secure_decrypt_userkey0,
	.check_object_trust = keybox_secure_check_nku,
};

int keybox_open_source(struct keybox_ctx *ctx,
	       const struct keybox_source_desc *source)
{
	struct keybox_source_runtime *runtime;
	int ret;

	if (!ctx || !source)
		return -EINVAL;
	if (ctx->opened)
		return -EBUSY;

	runtime = malloc(sizeof(*runtime));
	if (!runtime)
		return -ENOMEM;

	memset(runtime, 0, sizeof(*runtime));
	memcpy(&runtime->source, source, sizeof(*source));

	ret = keybox_open(ctx, &keybox_source_backend_ops,
			  &keybox_source_secure_ops, runtime);
	if (ret) {
		free(runtime);
		return ret;
	}

	ctx->owns_priv = 1;
	return 0;
}

int keybox_load_pubkey_from_source(struct keybox_ctx *ctx,
	      const struct keybox_source_desc *source,
	      const struct keybox_selector *selector,
	      struct security_pubkey *out)
{
	struct keybox_object obj;
	int ret;

	if (!ctx || !source || !selector || !out)
		return -EINVAL;

	ret = keybox_open_source(ctx, source);
	if (ret)
		return ret;

	ret = keybox_verify(ctx);
	if (ret)
		goto err_close;

	memset(&obj, 0, sizeof(obj));
	ret = keybox_find_object(ctx, selector, &obj);
	if (ret)
		goto err_close;

	ret = keybox_object_to_pubkey(&obj, out);
	if (ret)
		goto err_close;

	return 0;

err_close:
	keybox_close(ctx);
	return ret;
}
