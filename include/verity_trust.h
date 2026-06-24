/*
 * verity_trust.h - dm-verity trusted boot library
 *
 * Reads verity metadata from NOR flash, verifies root_hash with
 * TPM TIS SPI SM2 signature, and builds kernel bootargs.
 *
 * Storage layout (mtd2 = rootfs partition):
 *   <padded rootfs> <SM3 hash tree> <metadata 160B @ end of image>
 *
 * Metadata structure (160B):
 *   +0   root_hash     32B
 *   +32  salt          32B
 *   +64  data_blocks   4B   u32 LE
 *   +68  hash_blocks   4B   u32 LE
 *   +72  reserved      24B
 *   +96  signature     64B  SM2(root_hash)
 *
 * TPM communication is done through the public API in
 *   drivers/tpm/tpm_tis_spi.c    (via include/tpm_tis_spi.h)
 *   drivers/spi/jz_spi.c         (SPI transport)
 * This file does NOT contain any raw TPM/SPI I/O.
 *
 * Verification modes (both via tpm_tis_spi.h):
 *   - sigverify_sm2_handle    : TPM internal key (factory-provisioned)
 *   - sigverify_sm2_external  : external XY pubkey (for OTA key rotation)
 */
#ifndef __VERITY_TRUST_H__
#define __VERITY_TRUST_H__

#include <common.h>
#include <image.h>
#include <tpm_tis_spi.h>

#define VERITY_TRUST_METADATA_SIZE    160
#define VERITY_TRUST_BOOTARGS_MAX     1024
#define VERITY_TRUST_KERNEL_SIG_SIZE  64

struct verity_trust_meta {
	u8  root_hash[32];
	u8  salt[32];
	u32 data_blocks;
	u32 hash_blocks;
	u8  reserved[24];
	u8  signature[64];    /* SM2(root_hash), R||S */
};

struct verity_trust_kernel_layout {
	const image_header_t *hdr;
	ulong image_addr;
	ulong payload_addr;
	u32 payload_size;
	ulong sig_addr;
	u32 sig_size;
	u32 signed_size;
	u32 total_size;
};

/* ---- metadata I/O ---- */

/* Read metadata from NOR flash at given offset */
int verity_trust_load_meta(ulong nor_flash_offset,
			    struct verity_trust_meta *meta);

int verity_trust_load_kernel_header(ulong nor_flash_offset, ulong load_addr);

int verity_trust_parse_kernel_layout(ulong image_addr,
				      struct verity_trust_kernel_layout *layout);

int verity_trust_load_kernel_image(ulong nor_flash_offset, ulong load_addr,
				     struct verity_trust_kernel_layout *layout,
				     u32 max_size);

/* ---- signature verification ---- */

/* Verify with TPM internal key handle (legacy, key must be pre-loaded) */
int verity_trust_verify_handle(const struct verity_trust_meta *meta,
				u32 key_handle);

/* Verify via TPM policy NV - pubkey read from inside TPM NV storage.
 * nv_index:     NV index where policy (with pubkey) is stored, e.g. 0x013fffee
 * auth_handle:  authorization handle, e.g. TPM2_RH_PLATFORM
 * This is the recommended method: no U-Boot env dependency for the key. */
int verity_trust_verify_policy_nv(const struct verity_trust_meta *meta,
				   u32 nv_index, u32 auth_handle);

/* Verify with external SM2 public key (X||Y, 64B), e.g. for OTA key rotation */
int verity_trust_verify_external(const struct verity_trust_meta *meta,
				  const u8 pubkey_xy[64]);

int verity_trust_verify_kernel_policy_nv(
		const struct verity_trust_kernel_layout *layout,
		u32 nv_index, u32 auth_handle,
		u8 digest[TPM_TIS_SPI_DIGEST_SIZE]);

int verity_trust_get_nv_auth_from_env(u32 *nv_index, u32 *auth_handle);

int verity_trust_prepare_rootfs_bootargs(const char *base_bootargs,
				      char *bootargs,
				      unsigned int bootargs_size,
				      struct verity_trust_meta *meta);

/* Probe TPM policy NV state without modifying persistent U-Boot env. */
int verity_trust_policy_probe(u32 nv_index, u32 auth_handle);

/* Run kernel verification, rootfs verification, dm-verity bootargs, and boot. */
int verity_trust_boot_trusted(u32 nv_index, u32 auth_handle);

/* Select setup fallback or strict trusted boot according to tl and TPM NV. */
int verity_trust_auto_boot(void);

/* ---- bootargs assembly ---- */

/*
 * Build final bootargs = base_bootargs + dm-mod.create=...
 *
 * base_bootargs: stock cmdline (console, root, init, ...)
 * buf/buf_size:  output buffer (at least VERITY_TRUST_BOOTARGS_MAX)
 * Returns:       string length, or negative on error
 */
int verity_trust_build_bootargs(const char *base_bootargs,
				 const struct verity_trust_meta *meta,
				 char *buf, unsigned int buf_size);

#endif /* __VERITY_TRUST_H__ */
