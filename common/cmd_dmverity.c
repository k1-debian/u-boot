#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <dm_verity_boot.h>

__weak int board_dm_verity_get_config(struct dm_verity_boot_config *out)
{
	if (!out)
		return -EINVAL;

	(void)out;
	return -ENOSYS;
}

static int do_dmverity(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	struct dm_verity_boot_config cfg;
	int ret;

	(void)cmdtp;
	(void)flag;
	(void)argc;
	(void)argv;

	memset(&cfg, 0, sizeof(cfg));

	ret = board_dm_verity_get_config(&cfg);
	if (ret) {
		printf("dmverity: board config failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	ret = dm_verity_boot_run(&cfg);
	if (ret) {
		printf("dmverity: boot failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	dmverity, 1, 0, do_dmverity,
	"verify rootfs metadata and boot kernel",
	"dmverity"
);
