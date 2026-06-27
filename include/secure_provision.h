/*
 * Secure provisioning helpers for programming boot policy material into
 * TPM2-compatible NV storage.
 */

#ifndef __SECURE_PROVISION_H__
#define __SECURE_PROVISION_H__

#include <common.h>

#define SECURE_PROVISION_F_FORCE          0x00000001

#define SECURE_PROVISION_POLICY_NV_INDEX  0x013fffee
#define SECURE_PROVISION_POLICY_NV_SIZE   224
#define SECURE_PROVISION_POLICY_NV_AUTH   0x4000000c

#define SECURE_PROVISION_REPORT_NV_INDEX  0x013fffef
#define SECURE_PROVISION_REPORT_NV_SIZE   56
#define SECURE_PROVISION_REPORT_NV_AUTH   0x4000000c

#define SECURE_PROVISION_CERT_NV_INDEX    0x01800030
#define SECURE_PROVISION_CERT_NV_SIZE     512
#define SECURE_PROVISION_CERT_NV_AUTH     0x40000001

int secure_provision_validate_policy(const u8 *policy, u16 policy_len);
int secure_provision_validate_cert_der(const u8 *cert, u16 cert_len);
int secure_provision_status(void);
int secure_provision_apply_buffers(const u8 *policy, u16 policy_len,
				   const u8 *cert, u16 cert_len,
				   u32 flags);
int secure_provision_apply_embedded(u32 flags);

#endif /* __SECURE_PROVISION_H__ */
