/*
 * External secure provisioning package parser.
 */

#ifndef __EXT_SECURE_PROVISION_H__
#define __EXT_SECURE_PROVISION_H__

#include <common.h>
#include <secure_provision.h>

#define EXT_SECURE_PROVISION_MAGIC	"TPRV"
#define EXT_SECURE_PROVISION_MAGIC_SIZE	4
#define EXT_SECURE_PROVISION_VERSION	1

/* Magic is ASCII bytes. Other on-wire header fields are little-endian. */
struct ext_secure_provision_payload_header {
	u8 magic[EXT_SECURE_PROVISION_MAGIC_SIZE];
	u16 version;
	u16 header_size;
	u32 flags;
	u32 policy_len;
	u32 cert_len;
	u32 payload_crc;
	u32 reserved[3];
};

#define EXT_SECURE_PROVISION_MAX_PACKAGE_SIZE	\
	(sizeof(struct ext_secure_provision_payload_header) + \
	 SECURE_PROVISION_POLICY_NV_SIZE + \
	 SECURE_PROVISION_CERT_NV_SIZE)

int ext_secure_provision_apply_package(const void *buf, u32 len);

#endif /* __EXT_SECURE_PROVISION_H__ */
