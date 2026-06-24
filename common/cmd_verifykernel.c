/*
 * cmd_verifykernel.c - U-Boot command: verify kernel tail signature via TPM_TIS_SPI
 *
 * Usage: verifykernel
 */
#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <tpm_tis_spi.h>
#include <verity_trust.h>

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

#ifndef CONFIG_VERITY_TRUST_NV_AUTH
#define CONFIG_VERITY_TRUST_NV_AUTH  0x4000000C
#endif

static void print_digest_hex(const u8 digest[TPM_TIS_SPI_DIGEST_SIZE])
{
	int i;

	for (i = 0; i < TPM_TIS_SPI_DIGEST_SIZE; i++)
		printf("%02x", digest[i]);
	printf("\n");
}

static int do_verifykernel(cmd_tbl_t *cmdtp, int flag, int argc,
			   char * const argv[])
{
	struct verity_trust_kernel_layout layout;
	u8 digest[TPM_TIS_SPI_DIGEST_SIZE];
	u32 nv_index = CONFIG_VERITY_TRUST_NV_INDEX;
	u32 auth_handle = CONFIG_VERITY_TRUST_NV_AUTH;
	const char *env;
	int ret;

	(void)cmdtp;
	(void)flag;
	(void)argc;
	(void)argv;

	memset(&layout, 0, sizeof(layout));
	memset(digest, 0, sizeof(digest));

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

	printf("VERIFY-KERNEL: reading header @ NOR 0x%x\n",
	       CONFIG_VERITY_TRUST_KERNEL_OFFSET);
	ret = verity_trust_load_kernel_image(CONFIG_VERITY_TRUST_KERNEL_OFFSET,
					  CONFIG_VERITY_TRUST_KERNEL_LOAD_ADDR,
					  &layout,
					  CONFIG_VERITY_TRUST_KERNEL_MAX_SIZE);
	if (ret) {
		printf("VERIFY-KERNEL: image load FAILED (err %d)\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("VERIFY-KERNEL: ih_size=%u signed_size=%u total_size=%u\n",
	       layout.payload_size, layout.signed_size, layout.total_size);
	printf("VERIFY-KERNEL: image_addr=0x%lx sig_addr=0x%lx sig_size=%u\n",
	       layout.image_addr, layout.sig_addr, layout.sig_size);
	printf("VERIFY-KERNEL: TPM_TIS_SPI NV=0x%x auth=0x%x\n",
	       nv_index, auth_handle);

	ret = verity_trust_verify_kernel_policy_nv(&layout, nv_index,
					   auth_handle, digest);
	if (ret) {
		printf("VERIFY-KERNEL: verify FAILED — ABORT\n");
		return CMD_RET_FAILURE;
	}

	printf("VERIFY-KERNEL: SM3(image-with-header)=");
	print_digest_hex(digest);
	printf("VERIFY-KERNEL: verify OK\n");
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	verifykernel, 1, 0, do_verifykernel,
	"verify kernel image tail signature via TPM_TIS_SPI",
	"verifykernel"
);
