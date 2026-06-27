/*
 * TPM_TIS_SPI TPM2 over SPI helper API.
 *
 * This API is intentionally small so board code / boot flow can reuse it for
 * measured-boot gating.
 */

#ifndef __TPM_TIS_SPI_H__
#define __TPM_TIS_SPI_H__

#include <common.h>

#define TPM_TIS_SPI_DIGEST_SIZE 32
#define TPM_TIS_SPI_SIGNATURE_SIZE 64
#define TPM_TIS_SPI_PUBKEY_XY_SIZE 64

#define TPM_TIS_SPI_POLICY_NV_UNDEFINED  0
#define TPM_TIS_SPI_POLICY_NV_CONFIGURED 1
#define TPM_TIS_SPI_POLICY_NV_INVALID    2

int tpm_tis_spi_probe(u32 *did_vid, u8 *rid);
int tpm_tis_spi_nv_define_space(u32 auth_handle, u32 nv_index,
				u16 size, u32 attrs);
int tpm_tis_spi_nv_undefine_space(u32 auth_handle, u32 nv_index);
int tpm_tis_spi_nv_read_full(u32 auth_handle, u32 nv_index,
			     u8 *data, u16 data_len);
int tpm_tis_spi_nv_write_full(u32 auth_handle, u32 nv_index,
			      const u8 *data, u16 data_len);
int tpm_tis_spi_nv_index_defined(u32 nv_index);
int tpm_tis_spi_nv_read_public(u32 nv_index, u16 *data_size, u32 *attrs);
int tpm_tis_spi_hash_sm3_mem(ulong addr, ulong len, u8 digest[TPM_TIS_SPI_DIGEST_SIZE]);
int tpm_tis_spi_verify_mem_with_hex(ulong addr, ulong len, const char *hex_digest);
int tpm_tis_spi_verify_mem_with_env(ulong addr, ulong len, const char *env_name);
int tpm_tis_spi_sigverify_sm2_handle(u32 key_handle,
				     const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				     const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE]);
int tpm_tis_spi_sigverify_sm2_external(const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				       const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE],
				       const u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE]);
int tpm_tis_spi_sigverify_sm2_policy_nv(u32 nv_index, u32 auth_handle,
				       const u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				       const u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE]);
/* Read policy NV pubkey area and classify it without verifying signatures. */
int tpm_tis_spi_policy_nv_configured(u32 nv_index, u32 auth_handle);
void tpm_tis_spi_print_digest(const u8 digest[TPM_TIS_SPI_DIGEST_SIZE]);

#endif /* __TPM_TIS_SPI_H__ */
