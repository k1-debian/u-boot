/*
 * verity_trust.c - dm-verity rootfs trust verification library
 *
 * Three-phase trusted boot:
 *   1. Load   - read 160B metadata from NOR flash
 *   2. Verify - TPM_TIS_SPI SM2(root_hash) signature
 *   3. Boot   - assemble dm-mod.create bootargs
 *
 * TPM_TIS_SPI calls delegated to include/tpm_tis_spi.h (impl in drivers/tpm/).
 */
#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <tpm_tis_spi.h>
#include "verity_trust.h"

#ifndef CONFIG_VERITY_TRUST_DATA_DEV_MAJOR
#define CONFIG_VERITY_TRUST_DATA_DEV_MAJOR 31
#endif
#ifndef CONFIG_VERITY_TRUST_DATA_DEV_MINOR
#define CONFIG_VERITY_TRUST_DATA_DEV_MINOR 2
#endif
#ifndef CONFIG_VERITY_TRUST_HASH_DEV_MAJOR
#define CONFIG_VERITY_TRUST_HASH_DEV_MAJOR CONFIG_VERITY_TRUST_DATA_DEV_MAJOR
#endif
#ifndef CONFIG_VERITY_TRUST_HASH_DEV_MINOR
#define CONFIG_VERITY_TRUST_HASH_DEV_MINOR CONFIG_VERITY_TRUST_DATA_DEV_MINOR
#endif
#ifndef CONFIG_VERITY_TRUST_ROOTFS_OFFSET
#error "CONFIG_VERITY_TRUST_ROOTFS_OFFSET must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_METADATA_OFFSET
#error "CONFIG_VERITY_TRUST_METADATA_OFFSET must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_KERNEL_OFFSET
#error "CONFIG_VERITY_TRUST_KERNEL_OFFSET must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_KERNEL_LOAD_ADDR
#error "CONFIG_VERITY_TRUST_KERNEL_LOAD_ADDR must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_KERNEL_MAX_SIZE
#error "CONFIG_VERITY_TRUST_KERNEL_MAX_SIZE must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_NV_INDEX
#error "CONFIG_VERITY_TRUST_NV_INDEX must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_BOOTARGS
#error "CONFIG_VERITY_TRUST_BOOTARGS must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_BOOTCMD
#error "CONFIG_VERITY_TRUST_BOOTCMD must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_NV_AUTH
#define CONFIG_VERITY_TRUST_NV_AUTH  0x4000000C
#endif

/*
 * Auto boot state flow:
 *
 * Runtime dm-verity bootargs are rebuilt from rootfs metadata on every boot
 * and are not saved to persistent env.  TPM_TIS_SPI policy presence selects the
 * provisioning fallback or strict trusted boot path.
 */

/* ---- helpers ---- */

static void bytes_to_hex(const u8 *data, unsigned int len,
			  char *hex, unsigned int hex_size)
{
	static const char lut[] = "0123456789abcdef";
	unsigned int i;

	if (!data || !hex || hex_size < (len * 2u + 1u))
		return;

	for (i = 0; i < len; i++) {
		hex[i * 2]     = lut[(data[i] >> 4) & 0xf];
		hex[i * 2 + 1] = lut[data[i] & 0xf];
	}
	hex[len * 2] = '\0';
}

/* ---- metadata I/O ---- */

int verity_trust_load_meta(ulong flash_offset,
			    struct verity_trust_meta *meta)
{
	char cmd[128];
	u8 *buf;
	int ret;

	if (!meta || !flash_offset)
		return -EINVAL;

	buf = (u8 *)CONFIG_SYS_LOAD_ADDR;
	snprintf(cmd, sizeof(cmd),
		 "sfcnor read %lx %x %lx",
		 flash_offset, VERITY_TRUST_METADATA_SIZE,
		 (ulong)buf);

	ret = run_command(cmd, 0);
	if (ret)
		return -EIO;

	memcpy(meta->root_hash,  buf,        32);
	memcpy(meta->salt,       buf + 32,   32);
	meta->data_blocks = le32_to_cpu(*(u32 *)(buf + 64));
	meta->hash_blocks = le32_to_cpu(*(u32 *)(buf + 68));
	memcpy(meta->reserved,   buf + 72,   24);
	memcpy(meta->signature,  buf + 96,   64);

	return 0;
}

int verity_trust_load_kernel_header(ulong flash_offset, ulong load_addr)
{
	char cmd[128];
	int ret;

	if (!flash_offset || !load_addr)
		return -EINVAL;

	snprintf(cmd, sizeof(cmd),
		 "sfcnor read %lx %x %lx",
		 flash_offset, (unsigned int)sizeof(image_header_t), load_addr);

	ret = run_command(cmd, 0);
	if (ret)
		return -EIO;

	return 0;
}

int verity_trust_parse_kernel_layout(ulong image_addr,
				      struct verity_trust_kernel_layout *layout)
{
	const image_header_t *hdr = (const image_header_t *)image_addr;
	u32 payload_size;
	u32 signed_size;

	if (!image_addr || !layout)
		return -EINVAL;
	if (!image_check_magic(hdr))
		return -ENOEXEC;
	if (!image_check_hcrc(hdr))
		return -EBADMSG;

	payload_size = image_get_data_size(hdr);
	if (!payload_size)
		return -EINVAL;

	signed_size = (u32)sizeof(image_header_t) + payload_size;

	memset(layout, 0, sizeof(*layout));
	layout->hdr = hdr;
	layout->image_addr = image_addr;
	layout->payload_addr = image_addr + sizeof(image_header_t);
	layout->payload_size = payload_size;
	layout->sig_addr = image_addr + signed_size;
	layout->sig_size = VERITY_TRUST_KERNEL_SIG_SIZE;
	layout->signed_size = signed_size;
	layout->total_size = signed_size + VERITY_TRUST_KERNEL_SIG_SIZE;

	return 0;
}

int verity_trust_load_kernel_image(ulong nor_flash_offset, ulong load_addr,
				     struct verity_trust_kernel_layout *layout,
				     u32 max_size)
{
	char cmd[128];
	struct verity_trust_kernel_layout parsed;
	int ret;

	ret = verity_trust_load_kernel_header(nor_flash_offset, load_addr);
	if (ret)
		return ret;

	ret = verity_trust_parse_kernel_layout(load_addr, &parsed);
	if (ret)
		return ret;
	if (max_size && parsed.total_size > max_size)
		return -EFBIG;

	snprintf(cmd, sizeof(cmd),
		 "sfcnor read %lx %x %lx",
		 nor_flash_offset, parsed.total_size, load_addr);

	ret = run_command(cmd, 0);
	if (ret)
		return -EIO;

	return verity_trust_parse_kernel_layout(load_addr, layout);
}

/* ---- signature verification ---- */

int verity_trust_verify_handle(const struct verity_trust_meta *meta,
				u32 key_handle)
{
	int rc;

	if (!meta || !key_handle)
		return -EINVAL;

	rc = tpm_tis_spi_sigverify_sm2_handle(key_handle,
					      meta->root_hash,
					      meta->signature);
	if (rc) {
		printf("TRUST: TPM_TIS_SPI SM2 sigverify FAILED (err %d)\n", rc);
		return rc;
	}

	printf("TRUST: root_hash SM2 signature OK\n");
	return 0;
}

int verity_trust_verify_policy_nv(const struct verity_trust_meta *meta,
				   u32 nv_index, u32 auth_handle)
{
	int rc;

	if (!meta || !nv_index)
		return -EINVAL;

	rc = tpm_tis_spi_sigverify_sm2_policy_nv(nv_index, auth_handle,
						 meta->root_hash,
						 meta->signature);
	if (rc) {
		printf("TRUST: TPM_TIS_SPI policy NV sigverify FAILED (err %d)\n", rc);
		return rc;
	}

	printf("TRUST: root_hash SM2 policy-NV signature OK\n");
	return 0;
}

int verity_trust_verify_external(const struct verity_trust_meta *meta,
				  const u8 pubkey_xy[64])
{
	int rc;

	if (!meta || !pubkey_xy)
		return -EINVAL;

	rc = tpm_tis_spi_sigverify_sm2_external(meta->root_hash,
						meta->signature,
						pubkey_xy);
	if (rc) {
		printf("TRUST: TPM_TIS_SPI external SM2 sigverify FAILED (err %d)\n", rc);
		return rc;
	}

	printf("TRUST: root_hash SM2 external signature OK\n");
	return 0;
}

int verity_trust_verify_kernel_policy_nv(
		const struct verity_trust_kernel_layout *layout,
		u32 nv_index, u32 auth_handle,
		u8 digest[TPM_TIS_SPI_DIGEST_SIZE])
{
	int rc;

	if (!layout || !digest)
		return -EINVAL;

	rc = tpm_tis_spi_hash_sm3_mem(layout->image_addr,
				     layout->signed_size,
				     digest);
	if (rc) {
		printf("TRUST: kernel SM3 hash FAILED (err %d)\n", rc);
		return rc;
	}

	rc = tpm_tis_spi_sigverify_sm2_policy_nv(nv_index, auth_handle,
						digest,
						(const u8 *)layout->sig_addr);
	if (rc) {
		printf("TRUST: kernel TPM_TIS_SPI policy NV sigverify FAILED (err %d)\n", rc);
		return rc;
	}

	printf("TRUST: kernel SM2 signature OK\n");
	return 0;
}

int verity_trust_get_nv_auth_from_env(u32 *nv_index, u32 *auth_handle)
{
	const char *env;

	if (!nv_index || !auth_handle)
		return -EINVAL;

	env = getenv("tpm_tis_spi_pubkey_nv_index");
	if (env && env[0])
		*nv_index = simple_strtoul(env, NULL, 16);

	env = getenv("tpm_tis_spi_pubkey_nv_auth");
	if (env && env[0]) {
		if (env[0] == 'p' || env[0] == 'P')
			*auth_handle = 0x4000000C;
		else if (env[0] == 'o' || env[0] == 'O')
			*auth_handle = 0x40000001;
		else
			*auth_handle = simple_strtoul(env, NULL, 16);
	}

	return 0;
}

int verity_trust_prepare_rootfs_bootargs(const char *base_bootargs,
				      char *bootargs,
				      unsigned int bootargs_size,
				      struct verity_trust_meta *meta)
{
	ulong flash_offset;
	u32 nv_index = CONFIG_VERITY_TRUST_NV_INDEX;
	u32 auth_handle = CONFIG_VERITY_TRUST_NV_AUTH;
	int ret;

	if (!base_bootargs || !bootargs || !bootargs_size || !meta)
		return -EINVAL;

	memset(meta, 0, sizeof(*meta));
	memset(bootargs, 0, bootargs_size);

	flash_offset = CONFIG_VERITY_TRUST_ROOTFS_OFFSET +
		       CONFIG_VERITY_TRUST_METADATA_OFFSET;
	printf("VERITY-ROOTFS: reading metadata @ NOR 0x%lx\n", flash_offset);

	ret = verity_trust_load_meta(flash_offset, meta);
	if (ret)
		return ret;

	printf("VERITY-ROOTFS: data_blocks=%u hash_blocks=%u\n",
	       meta->data_blocks, meta->hash_blocks);

	verity_trust_get_nv_auth_from_env(&nv_index, &auth_handle);
	printf("VERIFY-ROOTFS: TPM_TIS_SPI NV=0x%x auth=0x%x\n",
	       nv_index, auth_handle);

	ret = verity_trust_verify_policy_nv(meta, nv_index, auth_handle);
	if (ret)
		return ret;

	ret = verity_trust_build_bootargs(base_bootargs, meta,
					   bootargs, bootargs_size);
	if (ret < 0 || (unsigned int)ret >= bootargs_size)
		return -ENOSPC;

	return ret;
}

/*
 * Probe whether TPM_TIS_SPI policy NV has been configured.
 *
 * Return value follows tpm_tis_spi_policy_nv_configured():
 *   TPM_TIS_SPI_POLICY_NV_UNDEFINED  policy NV is explicitly absent
 *   TPM_TIS_SPI_POLICY_NV_CONFIGURED policy NV exists and has a valid pubkey prefix
 *   TPM_TIS_SPI_POLICY_NV_INVALID    policy NV exists but content is invalid
 *   <0                         SPI/TPM/auth/protocol error
 *
 * This function only probes TPM_TIS_SPI policy state.  It must not save U-Boot env:
 * the NOR env area is part of the measured U-Boot range.
 */
int verity_trust_policy_probe(u32 nv_index, u32 auth_handle)
{
	return tpm_tis_spi_policy_nv_configured(nv_index, auth_handle);
}

/*
 * Execute the strict trusted boot path.
 *
 * This verifies the signed kernel first, then verifies rootfs metadata and
 * appends dm-verity bootargs. Any failure aborts boot.
 */
int verity_trust_boot_trusted(u32 nv_index, u32 auth_handle)
{
	struct verity_trust_kernel_layout layout;
	struct verity_trust_meta meta;
	char bootargs[VERITY_TRUST_BOOTARGS_MAX];
	u8 digest[TPM_TIS_SPI_DIGEST_SIZE];
	int ret;

	memset(&layout, 0, sizeof(layout));
	memset(&meta, 0, sizeof(meta));
	memset(bootargs, 0, sizeof(bootargs));
	memset(digest, 0, sizeof(digest));

	printf("TRUSTBOOT: verifying kernel ...\n");
	ret = verity_trust_load_kernel_image(CONFIG_VERITY_TRUST_KERNEL_OFFSET,
					  CONFIG_VERITY_TRUST_KERNEL_LOAD_ADDR,
					  &layout,
					  CONFIG_VERITY_TRUST_KERNEL_MAX_SIZE);
	if (ret) {
		printf("TRUSTBOOT: kernel load FAILED (err %d)\n", ret);
		return CMD_RET_FAILURE;
	}

	ret = verity_trust_verify_kernel_policy_nv(&layout, nv_index,
					   auth_handle, digest);
	if (ret) {
		printf("TRUSTBOOT: kernel verify FAILED\n");
		return CMD_RET_FAILURE;
	}

	printf("TRUSTBOOT: preparing rootfs bootargs ...\n");
	ret = verity_trust_prepare_rootfs_bootargs(CONFIG_VERITY_TRUST_BOOTARGS,
					  bootargs, sizeof(bootargs), &meta);
	if (ret == -ENOSPC) {
		printf("TRUSTBOOT: rootfs bootargs overflow\n");
		return CMD_RET_FAILURE;
	}
	if (ret < 0) {
		printf("TRUSTBOOT: rootfs prepare FAILED (err %d)\n", ret);
		return CMD_RET_FAILURE;
	}

	setenv("bootargs", bootargs);
	printf("TRUSTBOOT: bootargs set (len=%d)\n", ret);
	printf("TRUSTBOOT: booting kernel ...\n");
	return run_command(CONFIG_VERITY_TRUST_BOOTCMD, 0);
}

/*
 * Automatic trustboot entry point used by the U-Boot trustboot command.
 *
 *   - configured policy NV  -> trusted boot
 *   - undefined policy NV   -> abort, require explicit secure provisioning
 *   - invalid/error         -> abort
 */
int verity_trust_auto_boot(void)
{
	u32 nv_index = CONFIG_VERITY_TRUST_NV_INDEX;
	u32 auth_handle = CONFIG_VERITY_TRUST_NV_AUTH;
	int ret;

	verity_trust_get_nv_auth_from_env(&nv_index, &auth_handle);

	ret = verity_trust_policy_probe(nv_index, auth_handle);
	if (ret == TPM_TIS_SPI_POLICY_NV_CONFIGURED)
		return verity_trust_boot_trusted(nv_index, auth_handle);
	if (ret == TPM_TIS_SPI_POLICY_NV_UNDEFINED) {
		printf("TRUSTBOOT: TPM_TIS_SPI policy undefined, abort boot\n");
		printf("TRUSTBOOT: burn trust-provision.pkg, then press reset\n");
		return CMD_RET_FAILURE;
	}
	if (ret == TPM_TIS_SPI_POLICY_NV_INVALID) {
		printf("TRUSTBOOT: TPM_TIS_SPI policy invalid, abort\n");
		printf("TRUSTBOOT: inspect NV state; burn trust-provision.pkg if provisioning is required, then press reset\n");
		return CMD_RET_FAILURE;
	}

	printf("TRUSTBOOT: TPM_TIS_SPI policy probe FAILED (err %d)\n", ret);
	return CMD_RET_FAILURE;
}

/* ---- bootargs assembly ---- */

int verity_trust_build_bootargs(const char *base_bootargs,
				 const struct verity_trust_meta *meta,
				 char *buf, unsigned int buf_size)
{
	char root_hash_hex[65];
	char salt_hex[65];
	unsigned int data_sectors;
	unsigned int hash_start_block;

	if (!base_bootargs || !meta || !buf || !buf_size)
		return -EINVAL;

	bytes_to_hex(meta->root_hash, 32, root_hash_hex, sizeof(root_hash_hex));
	bytes_to_hex(meta->salt,      32, salt_hex, sizeof(salt_hex));

	printf("VERITY-TRUST: root_hash=%s\n", root_hash_hex);

	data_sectors  = meta->data_blocks * (4096u / 512u);
	hash_start_block = meta->data_blocks;

	printf("VERITY-TRUST: data_blocks=%u hash_blocks=%u dm_len=%u hash_start=%u\n",
	       meta->data_blocks, meta->hash_blocks,
	       data_sectors, hash_start_block);

	return snprintf(buf, buf_size,
		"%s dm-mod.create=\"verity-root,,,ro,0 %u verity 1 "
		"%u:%u %u:%u 4096 4096 %u %u sm3 %s %s\"",
		base_bootargs,
		data_sectors,
		CONFIG_VERITY_TRUST_DATA_DEV_MAJOR,
		CONFIG_VERITY_TRUST_DATA_DEV_MINOR,
		CONFIG_VERITY_TRUST_HASH_DEV_MAJOR,
		CONFIG_VERITY_TRUST_HASH_DEV_MINOR,
		meta->data_blocks, hash_start_block,
		root_hash_hex, salt_hex);
}
