/*
 * cmd_verityrootfs.c - U-Boot command: verify rootfs via TPM_TIS_SPI + dm-verity
 *
 * Usage: verityrootfs
 *
 * Flow:
 *   1. Load  160B metadata from NOR flash (root_hash, salt, sig, ...)
 *   2. Call  TPM_TIS_SPI policy-NV verify(root_hash, signature)
 *   3. Build final bootargs with dm-mod.create appended
 *   4. Boot  kernel from NOR flash
 */
#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <tpm_tis_spi.h>
#include <verity_trust.h>

/* ---- compile-time config (p109_printer.h) ---- */
#ifndef CONFIG_VERITY_TRUST_ROOTFS_OFFSET
#error "CONFIG_VERITY_TRUST_ROOTFS_OFFSET must be defined"
#endif
#ifndef CONFIG_VERITY_TRUST_METADATA_OFFSET
#error "CONFIG_VERITY_TRUST_METADATA_OFFSET must be defined"
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

/* default auth: platform authorization (TPM2_RH_PLATFORM = 0x4000000C) */
#ifndef CONFIG_VERITY_TRUST_NV_AUTH
#define CONFIG_VERITY_TRUST_NV_AUTH  0x4000000C
#endif

static int do_verityrootfs(cmd_tbl_t *cmdtp, int flag, int argc,
			    char * const argv[])
{
	struct verity_trust_meta meta;
	ulong flash_offset;
	char bootargs[VERITY_TRUST_BOOTARGS_MAX];
	u32 nv_index = CONFIG_VERITY_TRUST_NV_INDEX;
	u32 auth_handle = CONFIG_VERITY_TRUST_NV_AUTH;
	const char *env;
	int ret;

	(void)cmdtp;
	(void)flag;
	(void)argc;
	(void)argv;

	memset(&meta, 0, sizeof(meta));
	memset(bootargs, 0, sizeof(bootargs));

	/* ---- Phase 1: Load metadata from NOR ---- */
	flash_offset = CONFIG_VERITY_TRUST_ROOTFS_OFFSET +
		       CONFIG_VERITY_TRUST_METADATA_OFFSET;

	printf("VERITY-ROOTFS: reading metadata @ NOR 0x%lx\n", flash_offset);
	ret = verity_trust_load_meta(flash_offset, &meta);
	if (ret) {
		printf("VERITY-ROOTFS: metadata read FAILED (err %d)\n", ret);
		return CMD_RET_FAILURE;
	}
	printf("VERITY-ROOTFS: data_blocks=%u hash_blocks=%u\n",
	       meta.data_blocks, meta.hash_blocks);

	/* ---- Phase 2: TPM_TIS_SPI policy-NV SM2 sigverify ----
	 *
	 * Pubkey is stored inside TPM_TIS_SPI NV (written by gen-policy-bin.sh).
	 * No U-Boot env needed — key never leaves the chip.
	 *
	 * Env overrides (optional):
	 *   tpm_tis_spi_pubkey_nv_index  — override NV index
	 *   tpm_tis_spi_pubkey_nv_auth   — override auth ('p'=platform, 'o'=owner, or hex)
	 */
	env = getenv("tpm_tis_spi_pubkey_nv_index");
	if (env && env[0])
		nv_index = simple_strtoul(env, NULL, 16);

	env = getenv("tpm_tis_spi_pubkey_nv_auth");
	if (env && env[0]) {
		if (env[0] == 'p' || env[0] == 'P')
			auth_handle = 0x4000000C;
		else if (env[0] == 'o' || env[0] == 'O')
			auth_handle = 0x40000001;
		else
			auth_handle = simple_strtoul(env, NULL, 16);
	}

	printf("VERIFY-ROOTFS: TPM_TIS_SPI NV=0x%x auth=0x%x\n",
	       nv_index, auth_handle);
	ret = verity_trust_verify_policy_nv(&meta, nv_index, auth_handle);
	if (ret) {
		printf("VERIFY-ROOTFS: TPM_TIS_SPI verify FAILED — ABORT BOOT\n");
		return CMD_RET_FAILURE;
	}

	/* ---- Phase 3: Build dm-mod.create bootargs ---- */
	ret = verity_trust_build_bootargs(CONFIG_VERITY_TRUST_BOOTARGS,
					   &meta, bootargs, sizeof(bootargs));
	if (ret < 0 || (unsigned int)ret >= sizeof(bootargs)) {
		printf("VERIFY-ROOTFS: bootargs overflow (len %d)\n", ret);
		return CMD_RET_FAILURE;
	}
	setenv("bootargs", bootargs);
	printf("VERIFY-ROOTFS: bootargs set (len=%d)\n", ret);

	/* ---- Phase 4: Boot kernel ---- */
	printf("VERIFY-ROOTFS: booting kernel ...\n");
	return run_command(CONFIG_VERITY_TRUST_BOOTCMD, 0);
}

U_BOOT_CMD(
	verityrootfs, 1, 0, do_verityrootfs,
	"TPM_TIS_SPI policy-NV verify rootfs and boot via dm-verity",
	"verityrootfs"
);
