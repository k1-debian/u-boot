/*
 * cmd_trustboot.c - U-Boot command: verify kernel + rootfs and boot
 *
 * Usage: trustboot
 */
#include <common.h>
#include <command.h>
#include <verity_trust.h>

static int do_trustboot(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	(void)cmdtp;
	(void)flag;
	(void)argc;
	(void)argv;

	return verity_trust_auto_boot();
}

U_BOOT_CMD(
	trustboot, 1, 0, do_trustboot,
	"verify kernel + rootfs and boot",
	"trustboot"
);
