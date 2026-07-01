/*
 * External secure provisioning package parser.
 */

#include <common.h>
#include <asm-generic/errno.h>
#include <asm/unaligned.h>
#include <linux/string.h>
#include <u-boot/crc.h>

#include <secure_provision.h>

#include "ext_secure_provision.h"

#define EXT_SECURE_PROVISION_ALLOWED_FLAGS	SECURE_PROVISION_F_FORCE
#define EXT_SECURE_PROVISION_U16_MAX		0xffffU

int ext_secure_provision_apply_package(const void *buf, u32 len)
{
	u16 version;
	u16 header_size;
	u32 flags;
	u32 policy_len;
	u32 cert_len;
	u32 payload_crc;
	const u8 *policy;
	const u8 *cert;
	u32 expected_len;
	u32 crc;
	int ret;

	if (!buf || len < sizeof(struct ext_secure_provision_payload_header)) {
		printf("ext secure provision: invalid package buffer len=%u\n", len);
		return -EINVAL;
	}

	version = get_unaligned_le16((const u8 *)buf + 4);
	header_size = get_unaligned_le16((const u8 *)buf + 6);
	flags = get_unaligned_le32((const u8 *)buf + 8);
	policy_len = get_unaligned_le32((const u8 *)buf + 12);
	cert_len = get_unaligned_le32((const u8 *)buf + 16);
	payload_crc = get_unaligned_le32((const u8 *)buf + 20);

	if (memcmp(buf, EXT_SECURE_PROVISION_MAGIC,
		   EXT_SECURE_PROVISION_MAGIC_SIZE)) {
		printf("ext secure provision: invalid magic\n");
		return -EINVAL;
	}
	if (version != EXT_SECURE_PROVISION_VERSION) {
		printf("ext secure provision: unsupported version %u\n",
		       version);
		return -EINVAL;
	}
	if (header_size != sizeof(struct ext_secure_provision_payload_header)) {
		printf("ext secure provision: invalid header size %u\n",
		       header_size);
		return -EINVAL;
	}
	if (flags & ~EXT_SECURE_PROVISION_ALLOWED_FLAGS) {
		printf("ext secure provision: invalid flags 0x%08x\n",
		       flags);
		return -EINVAL;
	}
	if (policy_len != SECURE_PROVISION_POLICY_NV_SIZE) {
		printf("ext secure provision: invalid policy length %u\n",
		       policy_len);
		return -EINVAL;
	}
	if (!cert_len || cert_len > SECURE_PROVISION_CERT_NV_SIZE) {
		printf("ext secure provision: invalid cert length %u\n",
		       cert_len);
		return -EINVAL;
	}
	if (policy_len > EXT_SECURE_PROVISION_U16_MAX ||
	    cert_len > EXT_SECURE_PROVISION_U16_MAX) {
		printf("ext secure provision: length too large policy=%u cert=%u\n",
		       policy_len, cert_len);
		return -EINVAL;
	}
	if (header_size > len ||
	    policy_len > len - header_size ||
	    cert_len > len - header_size - policy_len) {
		printf("ext secure provision: package length overflow len=%u\n",
		       len);
		return -EINVAL;
	}

	expected_len = header_size + policy_len + cert_len;
	if (expected_len != len) {
		printf("ext secure provision: package length mismatch len=%u expected=%u\n",
		       len, expected_len);
		return -EINVAL;
	}

	policy = (const u8 *)buf + header_size;
	cert = policy + policy_len;

	if (payload_crc) {
		crc = crc32(0, policy, policy_len + cert_len);
		if (crc != payload_crc) {
			printf("ext secure provision: payload crc mismatch src=%08x calc=%08x\n",
			       payload_crc, crc);
			return -EINVAL;
		}
	}

	ret = secure_provision_apply_buffers(policy, (u16)policy_len,
					     cert, (u16)cert_len, flags);
	if (ret)
		printf("ext secure provision: apply failed %d\n", ret);
	else
		printf("ext secure provision: applied successfully\n");

	return ret;
}
