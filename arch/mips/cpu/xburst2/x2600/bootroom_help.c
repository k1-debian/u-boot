#include <common.h>
#include <config.h>
#include <asm/mipsregs.h>
#include <asm/arch/clk.h>
#include <asm-generic/gpio.h>
#include <asm-generic/sections.h>
#include <asm/jz_cache.h> 
#include <spl.h>
#include <x2600_bootroom_help.h>

#if !defined(CONFIG_X2600_BOOTROOM_HELP_STAGE1)
#error "x2600 bootroom helper must only be built in stage1 mode"
#endif

#if !defined(CONFIG_X2600_BOOTROOM_HELP_TARGET_MMC)
#error "x2600 bootroom helper requires CONFIG_X2600_BOOTROOM_HELP_TARGET_MMC"
#endif

#if defined(CONFIG_X2600_BOOTROOM_HELP_TARGET_MMC) && \
	!defined(CONFIG_SPL_JZSDHCI)
#error "x2600 bootroom helper MMC target requires CONFIG_SPL_JZSDHCI"
#endif

#if defined(CONFIG_X2600_BOOTROOM_HELP_TARGET_NAND)
#error "x2600 bootroom helper NAND target backend is not implemented yet"
#endif

#if (CONFIG_X2600_BOOTROOM_HELP_TARGET_MMC_OFFSET & 0x1ff)
#error "x2600 bootroom helper MMC offset must be 512-byte aligned"
#endif

#if !(((CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR + \
	CONFIG_X2600_BOOTROOM_HELP_STAGE2_MAX_SIZE) <= CONFIG_SPL_TEXT_BASE) || \
	(CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR >= \
	 (CONFIG_SPL_TEXT_BASE + CONFIG_SPL_MAX_SIZE)))
#error "x2600 bootroom helper stage2 output overlaps the current SPL image"
#endif

#if !(((CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR + \
	CONFIG_X2600_BOOTROOM_HELP_STAGE2_MAX_SIZE) <= \
	 CONFIG_SPL_TEXT_BASE) || \
	(CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR >= \
	 (CONFIG_SPL_TEXT_BASE + CONFIG_SPL_MAX_SIZE)))
#error "x2600 bootroom helper stage2 load buffer overlaps the current SPL image"
#endif

#ifndef CONFIG_JZ_SECURE_SUPPORT
#if (CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR != \
     CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR)
#error "non-secure x2600 bootroom helper requires stage2 load and entry addresses to match"
#endif
#endif

#ifdef CONFIG_JZ_SECURE_SUPPORT
extern int secure_scboot(void *input, void *output);
#endif
extern void gpio_init(void);
extern void pll_init(void);
extern void sdram_init(void);
extern void change_lcd_ddrc_process_priority(void);
extern void flush_cache_all(void);
extern void jump_to_entry_point(unsigned long entry_point);

//#define CONFIG_X2600_BOOTROOM_HELP_VERBOSE_DEBUG

#ifdef CONFIG_X2600_BOOTROOM_HELP_VERBOSE_DEBUG
#define X2600_BOOTROOM_HELP_SCBOOT_MAGIC 0x54424353
#define BOOTROOM_HELP_VERBOSE(fmt, args...) serial_debug(fmt, ##args)
#else
#define BOOTROOM_HELP_VERBOSE(fmt, args...) do { } while (0)
#endif

struct x2600_bootroom_backend {
	const char *name;
	int (*load)(void);
};

#ifdef CONFIG_X2600_BOOTROOM_HELP_VERBOSE_DEBUG
static void x2600_bootroom_help_dump_stage2_header(void)
{
	volatile unsigned int *header =
		(volatile unsigned int *)CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR;

	serial_debug("BOOTROOM-HELP: stage2 head @0x%x = %x %x %x %x\n",
		     CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR,
		     header[0], header[1], header[2], header[3]);
#ifdef CONFIG_JZ_SECURE_SUPPORT
	serial_debug("BOOTROOM-HELP: stage2 sc magic expect=%x got=%x %s\n",
		     X2600_BOOTROOM_HELP_SCBOOT_MAGIC, header[0],
		     header[0] == X2600_BOOTROOM_HELP_SCBOOT_MAGIC ?
		     "match" : "mismatch");
#endif
}
#else
static inline void x2600_bootroom_help_dump_stage2_header(void)
{
}
#endif

#ifdef CONFIG_JZ_SECURE_SUPPORT
#ifdef CONFIG_X2600_BOOTROOM_HELP_VERBOSE_DEBUG
static void x2600_bootroom_help_dump_stage2_exec_head(void)
{
	volatile unsigned int *exec =
		(volatile unsigned int *)CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR;

	serial_debug("BOOTROOM-HELP: stage2 exec @0x%x = %x %x %x %x\n",
		     CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR,
		     exec[0], exec[1], exec[2], exec[3]);
}
#else
static inline void x2600_bootroom_help_dump_stage2_exec_head(void)
{
}
#endif
#endif

static int x2600_bootroom_help_load_from_mmc(void)
{
	unsigned int start_sector;
	unsigned int blkcnt;
	unsigned int ret;

	start_sector = CONFIG_X2600_BOOTROOM_HELP_TARGET_MMC_OFFSET / 512;
	blkcnt = (CONFIG_X2600_BOOTROOM_HELP_STAGE2_MAX_SIZE + 511) / 512;

    BOOTROOM_HELP_VERBOSE("BOOTROOM-HELP: MMC read sector=%d blkcnt=%d dst=0x%x\n",
            start_sector, blkcnt,
           CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR);

	if (x2600_bootroom_mmc_init()) {
		serial_debug("BOOTROOM-HELP: MMC init failed\n");
		return -1;
	}

	ret = x2600_bootroom_mmc_block_read(
		start_sector, blkcnt,
		(unsigned int *)CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR);
	if (ret != blkcnt) {
		serial_debug("BOOTROOM-HELP: MMC read failed %d/%d\n", ret, blkcnt);
		return -1;
	}

    BOOTROOM_HELP_VERBOSE("BOOTROOM-HELP: mmc first word=0x%x\n",
            *(volatile unsigned int *)
            CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR);

	return 0;
}

static const struct x2600_bootroom_backend x2600_bootroom_backend = {
	.name = "mmc",
	.load = x2600_bootroom_help_load_from_mmc,
};

void x2600_bootroom_help_stage1_entry(void)
{
	/* Stage1 does not reach board_init_r(), so clear its BSS here. */
	memset(__bss_start, 0, (char *)&__bss_end - __bss_start);

	gpio_init();

	/* Bring up UART before any helper diagnostics. */
	enable_uart_clk();

#ifdef CONFIG_SPL_SERIAL_SUPPORT
	preloader_console_init();
#endif

#if defined(CONFIG_POWER_PIN_HIGH)
	gpio_direction_output(CONFIG_POWER_PIN_HIGH, 1);
#elif defined(CONFIG_POWER_PIN_LOW)
	gpio_direction_output(CONFIG_POWER_PIN_LOW, 0);
#endif

	ingenic_setup_voltage_config();

	serial_debug("ERROR EPC %x\n", read_c0_errorepc());
	serial_debug("Reset status %x\n", *(volatile unsigned int *)0xb0000008);

	timer_init();
	x2600_bootroom_help_run();
}

void x2600_bootroom_help_run(void)
{
    BOOTROOM_HELP_VERBOSE("BOOTROOM-HELP: backend=%s load=0x%x entry=0x%x size=0x%x\n",
            x2600_bootroom_backend.name,
            CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR,
            CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR,
            CONFIG_X2600_BOOTROOM_HELP_STAGE2_MAX_SIZE);

	if (x2600_bootroom_backend.load())
		hang();

	x2600_bootroom_help_dump_stage2_header();

#ifdef CONFIG_JZ_SECURE_SUPPORT
	{
		int ret;

		ret = secure_scboot((void *)CONFIG_X2600_BOOTROOM_HELP_STAGE2_LOAD_ADDR,
				    (void *)CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR);
		if (ret) {
			serial_debug("BOOTROOM-HELP: stage2 secure_scboot failed %d\n", ret);
			hang();
		}

		x2600_bootroom_help_dump_stage2_exec_head();
        //flush_cache_all();
        flush_icache_all(); /* invalid icache */

        flush_dcache_all(); /* writeback invalid dcache,  */
        __asm__ volatile(
                ".set push     \n\t"
                ".set mips32r2 \n\t"
                "sync          \n\t"
                ".set pop      \n\t"
                );
        ret = ((int (*)(void))CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR)();

		//jump_to_entry_point(CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR);
		hang();
	}
#endif

	//flush_cache_all();
	jump_to_entry_point(CONFIG_X2600_BOOTROOM_HELP_STAGE2_ENTRY_ADDR);
	hang();
}
