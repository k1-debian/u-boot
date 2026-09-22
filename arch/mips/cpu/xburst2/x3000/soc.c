/*
 * X3000 common routines
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

//#define DEBUG
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

/* X3000 exposes logical UART0-UART7 through two non-contiguous address banks. */
static const unsigned int x3000_uart_base_table[] = {
	UART0_BASE, UART1_BASE, UART2_BASE, UART3_BASE,
	UART4_BASE, UART5_BASE, UART6_BASE, UART7_BASE,
};

unsigned int jz_uart_get_base(unsigned int uart_idx)
{
	if (uart_idx >= ARRAY_SIZE(x3000_uart_base_table))
		uart_idx = 0;

	return x3000_uart_base_table[uart_idx];
}

void reallocate_cache(void)
{
	flush_cache_all();

	/* allcate L2 cache size */
	/***********************************
	  L2 cache size
	  reg addr: 0x12200060
	  bit   12 11 10
	  0   0  0   L2C=0KB
	  0   0  1   L2C=128KB
	  0   1  0   L2C=256KB
	  0   1  1   L2C=512KB
	  1   0  0   L2C=1024KB
	 ***********************************/
	/* wait l2cache alloc ok */
	__asm__ volatile(
			".set push     \n\t"
			".set mips32r2 \n\t"
			"sync          \n\t"
			"lw $0,0(%0)   \n\t"
			".set pop      \n\t"
			::"r" (0xa0000000));
	*((volatile unsigned int *)(0xb2200060)) = 0x00000400;
	__asm__ volatile(
			".set push     \n\t"
			".set mips32r2 \n\t"
			"sync          \n\t"
			"lw $0,0(%0)   \n\t"
			".set pop      \n\t"
			::"r" (0xa0000000));
}


#ifdef CONFIG_SPL_BUILD

/* Pointer to as well as the global data structure for SPL */
DECLARE_GLOBAL_DATA_PTR;
gd_t gdata __attribute__ ((section(".data")));

#ifndef CONFIG_BURNER
struct global_info ginfo __attribute__ ((section(".data"))) = {
	.extal		= CONFIG_SYS_EXTAL,
	.cpufreq	= CONFIG_SYS_CPU_FREQ,
	.ddrfreq	= CONFIG_SYS_MEM_FREQ,
	.uart_idx	= CONFIG_SYS_UART_INDEX,
	.baud_rate	= CONFIG_BAUDRATE,
};
#endif

extern void gpio_init(void);
extern void pll_init(void);
extern void sdram_init(void);
extern void ddr_test_refresh(unsigned int start_addr, unsigned int end_addr);
extern void flush_cache_all(void);
#ifdef CONFIG_DDR_DEBUG_STUB
extern void debug_stub_run(int phase);
#endif

#ifdef CONFIG_BURNER
extern void burner_param_info(void);
#endif

static void x3000_wdt_disable(void)
{
	writel(0, WDT_BASE + WDT_TCER);
}

#ifndef CONFIG_SPL
void board_init_f(ulong dummy) {}
#else
void board_init_f(ulong dummy)
{
	/* Disable the watchdog before a burner waits for USB parameters. */
	x3000_wdt_disable();

	/* Set global data pointer */
	gd = &gdata;

	/* Setup global info */
#ifndef CONFIG_BURNER
	gd->arch.gi = &ginfo;
#ifdef CONFIG_USE_GLOBAL_SHARED_PARAMS
	unsigned int global_params_addr = CONFIG_SPL_TEXT_BASE +
		CONFIG_GLOBAL_PARAMS_OFFSET;
	gd->arch.gp = (struct global_shared_params *)global_params_addr;
	ginfo_set(gd->arch.gi, gd->arch.gp);
#endif
#else
	burner_param_info();
#endif

	gpio_init();

	/* Init uart first */
	enable_uart_clk();

#ifdef CONFIG_SPL_SERIAL_SUPPORT
	preloader_console_init();
#endif

	ingenic_setup_voltage_config();

	serial_debug("ERROR EPC %x\n", read_c0_errorepc());
	serial_debug("Reset status %x\n", readl(CPM_BASE + CPM_CPCSR));

	// TODO: timer 需要时钟，这里全部打开不合适.
	*(volatile unsigned int *)0xb0000160 = 0xffffffff;
	*(volatile unsigned int *)0xb0000164 = 0xffffffff;
	*(volatile unsigned int *)0xb0000168 = 0xffffffff;
	debug("Timer init\n");
	timer_init();

#ifdef CONFIG_DDR_DEBUG_STUB
	/*
	 * DDR/时钟在线调试 stub —— **阶段 1**（默认关闭，见 CONFIG_DDR_DEBUG_STUB）：
	 * UART 已可用、但时钟与 DDR 参数都还没生效 —— 这里**只协商参数**
	 * （收 SETALL/PARAM 改 PLL 分频与 DDR 参数），改完 return 回来，
	 * 下面的 pll_init()/clk_init()/sdram_init() 就用新值
	 * —— **参数是这样生效的**（用户 2026-09-21 明确的设计）。
	 *
	 * ⛔ 阶段 1 里**不能**跑 DDR 初始化或自检：`pll_init()` 还没跑，
	 *    DDR 时钟根本没起来（旧代码在这儿调 `sdram_init()` 必然
	 *    `wait clk 0 timeout`）。读写测试放在 `sdram_init()` 之后的
	 *    **阶段 2**（见下面）。
	 *
	 * ⭐ **必须在 `timer_init()` 之后**（用户 2026-09-21 明确要求："需要在 soc.c
	 *    里面，将 debug_run 放到 timer_init 之后"）：stub 的"等主机"超时现在用
	 *    `get_ticks()`（OST 计数器，见 debug_stub.c 的 `DS_IDLE_MS_*`）判定，
	 *    而 OST 是 `timer_init()` 配好的 —— 放在它前面 `get_ticks()` 没意义。
	 *    ⚠️ 这个位置**不破坏**"改 PLL 前协商参数"的语义：此时 PLL 还没动，
	 *       OST 的 tick 就是 `timer_init()` 按 EXTAL 配的那个 ⇒ 阶段 1 的
	 *       500ms **是准的**。
	 *    ⚠️ 与其它 SoC 的顺序也一致（uart → timer_init → clk_prepare → pll_init，
	 *       见 x2000/x2100/x2500/x2600 的 soc.c）；`x2600/bootroom_help.c` 就是
	 *       `timer_init(); x2600_bootroom_help_run();` 的写法。
	 * 不接上位机时它自己会超时，照默认参数正常启动（不会变砖）。
	 */
	debug_stub_run(1);
#endif

#ifndef CONFIG_BURNER
	debug("CLK stop\n");
	clk_prepare();
#endif
	debug("PLL init\n");
	pll_init();

	debug("CLK init\n");
	clk_init();

	*(volatile unsigned int *)0xb0000160 = 0xffffffff;
	*(volatile unsigned int *)0xb0000164 = 0xffffffff;
	*(volatile unsigned int *)0xb0000168 = 0xffffffff;

#ifdef CONFIG_HW_WATCHDOG
	debug("WATCHDOG init\n");
	hw_watchdog_init();
#endif

	debug("SDRAM init\n");
	sdram_init();

#ifdef CONFIG_DDR_DEBUG_STUB
	/*
	 * DDR/时钟在线调试 stub —— **阶段 2**（用户 2026-09-21 的设计）：
	 * 此时 DDR 已经按阶段 1 收到的参数初始化完了，所以这里能**真做**
	 * 读写测试，把"这组参数到底行不行"的初步结论回报给上位机
	 * （DDR 初始化卡死的话根本走不到这里 —— 那本身就是结论）。
	 * 拿到结论后 return 回来继续正常启动；U-Boot / kernel 能否启动
	 * 由上位机看串口日志判定。
	 *
	 * ⚠️ 必须在下面"清 BSS"之前：阶段 1 的 static 状态（s_want_selftest …）
	 *    在 BSS 里，清掉就没了。
	 */
	debug_stub_run(2);
#endif

#ifdef CONFIG_DDR_AUTO_REFRESH_TEST
	ddr_test_refresh(0xa0000000, 0xa1000000);
#endif

#ifdef CONFIG_DDR_TEST
	ddr_basic_tests();
#endif

#ifndef CONFIG_BURNER
	reallocate_cache();

	debug("board_init_r\n");
	board_init_r(NULL, 0);
#else
	debug("run start1 firmware finished\n");
#endif
}
#endif

#ifdef CONFIG_JZ_SECURE_SUPPORT
extern int secure_scboot(void *, void *);
#endif

void jump_to_entry_point(unsigned long entry_point)
{
	flush_cache_all();
	__asm__ volatile (
			".set push              \n\t"
			".set noreorder         \n\t"
			".set mips32r2          \n\t"
			"jr.hb %0               \n\t"
			"nop                     \n\t"
			:
			: "r"(entry_point));
}

void jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void (*image_entry_noargs_t)(void);
	image_entry_noargs_t image_entry;

#ifdef CONFIG_JZ_SECURE_SUPPORT
	int ret;

	flush_cache_all();
	spl_image->entry_point += 2048;
	ret = secure_scboot((void *)spl_image->load_addr,
			    (void *)spl_image->entry_point);
	if (ret) {
		serial_debug("SCBOOT: load secure uboot error!\n");
		hang();
	}
#endif

	debug("image entry point: 0x%x\n", spl_image->entry_point);
	image_entry = (image_entry_noargs_t)spl_image->entry_point;

	jump_to_entry_point((unsigned long)image_entry);
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
