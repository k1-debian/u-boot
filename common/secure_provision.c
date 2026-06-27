/*
 * Secure provisioning flow for TPM2 NV policy/certificate material.
 */

#include <common.h>
#include <asm-generic/errno.h>
#include <linux/string.h>

#include <secure_provision.h>
#include <tpm_tis_spi.h>

#define SECURE_PROVISION_POLICY_PUBKEY_OFFSET 0x004c
#define SECURE_PROVISION_POLICY_PUBKEY_SIZE   65

/*
 * Match the attributes produced by:
 * nvdefinespace -ha 013fffee -hi p -hia p -ty o +at wst +at wa +at wd -at da -sz 224 -nalg sm3
 */
#define SECURE_PROVISION_POLICY_NV_ATTRS      0x40017001

/*
 * Match the attributes produced by:
 * nvdefinespace -ha 013fffef -hi p -hia p -ty o +at or +at ar +at wd -at da -sz 56 -nalg sm3
 */
#define SECURE_PROVISION_REPORT_NV_ATTRS      0x40072001

/*
 * Match the attributes produced by:
 * nvdefinespace -ha 01800030 -hi o -hia o -ty o -sz 512 -nalg sm3
 */
#define SECURE_PROVISION_CERT_NV_ATTRS        0x02020002
#define SECURE_PROVISION_NV_WRITEDEFINE       0x00002000
#define SECURE_PROVISION_NV_WRITTEN           0x20000000
#define SECURE_PROVISION_NV_PUBLIC_ATTR_MASK  0xdfffffff

#ifdef CONFIG_SECURE_PROVISION_EMBEDDED
#ifndef CONFIG_SECURE_PROVISION_POLICY_BLOB
#error "CONFIG_SECURE_PROVISION_POLICY_BLOB is required for embedded provisioning"
#endif
#ifndef CONFIG_SECURE_PROVISION_CERT_BLOB
#error "CONFIG_SECURE_PROVISION_CERT_BLOB is required for embedded provisioning"
#endif

static const u8 secure_provision_policy[] = CONFIG_SECURE_PROVISION_POLICY_BLOB;
static const u8 secure_provision_cert[] = CONFIG_SECURE_PROVISION_CERT_BLOB;

#define secure_provision_policy_len sizeof(secure_provision_policy)
#define secure_provision_cert_len   sizeof(secure_provision_cert)
#endif

static bool secure_provision_nv_public_matches(u16 size, u32 attrs,
					       u16 expected_size,
					       u32 expected_attrs)
{
	return size == expected_size &&
	       ((attrs ^ expected_attrs) & SECURE_PROVISION_NV_PUBLIC_ATTR_MASK) == 0;
}

int secure_provision_validate_policy(const u8 *policy, u16 policy_len)
{
	if (!policy)
		return -EINVAL;
	if (policy_len != SECURE_PROVISION_POLICY_NV_SIZE)
		return -EINVAL;
	if (policy[SECURE_PROVISION_POLICY_PUBKEY_OFFSET] != 0x04)
		return -EINVAL;

	return 0;
}

static int secure_provision_der_total_len(const u8 *cert, u16 cert_len,
					  u16 *total_len)
{
	u16 len;

	if (!cert || !total_len || cert_len < 2)
		return -EINVAL;
	if (cert[0] != 0x30)
		return -EINVAL;

	if (!(cert[1] & 0x80)) {
		len = cert[1];
		*total_len = len + 2;
		return 0;
	}

	if (cert[1] == 0x81) {
		if (cert_len < 3)
			return -EINVAL;
		len = cert[2];
		*total_len = len + 3;
		return 0;
	}

	if (cert[1] == 0x82) {
		if (cert_len < 4)
			return -EINVAL;
		len = ((u16)cert[2] << 8) | cert[3];
		*total_len = len + 4;
		return 0;
	}

	return -EINVAL;
}

int secure_provision_validate_cert_der(const u8 *cert, u16 cert_len)
{
	u16 total_len;
	int rc;

	if (!cert || !cert_len || cert_len > SECURE_PROVISION_CERT_NV_SIZE)
		return -EINVAL;

	rc = secure_provision_der_total_len(cert, cert_len, &total_len);
	if (rc)
		return rc;
	if (total_len != cert_len)
		return -EINVAL;

	return 0;
}

static int secure_provision_ensure_defined(u32 auth_handle, u32 nv_index,
					   u16 nv_size, u32 attrs,
					   u32 flags, const char *name)
{
	u32 current_attrs;
	u16 current_size;
	int rc;

	rc = tpm_tis_spi_nv_read_public(nv_index, &current_size, &current_attrs);
	if (!rc) {
		if (!secure_provision_nv_public_matches(current_size, current_attrs,
							nv_size, attrs)) {
			printf("secureprov: %s NV 0x%08x public mismatch size=%u attrs=0x%08x expected size=%u attrs=0x%08x\n",
			       name, nv_index, current_size, current_attrs,
			       nv_size, attrs);
			if (!(flags & SECURE_PROVISION_F_FORCE))
				return -EEXIST;

			rc = tpm_tis_spi_nv_undefine_space(auth_handle, nv_index);
			if (rc) {
				printf("secureprov: undefine %s NV 0x%08x failed: %d\n",
				       name, nv_index, rc);
				return rc;
			}
			printf("secureprov: undefined %s NV 0x%08x for force redefine\n",
			       name, nv_index);
			rc = -ENOENT;
		} else {
			printf("secureprov: %s NV 0x%08x already defined\n",
			       name, nv_index);
			return 0;
		}
	}
	if (rc != -ENOENT)
		return rc;

	rc = tpm_tis_spi_nv_define_space(auth_handle, nv_index, nv_size, attrs);
	if (rc) {
		printf("secureprov: define %s NV 0x%08x failed: %d\n",
		       name, nv_index, rc);
		return rc;
	}

	printf("secureprov: defined %s NV 0x%08x size=%u\n",
	       name, nv_index, nv_size);
	return 0;
}

static int secure_provision_read_cmp(u32 auth_handle, u32 nv_index,
				     const u8 *expected, u16 expected_len,
				     const char *name)
{
	u8 current[SECURE_PROVISION_CERT_NV_SIZE];
	int rc;

	if (expected_len > sizeof(current))
		return -EINVAL;

	rc = tpm_tis_spi_nv_read_full(auth_handle, nv_index, current, expected_len);
	if (rc)
		return rc;

	if (memcmp(current, expected, expected_len)) {
		printf("secureprov: %s NV 0x%08x content mismatch\n", name, nv_index);
		return -EEXIST;
	}

	return 0;
}

static int secure_provision_define_write_verify(u32 auth_handle, u32 nv_index,
						u16 nv_size, u32 attrs,
						const u8 *data,
						u32 flags,
						const char *name)
{
	bool force = flags & SECURE_PROVISION_F_FORCE;
	u32 current_attrs;
	u16 current_size;
	int read_rc;
	int rc;

	rc = tpm_tis_spi_nv_read_public(nv_index, &current_size, &current_attrs);
	if (rc && rc != -ENOENT)
		return rc;

	if (!rc && !secure_provision_nv_public_matches(current_size, current_attrs,
						      nv_size, attrs)) {
		printf("secureprov: %s NV 0x%08x public mismatch size=%u attrs=0x%08x expected size=%u attrs=0x%08x\n",
		       name, nv_index, current_size, current_attrs, nv_size, attrs);
		if (!force)
			return -EEXIST;
		read_rc = -EEXIST;
	} else if (!rc) {
		rc = secure_provision_read_cmp(auth_handle, nv_index, data, nv_size, name);
		read_rc = rc;
		if (rc && !force)
			return rc;
	} else {
		read_rc = rc;
	}

	if (!read_rc) {
		printf("secureprov: %s NV 0x%08x already configured\n", name, nv_index);
		return 0;
	}

	if (read_rc == -ENOENT) {
		rc = tpm_tis_spi_nv_define_space(auth_handle, nv_index, nv_size, attrs);
		if (rc) {
			printf("secureprov: define %s NV 0x%08x failed: %d\n",
			       name, nv_index, rc);
			return rc;
		}
		printf("secureprov: defined %s NV 0x%08x size=%u\n",
		       name, nv_index, nv_size);
	} else if (force) {
		printf("secureprov: force reprovision %s NV 0x%08x after read rc=%d\n",
		       name, nv_index, read_rc);

		rc = tpm_tis_spi_nv_undefine_space(auth_handle, nv_index);
		if (rc) {
			printf("secureprov: undefine %s NV 0x%08x failed: %d\n",
			       name, nv_index, rc);
			return rc;
		}
		printf("secureprov: undefined %s NV 0x%08x for force reprovision\n",
		       name, nv_index);

		rc = tpm_tis_spi_nv_define_space(auth_handle, nv_index, nv_size, attrs);
		if (rc) {
			printf("secureprov: redefine %s NV 0x%08x failed: %d\n",
			       name, nv_index, rc);
			return rc;
		}
		printf("secureprov: redefined %s NV 0x%08x size=%u\n",
		       name, nv_index, nv_size);
	}

	rc = tpm_tis_spi_nv_write_full(auth_handle, nv_index, data, nv_size);
	if (rc) {
		printf("secureprov: write %s NV 0x%08x failed: %d\n",
		       name, nv_index, rc);
		return rc;
	}

	rc = secure_provision_read_cmp(auth_handle, nv_index, data, nv_size, name);
	if (rc) {
		printf("secureprov: verify %s NV 0x%08x failed: %d\n",
		       name, nv_index, rc);
		return rc;
	}

	printf("secureprov: programmed %s NV 0x%08x size=%u\n",
	       name, nv_index, nv_size);
	return 0;
}

int secure_provision_status(void)
{
	u8 policy[SECURE_PROVISION_POLICY_NV_SIZE];
	u8 cert[SECURE_PROVISION_CERT_NV_SIZE];
	int policy_rc;
	int report_rc;
	int cert_rc;

	policy_rc = tpm_tis_spi_nv_read_full(SECURE_PROVISION_POLICY_NV_AUTH,
					     SECURE_PROVISION_POLICY_NV_INDEX,
					     policy, sizeof(policy));
	report_rc = tpm_tis_spi_nv_index_defined(SECURE_PROVISION_REPORT_NV_INDEX);
	cert_rc = tpm_tis_spi_nv_read_full(SECURE_PROVISION_CERT_NV_AUTH,
					   SECURE_PROVISION_CERT_NV_INDEX,
					   cert, sizeof(cert));

	printf("secureprov: policy NV 0x%08x %s\n",
	       SECURE_PROVISION_POLICY_NV_INDEX,
	       (policy_rc == -ENOENT) ? "undefined" :
	       (policy_rc ? "error" : "defined"));
	printf("secureprov: report NV 0x%08x %s\n",
	       SECURE_PROVISION_REPORT_NV_INDEX,
	       (report_rc == -ENOENT) ? "undefined" :
	       (report_rc ? "error" : "defined"));
	printf("secureprov: cert NV 0x%08x %s\n",
	       SECURE_PROVISION_CERT_NV_INDEX,
	       (cert_rc == -ENOENT) ? "undefined" :
	       (cert_rc ? "error" : "defined"));

	if (policy_rc && policy_rc != -ENOENT)
		return policy_rc;
	if (report_rc && report_rc != -ENOENT)
		return report_rc;
	if (cert_rc && cert_rc != -ENOENT)
		return cert_rc;
	return (policy_rc == -ENOENT || report_rc == -ENOENT ||
		cert_rc == -ENOENT) ? -ENOENT : 0;
}

int secure_provision_apply_buffers(const u8 *policy, u16 policy_len,
				   const u8 *cert, u16 cert_len,
				   u32 flags)
{
	u8 cert_nv[SECURE_PROVISION_CERT_NV_SIZE];
	int rc;

	rc = secure_provision_validate_policy(policy, policy_len);
	if (rc) {
		printf("secureprov: invalid policy buffer len=%u\n", policy_len);
		return rc;
	}

	rc = secure_provision_validate_cert_der(cert, cert_len);
	if (rc) {
		printf("secureprov: invalid cert DER len=%u\n", cert_len);
		return rc;
	}

	memset(cert_nv, 0, sizeof(cert_nv));
	memcpy(cert_nv, cert, cert_len);

	rc = secure_provision_define_write_verify(SECURE_PROVISION_CERT_NV_AUTH,
						 SECURE_PROVISION_CERT_NV_INDEX,
						 SECURE_PROVISION_CERT_NV_SIZE,
						 SECURE_PROVISION_CERT_NV_ATTRS,
						 cert_nv, flags, "cert");
	if (rc)
		return rc;

	rc = secure_provision_ensure_defined(SECURE_PROVISION_REPORT_NV_AUTH,
					     SECURE_PROVISION_REPORT_NV_INDEX,
					     SECURE_PROVISION_REPORT_NV_SIZE,
					     SECURE_PROVISION_REPORT_NV_ATTRS,
					     flags, "report");
	if (rc)
		return rc;

	return secure_provision_define_write_verify(SECURE_PROVISION_POLICY_NV_AUTH,
						    SECURE_PROVISION_POLICY_NV_INDEX,
						    SECURE_PROVISION_POLICY_NV_SIZE,
						    SECURE_PROVISION_POLICY_NV_ATTRS,
						    policy, flags, "policy");
}

int secure_provision_apply_embedded(u32 flags)
{
#ifdef CONFIG_SECURE_PROVISION_EMBEDDED
	if (secure_provision_policy_len > 0xffff ||
	    secure_provision_cert_len > 0xffff)
		return -EINVAL;
	if (!secure_provision_policy_len || !secure_provision_cert_len) {
		printf("secureprov: embedded provisioning data is not linked\n");
		return -ENOSYS;
	}

	return secure_provision_apply_buffers(secure_provision_policy,
					      (u16)secure_provision_policy_len,
					      secure_provision_cert,
					      (u16)secure_provision_cert_len,
					      flags);
#else
	printf("secureprov: embedded provisioning data is not enabled\n");
	return -ENOSYS;
#endif
}
