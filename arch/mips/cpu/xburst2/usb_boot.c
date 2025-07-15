#include <common.h>
#include <command.h>
#include <asm/usb_boot.h>
#include <asm/reboot.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>

void enter_usb_boot_mode(void)
{
#if defined(CPM_SLPC) || defined(CPM_SOFT_APPR)
#define SLPC_SW_MAGIC           0x425753 // SWB (software boot)
#define SLPC_SW_USB_BOOT        (0x2 << 0)

	unsigned int val = SLPC_SW_MAGIC << 8 | SLPC_SW_USB_BOOT;
#if defined(CPM_SLPC)
	cpm_outl(val, CPM_SLPC);
#elif defined(CPM_SOFT_APPR)
	cpm_outl(val, CPM_SOFT_APPR);
#endif
	_machine_restart();
#else
        printf("Not support soft burn!\n");
#endif
}
