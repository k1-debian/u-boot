/*
 * X2000 common routines
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
 * Based on: arch/mips/cpu/xburst/jz4780/jz4780.c
 *           Written by Paul Burton <paul.burton@imgtec.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define DEBUG
#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#include <asm/arch/wdt.h>
#include <spl.h>
#include <regulator.h>
#ifdef CONFIG_AUDIO_CAL_DIV
#include <generated/audio.h>
#endif
#ifdef CONFIG_SPL_BUILD

/* Pointer to as well as the global data structure for SPL */
DECLARE_GLOBAL_DATA_PTR;
gd_t gdata __attribute__ ((section(".data")));

#ifndef CONFIG_BURNER
#include <generated/ddr_reg_values.h>
struct global_info ginfo __attribute__ ((section(".data"))) = {
	.extal		= CONFIG_SYS_EXTAL,
	.cpufreq	= CONFIG_SYS_CPU_FREQ,
	.ddrfreq	= CONFIG_SYS_MEM_FREQ,
	.uart_idx	= CONFIG_SYS_UART_INDEX,
	.baud_rate	= CONFIG_BAUDRATE,

	.ddr_change_param = {
		DDRC_CFG_VALUE,
		DDRC_MMAP0_VALUE,
		DDRC_MMAP1_VALUE,
		DDRC_TIMING4_VALUE,
		DDRC_AUTOSR_EN_VALUE,
		.ddr_remap_array = REMMAP_ARRAY
	}
};
#endif

extern void gpio_init(void);
extern void pll_init(void);
extern void sdram_init(void);
extern void ddr_test_refresh(unsigned int start_addr, unsigned int end_addr);
extern void flush_cache_all(void);


void board_init_f(ulong dummy)
{
	/* Set global data pointer */
	gd = &gdata;

	/* Setup global info */
#ifndef CONFIG_BURNER
	gd->arch.gi = &ginfo;
#else
	burner_param_info();
#endif

	gpio_init();
	/* OST clk gate set 0 */
	cpm_outl(cpm_inl(CPM_CLKGR0) & (~CPM_CLKGR_OST), CPM_CLKGR0);
	/* wtd disable */
	writel(0, WDT_BASE + WDT_TCER);

	cpm_outl(cpm_inl(CPM_MESTSEL) | 0x7, CPM_MESTSEL);

	/* Init uart first */
#ifndef CONFIG_X2000_FPGA
	enable_uart_clk();
#endif

#ifdef CONFIG_SPL_SERIAL_SUPPORT
	preloader_console_init();
#endif
#ifndef CONFIG_X2000_FPGA
	debug("Timer init\n");
	timer_init();

	debug("CLK stop\n");
	clk_prepare();

	debug("PLL init\n");
	pll_init();

	debug("CLK init\n");
	clk_init();
#endif

	debug("SDRAM init\n");
	sdram_init();

#ifdef CONFIG_DDR_AUTO_REFRESH_TEST
	ddr_test_refresh(0xa0000000, 0xa1000000);
#endif


#ifdef CONFIG_DDR_TEST
	ddr_basic_tests();
#endif

#ifndef CONFIG_BURNER
	/* Clear the BSS */
	memset(__bss_start, 0, (char *)&__bss_end - __bss_start);

	debug("board_init_r\n");
	board_init_r(NULL, 0);
#else
#if defined(CONFIG_CMD_BURN) && defined(CONFIG_X2000_V12)
#define EFUSE_STATUS    	0xb3540008
#define BOOT_MODE       	0xb2401800
#define CHANGE_LENGTH   	0xb2400100
#define CHANGE_LENGTH_SIZE      512 * 1024
	writel(readl(EFUSE_STATUS), BOOT_MODE);
	debug("EFUSE_STATUS=%x\n", readl(BOOT_MODE));
	writel(CHANGE_LENGTH_SIZE, CHANGE_LENGTH);
	debug("SPL_SIZE=%x\n", readl(CHANGE_LENGTH));
#endif
	debug("run start1 firmware finished\n");
	return;
#endif
}

#ifdef CONFIG_JZ_SCBOOT
extern int secure_scboot (void *, void *);
static int secure_load_uboot(struct spl_image_info *spl_image)
{
	int ret = secure_scboot ((void *)spl_image->load_addr,
				 (void *)spl_image->entry_point);
	return ret;
}
#endif

void jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void  (*image_entry_noargs_t)(void);

#ifdef CONFIG_JZ_SCBOOT
	flush_cache_all();
	int ret = secure_load_uboot(spl_image);
	if (ret) {
	  printf("Error spl secure load uboot.\n");
	  hang();
	}
	spl_image->entry_point += 2048;
#endif

	debug("image entry point: 0x%x\n", spl_image->entry_point);
	image_entry_noargs_t image_entry =
			(image_entry_noargs_t) spl_image->entry_point;

	flush_cache_all();
	__asm__ volatile (
			".set push              \n\t"
			".set noreorder         \n\t"
			".set mips32r2          \n\t"
			"jr.hb %0              \n\t"
			"nop	\n\t"
			:
			:"r"(image_entry));

}

#endif /* CONFIG_SPL_BUILD */

/*
 * U-Boot common functions
 */

void enable_interrupts(void)
{
}

int disable_interrupts(void)
{
	return 0;
}
