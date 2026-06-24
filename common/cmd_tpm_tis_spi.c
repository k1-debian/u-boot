/*
 * TPM_TIS_SPI helper command for bring-up and verification diagnostics.
 */

#include <common.h>
#include <asm-generic/errno.h>
#include <command.h>
#include <environment.h>
#include <linux/string.h>

#include <tpm_tis_spi.h>

#define TPM_TIS_SPI_ENV_SIG_KEY_HANDLE "tpm_tis_spi_sig_key_handle"
#define TPM_TIS_SPI_ENV_PUBKEY_NV_INDEX "tpm_tis_spi_pubkey_nv_index"
#define TPM_TIS_SPI_ENV_PUBKEY_NV_AUTH "tpm_tis_spi_pubkey_nv_auth"
#define TPM_TIS_SPI_DEFAULT_PUBKEY_NV_INDEX 0x013fffee
#define TPM_TIS_SPI_RH_OWNER 0x40000001
#define TPM_TIS_SPI_RH_PLATFORM 0x4000000c

static int tpm_tis_spi_cmd_hex_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/*
 * 功能：解析固定长度十六进制字符串，避免签名/公钥长度错误被静默接受。
 */
static int tpm_tis_spi_cmd_parse_hex_exact(const char *hex, u8 *out,
				     unsigned int out_len)
{
	unsigned int i;
	int hi;
	int lo;

	if (!hex || !out)
		return -EINVAL;

	for (i = 0; i < out_len; i++) {
		hi = tpm_tis_spi_cmd_hex_val(hex[i * 2]);
		lo = tpm_tis_spi_cmd_hex_val(hex[i * 2 + 1]);
		if (hi < 0 || lo < 0)
			return -EINVAL;
		out[i] = (u8)((hi << 4) | lo);
	}

	if (hex[out_len * 2] != '\0')
		return -EINVAL;

	return 0;
}

/*
 * 功能：解析外部 SM2 公钥，支持 X||Y(64B) 或 04||X||Y(65B) 两种格式。
 */
static int tpm_tis_spi_cmd_parse_pubkey_xy(const char *hex,
				     u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE])
{
	unsigned int hex_len;

	if (!hex)
		return -EINVAL;

	hex_len = strlen(hex);
	if (hex_len == TPM_TIS_SPI_PUBKEY_XY_SIZE * 2)
		return tpm_tis_spi_cmd_parse_hex_exact(hex, pubkey_xy,
						     TPM_TIS_SPI_PUBKEY_XY_SIZE);

	if (hex_len == (TPM_TIS_SPI_PUBKEY_XY_SIZE + 1) * 2) {
		if (hex[0] != '0' || hex[1] != '4')
			return -EINVAL;
		return tpm_tis_spi_cmd_parse_hex_exact(hex + 2, pubkey_xy,
						     TPM_TIS_SPI_PUBKEY_XY_SIZE);
	}

	return -EINVAL;
}

static int tpm_tis_spi_cmd_parse_digest_sig(const char *digest_hex,
				       const char *sig_hex,
				       u8 digest[TPM_TIS_SPI_DIGEST_SIZE],
				       u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE])
{
	int rc;

	rc = tpm_tis_spi_cmd_parse_hex_exact(digest_hex, digest, TPM_TIS_SPI_DIGEST_SIZE);
	if (rc) {
		printf("TPM_TIS_SPI: bad digest, need 64 hex chars\n");
		return rc;
	}

	/* 签名格式为 r||s，各 32 字节，大端顺序。 */
	rc = tpm_tis_spi_cmd_parse_hex_exact(sig_hex, signature,
				       TPM_TIS_SPI_SIGNATURE_SIZE);
	if (rc)
		printf("TPM_TIS_SPI: bad signature, need 128 hex chars r||s\n");

	return rc;
}

/*
 * 功能：取得策略 NV index，默认使用 0x013fffee，可由环境变量或命令参数覆盖。
 */
static u32 tpm_tis_spi_cmd_get_pubkey_nv_index(const char *arg)
{
	const char *env;

	if (arg && arg[0])
		return simple_strtoul(arg, NULL, 16);

	env = getenv(TPM_TIS_SPI_ENV_PUBKEY_NV_INDEX);
	if (env && env[0])
		return simple_strtoul(env, NULL, 16);

	return TPM_TIS_SPI_DEFAULT_PUBKEY_NV_INDEX;
}

/*
 * 功能：取得 NV_Read 的授权层级，默认 platform；环境变量支持 p/o/数值 handle。
 */
static u32 tpm_tis_spi_cmd_get_pubkey_nv_auth(void)
{
	const char *env = getenv(TPM_TIS_SPI_ENV_PUBKEY_NV_AUTH);

	if (!env || !env[0] || env[0] == 'p' || env[0] == 'P')
		return TPM_TIS_SPI_RH_PLATFORM;
	if (env[0] == 'o' || env[0] == 'O')
		return TPM_TIS_SPI_RH_OWNER;

	return simple_strtoul(env, NULL, 16);
}

static int do_tpm_tis_spi(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong addr;
	ulong len;
	u8 digest[TPM_TIS_SPI_DIGEST_SIZE];
	u8 signature[TPM_TIS_SPI_SIGNATURE_SIZE];
	u8 pubkey_xy[TPM_TIS_SPI_PUBKEY_XY_SIZE];
	u32 key_handle;
	u32 nv_index;
	u32 auth_handle;
	u32 did_vid;
	u8 rid;
	const char *handle_env;
	int rc;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "probe")) {
		rc = tpm_tis_spi_probe(&did_vid, &rid);
		if (rc) {
			printf("tpm_tis_spi probe failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI DID_VID=0x%08x RID=0x%02x\n", did_vid, rid);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "hash")) {
		if (argc != 4)
			return CMD_RET_USAGE;
		addr = simple_strtoul(argv[2], NULL, 16);
		len = simple_strtoul(argv[3], NULL, 16);
		rc = tpm_tis_spi_hash_sm3_mem(addr, len, digest);
		if (rc) {
			printf("tpm_tis_spi hash failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI SM3: ");
		tpm_tis_spi_print_digest(digest);
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "verify")) {
		if (argc != 5)
			return CMD_RET_USAGE;
		addr = simple_strtoul(argv[2], NULL, 16);
		len = simple_strtoul(argv[3], NULL, 16);
		rc = tpm_tis_spi_verify_mem_with_hex(addr, len, argv[4]);
		if (rc) {
			printf("tpm_tis_spi verify failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI verify success\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "verifyenv")) {
		if (argc != 4 && argc != 5)
			return CMD_RET_USAGE;
		addr = simple_strtoul(argv[2], NULL, 16);
		len = simple_strtoul(argv[3], NULL, 16);
		rc = tpm_tis_spi_verify_mem_with_env(addr, len,
						    (argc == 5) ? argv[4] : NULL);
		if (rc) {
			printf("tpm_tis_spi verifyenv failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI verifyenv success\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "sigverify")) {
		if (argc != 4)
			return CMD_RET_USAGE;

		handle_env = getenv(TPM_TIS_SPI_ENV_SIG_KEY_HANDLE);
		if (!handle_env || !handle_env[0]) {
			printf("TPM_TIS_SPI: missing env %s\n", TPM_TIS_SPI_ENV_SIG_KEY_HANDLE);
			return CMD_RET_FAILURE;
		}

		key_handle = simple_strtoul(handle_env, NULL, 16);
		if (!key_handle) {
			printf("TPM_TIS_SPI: bad %s value\n", TPM_TIS_SPI_ENV_SIG_KEY_HANDLE);
			return CMD_RET_FAILURE;
		}

		rc = tpm_tis_spi_cmd_parse_digest_sig(argv[2], argv[3], digest, signature);
		if (rc)
			return CMD_RET_FAILURE;

		rc = tpm_tis_spi_sigverify_sm2_handle(key_handle, digest, signature);
		if (rc) {
			printf("tpm_tis_spi sigverify failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI sigverify success\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "sigverifyh")) {
		if (argc != 5)
			return CMD_RET_USAGE;

		key_handle = simple_strtoul(argv[2], NULL, 16);
		if (!key_handle) {
			printf("TPM_TIS_SPI: bad key_handle\n");
			return CMD_RET_FAILURE;
		}

		rc = tpm_tis_spi_cmd_parse_digest_sig(argv[3], argv[4], digest, signature);
		if (rc)
			return CMD_RET_FAILURE;

		rc = tpm_tis_spi_sigverify_sm2_handle(key_handle, digest, signature);
		if (rc) {
			printf("tpm_tis_spi sigverifyh failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI sigverifyh success\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "sigverifynv")) {
		if (argc != 4 && argc != 5)
			return CMD_RET_USAGE;

		rc = tpm_tis_spi_cmd_parse_digest_sig(argv[2], argv[3], digest, signature);
		if (rc)
			return CMD_RET_FAILURE;

		nv_index = tpm_tis_spi_cmd_get_pubkey_nv_index((argc == 5) ? argv[4] : NULL);
		auth_handle = tpm_tis_spi_cmd_get_pubkey_nv_auth();
		if (!nv_index || !auth_handle) {
			printf("TPM_TIS_SPI: bad NV index/auth handle\n");
			return CMD_RET_FAILURE;
		}

		rc = tpm_tis_spi_sigverify_sm2_policy_nv(nv_index, auth_handle,
							 digest, signature);
		if (rc) {
			printf("tpm_tis_spi sigverifynv failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI sigverifynv success\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "sigverifyext")) {
		if (argc != 5)
			return CMD_RET_USAGE;

		rc = tpm_tis_spi_cmd_parse_digest_sig(argv[2], argv[3], digest, signature);
		if (rc)
			return CMD_RET_FAILURE;

		rc = tpm_tis_spi_cmd_parse_pubkey_xy(argv[4], pubkey_xy);
		if (rc) {
			printf("TPM_TIS_SPI: bad pubkey, need X||Y(128 hex) or 04||X||Y(130 hex)\n");
			return CMD_RET_FAILURE;
		}

		rc = tpm_tis_spi_sigverify_sm2_external(digest, signature, pubkey_xy);
		if (rc) {
			printf("tpm_tis_spi sigverifyext failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		printf("TPM_TIS_SPI sigverifyext success\n");
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	tpm_tis_spi,	5,	1,	do_tpm_tis_spi,
	"TPM_TIS_SPI TPM2-over-SPI helper commands",
	"probe\n"
	"tpm_tis_spi hash <addr> <len>\n"
	"tpm_tis_spi verify <addr> <len> <digest_hex_64>\n"
	"tpm_tis_spi verifyenv <addr> <len> [env_name]\n"
	"tpm_tis_spi sigverify <digest_hex_64> <sig_hex_128>\n"
	"    use env tpm_tis_spi_sig_key_handle as internal/persistent key handle\n"
	"tpm_tis_spi sigverifyh <key_handle> <digest_hex_64> <sig_hex_128>\n"
	"tpm_tis_spi sigverifynv <digest_hex_64> <sig_hex_128> [nv_index]\n"
	"    read pubkey[65] from policy NV, env tpm_tis_spi_pubkey_nv_auth defaults to p\n"
	"tpm_tis_spi sigverifyext <digest_hex_64> <sig_hex_128> <pubkey_hex_128_or_130>"
);
