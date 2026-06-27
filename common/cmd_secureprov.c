/*
 * U-Boot command entry for secure provisioning.
 */

#include <common.h>
#include <asm-generic/errno.h>
#include <command.h>
#include <linux/string.h>

#include <secure_provision.h>

static int secureprov_parse_ulong(const char *arg, int base, ulong *value)
{
	char *endp;

	if (!arg || !arg[0] || !value)
		return -EINVAL;
	if (arg[0] == '-' || arg[0] == '+')
		return -EINVAL;

	*value = simple_strtoul(arg, &endp, base);
	if (endp == arg || *endp)
		return -EINVAL;

	return 0;
}

static int secureprov_parse_flags(int argc, char * const argv[], int first,
				  u32 *flags)
{
	int i;

	if (!flags)
		return -EINVAL;

	*flags = 0;
	for (i = first; i < argc; i++) {
		if (!strcmp(argv[i], "force")) {
			*flags |= SECURE_PROVISION_F_FORCE;
			continue;
		}

		printf("secureprov: unknown option '%s'\n", argv[i]);
		return -EINVAL;
	}

	return 0;
}

static int do_secureprov(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	ulong policy_addr;
	ulong cert_addr;
	ulong policy_len;
	ulong cert_len;
	u32 flags;
	int rc;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "status")) {
		rc = secure_provision_status();
		return (rc && rc != -ENOENT) ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "embedded")) {
		if (secureprov_parse_flags(argc, argv, 2, &flags))
			return CMD_RET_USAGE;

		rc = secure_provision_apply_embedded(flags);
		if (rc) {
			printf("secureprov embedded failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "mem")) {
		if (argc < 6)
			return CMD_RET_USAGE;

		if (secureprov_parse_ulong(argv[2], 16, &policy_addr) ||
		    secureprov_parse_ulong(argv[3], 0, &policy_len) ||
		    secureprov_parse_ulong(argv[4], 16, &cert_addr) ||
		    secureprov_parse_ulong(argv[5], 0, &cert_len))
			return CMD_RET_USAGE;

		if (secureprov_parse_flags(argc, argv, 6, &flags))
			return CMD_RET_USAGE;

		if (!policy_addr || !cert_addr ||
		    policy_len != SECURE_PROVISION_POLICY_NV_SIZE ||
		    !cert_len || cert_len > SECURE_PROVISION_CERT_NV_SIZE) {
			printf("secureprov: invalid policy/cert address or length\n");
			return CMD_RET_USAGE;
		}

		rc = secure_provision_apply_buffers((const u8 *)policy_addr,
						    (u16)policy_len,
						    (const u8 *)cert_addr,
						    (u16)cert_len,
						    flags);
		if (rc) {
			printf("secureprov mem failed: %d\n", rc);
			return CMD_RET_FAILURE;
		}
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	secureprov, 7, 1, do_secureprov,
	"program secure boot policy material into TPM2 NV",
	"status\n"
	"secureprov embedded [force]\n"
	"secureprov mem <policy_addr> <policy_len> <cert_addr> <cert_len> [force]"
);
