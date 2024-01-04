#include <common.h>
#include <command.h>

static int do_usb_load(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long addr, offset;
	int len;
	const char *stage;

	if (argc < 2 || argc > 5){
		printf("Usage: usbload [address] [offset] [length] [cmd]\n");
		return CMD_RET_USAGE;
	}

	if (argc >= 4) {
		addr = simple_strtoul(argv[1], NULL, 16);
		offset = simple_strtoul(argv[2], NULL, 16);
		len = simple_strtoul(argv[3], NULL, 10);
		stage = argv[4];
	}

	if(strncmp(stage, "initddr", 7) == 0){
		printf("Load SPL from 0x%x and run from 0x%x by Initialize DDR.\n",addr, offset);
		usb_load_run_stage1_firmware(addr, offset, len);
	}else if(strncmp(stage, "load", 4) == 0){
		printf("Load firmware from 0x%x and run from 0x%x.\n",addr, offset);
		usb_load_run_stage2_firmware(addr, offset, len);
	}
	return 0;
}

U_BOOT_CMD(
	usbload, 5, 0, do_usb_load,
	"load binary data from usb host.",
	"usbload [address] [offset] [length] [cmd].\n"
	"	-[address] usb host source address.\n"
	"	-[offset] usb device target address.\n"
	"	-[length] gives the size(bytes) to load.\n"
	"	-[cmd]	'initddr':load and run spl for init ddr.\n"
	"		'load':load and run firmware."
);
