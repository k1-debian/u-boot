#include <common.h>
#include <command.h>
#include <asm/usb_boot.h>
#include <asm/reboot.h>
#include <asm/io.h>
#include <asm/arch/wdt.h>
#include <asm/arch/cpm.h>

void __attribute__((weak)) _machine_restart(void)
{
	int time = RTC_FREQ / WDT_DIV * RESET_DELAY_MS / 1000;

	if(time > 65535)
		time = 65535;

	writel(TSCR_WDTSC, TCU_BASE + TCU_TSCR);

	writel(0, WDT_BASE + WDT_TCNT);
	writel(time, WDT_BASE + WDT_TDR);
	writel(TCSR_PRESCALE | TCSR_RTC_EN
#if (defined(CONFIG_X1600))
			| TCSR_CLRZ
#endif
			, WDT_BASE + WDT_TCSR);
	writel(0,WDT_BASE + WDT_TCER);

	serial_debug("reset in %dms", RESET_DELAY_MS);
	writel(TCER_TCEN,WDT_BASE + WDT_TCER);
	mdelay(1000);
}

void enter_usb_boot_mode(void)
{
#if defined(CPM_SLPC)
#if defined(CONFIG_X1000) || defined(CONFIG_M200)
	unsigned int val = 'b' << 24 | 'u' << 16 | 'r' << 8 | 'n';
#else
#define SLPC_SW_MAGIC           0x425753 // SWB (software boot)
#define SLPC_SW_USB_BOOT        (0x2 << 0)
	unsigned int val = SLPC_SW_MAGIC << 8 | SLPC_SW_USB_BOOT;
#endif

	cpm_outl(val, CPM_SLPC);

	_machine_restart();
#else
        printf("Not support soft burn!\n");
#endif
}


