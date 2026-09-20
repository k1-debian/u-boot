/*
 * Ingenic X3000 base minimal configuration.
 */
#ifndef __X3000_BASE_COMMON_H__
#define __X3000_BASE_COMMON_H__

/**
 * Basic configuration(SOC, Cache, UART, DDR).
 */
#define CONFIG_MIPS32		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST2
#define CONFIG_SYS_LITTLE_ENDIAN
#define CONFIG_X3000		/* x3000 SoC */

#include "x3000_ddr.h"

#define CONFIG_SYS_APLL_FREQ		800000000	/*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ		800000000	/*If MPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ		300000000	/*If MPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL			APLL
#define CONFIG_DDR_SEL_PLL			MPLL
#define CONFIG_SYS_CPU_FREQ			800000000
#define CONFIG_SYS_MEM_FREQ			200000000

#define CONFIG_SYS_AHB0_FREQ		200000000
#define CONFIG_SYS_AHB1_FREQ		200000000
#define CONFIG_SYS_AHB2_FREQ		200000000	/*APB = AHB2/2*/
#define CONFIG_SYS_PCLK_FREQ		100000000
#define CONFIG_SYS_LEP_FREQ			300000000

/* CLK CGU */
#define CGU_CLK_SRC {				\
		{DDR, MPLL},			\
		{MSC0, MPLL},			\
		{MSC1, MPLL},			\
		{MSC2, MPLL},			\
		{SFC, MPLL},			\
		{SRC_EOF, SRC_EOF}		\
	}

#define CONFIG_SYS_EXTAL		24000000	/* EXTAL freq: 24 MHz */
#define CONFIG_SYS_OST_FREQ		12500000	/* on fpga, 12.5MHz */
#define CONFIG_SYS_HZ			1000		/* incrementer freq */

/**
 *  Cache Configs:
 *  	Must implement DCACHE/ICACHE SCACHE according to xburst spec.
 * */
#define CONFIG_SYS_DCACHE_SIZE		(32 * 1024)
#define CONFIG_SYS_DCACHELINE_SIZE	(32)
#define CONFIG_SYS_DCACHE_WAYS		(8)
#define CONFIG_SYS_ICACHE_SIZE		(32 * 1024)
#define CONFIG_SYS_ICACHELINE_SIZE	(32)
#define CONFIG_SYS_ICACHE_WAYS		(8)
#define CONFIG_SYS_CACHELINE_SIZE	(32)
/* A switch to configure whether cpu has a 2nd level cache */
#define CONFIG_BOARD_SCACHE
#define CONFIG_SYS_SCACHE_SIZE		(256 * 1024)
#define CONFIG_SYS_SCACHELINE_SIZE	(64)
#define CONFIG_SYS_SCACHE_WAYS		(8)

/* MMC */
#if defined(CONFIG_JZ_MMC_MSC0) || defined(CONFIG_JZ_MMC_MSC1) || \
	defined(CONFIG_JZ_MMC_MSC2)
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_MMC_SPL_PARAMS
#define CONFIG_SDHCI
#define CONFIG_JZ_SDHCI
#define MSC_INIT_CLK			400000
#define MSC_WORKING_CLK			50000000
#endif

#ifdef CONFIG_JZ_MMC_MSC0
#define CONFIG_JZ_MMC_MSC0_PD
#define CONFIG_SPL_JZ_MSC_BUS_4BIT
#elif defined(CONFIG_JZ_MMC_MSC1)
#define CONFIG_JZ_MMC_MSC1_PD
#elif defined(CONFIG_JZ_MMC_MSC2)
#define CONFIG_JZ_MMC_MSC2_PD
#endif

#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SPL_SFC_SUPPORT
#define CONFIG_SPL_VERSION		1
#define CONFIG_SFC_V20
#define CONFIG_JZ_SFC_PD		/* set gpio */
#define CONFIG_SFC_RATE			48000000
#define CONFIG_SPIFLASH_PART_OFFSET		0x5800
#endif

/* sfc nor config */
#ifdef CONFIG_SPL_SFC_NOR
#define CONFIG_JZ_SFC
#define CONFIG_JZ_SFC_NOR
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SFC_NOR_RATE	200000000
#define CONFIG_SFC_QUAD
#define CONFIG_SPI_NORFLASH_PART_OFFSET		(CONFIG_SPIFLASH_PART_OFFSET + 0x74)
#define CONFIG_NOR_MAJOR_VERSION_NUMBER		1
#define CONFIG_NOR_MINOR_VERSION_NUMBER		0
#define CONFIG_NOR_REVERSION_NUMBER			0
#define CONFIG_NOR_VERSION	(CONFIG_NOR_MAJOR_VERSION_NUMBER | \
		(CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | \
		(CONFIG_NOR_REVERSION_NUMBER << 16))
#define CONFIG_FLASH_TYPE "flashtype=nor"
#endif

/* sfc nand config */
#ifdef  CONFIG_SPL_SFC_NAND
#define CONFIG_JZ_SFC
#define CONFIG_SFC_QUAD
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SFC_NAND_RATE		200000000
#define CONFIG_SPI_NAND_BPP			(2048 +64) /*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB			(64) /*Page Per Block*/
#define CONFIG_SYS_MAX_NAND_DEVICE		1
#define CONFIG_SYS_NAND_BASE			SFC_BASE
#define CONFIG_SYS_NAND_BLOCK_SIZE	(128 * 1024)
#define CONFIG_SYS_NAND_SELF_INIT
#define CONFIG_FLASH_TYPE "flashtype=nand"
#endif
/* end of sfc */

#define CONFIG_SPL_PAD_TO		0x6000
#define CONFIG_SPL_MAX_SIZE		0x5800
#define CONFIG_UBOOT_OFFSET		CONFIG_SPL_PAD_TO

/* GPIO */
#define CONFIG_JZ_GPIO

#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_SYS_NO_FLASH

#define CONFIG_SYS_MONITOR_LEN		(512 * 1024)
#define CONFIG_SYS_MALLOC_LEN		(16 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000 /* cached (KSEG0) address */
#define CONFIG_SYS_SDRAM_MAX_TOP	0x90000000 /* don't run into IO space */
#define CONFIG_SYS_INIT_SP_OFFSET	0x400000

#define CONFIG_SYS_TEXT_BASE		0x80100000
#define CONFIG_SYS_SC_TEXT_BASE     0x80100004
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_SYS_MAXARGS 16
#define CONFIG_SYS_LOAD_ADDR		0x88000000
#define CONFIG_SYS_CBSIZE 1024 /* Console I/O Buffer Size */
#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "# "
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

/**
 * Environment
 */
#ifdef CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			(32 << 10)
#define CONFIG_ENV_OFFSET		(CONFIG_SYS_MONITOR_LEN + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#elif defined(CONFIG_ENV_IS_IN_SFC)
#define CONFIG_ENV_SIZE                 (4 << 10)
#define CONFIG_ENV_OFFSET               0x3f000 /*write nor flash 252k address*/

#elif defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_SECT_SIZE	CONFIG_SYS_NAND_BLOCK_SIZE /* 128K */
#define SPI_NAND_BLK            CONFIG_SYS_NAND_BLOCK_SIZE /* the spi nand block size */
#define CONFIG_ENV_SIZE         SPI_NAND_BLK /* uboot is 1M but the last block size is the env */
#define CONFIG_ENV_OFFSET       (CONFIG_SYS_NAND_BLOCK_SIZE * 6) /* offset is 768k */
#define CONFIG_ENV_OFFSET_REDUND (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_ENV_IS_IN_SFC_NAND
#endif

/**
 * SPL configuration
 */
#define CONFIG_SPL
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH		"$(CPUDIR)/$(SOC)"
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-spl.lds"
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_TEXT_BASE		0x80001000

/* MMC  spl stage */
#if defined(CONFIG_SPL_MMC_SUPPORT) || defined(CONFIG_SPL_JZMMC_SUPPORT)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	82
#ifdef CONFIG_SPL_JZMMC_SUPPORT
#define CONFIG_SPL_JZSDHCI
#endif
#ifdef CONFIG_SPL_MMC_SUPPORT
#define CONFIG_JZ_MMC_SPLMSC
#endif
#define CONFIG_GPT_TAB_BUILT_IN
#endif

/**
 * GPT configuration
 */
#ifdef CONFIG_GPT_CREATOR
#ifndef CONFIG_GPT_TABLE_PATH
#define CONFIG_GPT_TABLE_PATH	"$(TOPDIR)/board/$(BOARDDIR)"
#endif
#endif

/*
 * uart setting
 */
#ifndef CONFIG_SYS_UART_INDEX
#define CONFIG_SYS_UART_INDEX	7
#endif

#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE			3000000
#endif

/* boot args console tty
 */
#if CONFIG_SYS_UART_INDEX == 0
#define ARG_CONSOLE_TTY "console=ttyS0,"
#elif CONFIG_SYS_UART_INDEX == 1
#define ARG_CONSOLE_TTY "console=ttyS1,"
#elif CONFIG_SYS_UART_INDEX == 2
#define ARG_CONSOLE_TTY "console=ttyS2,"
#elif CONFIG_SYS_UART_INDEX == 3
#define ARG_CONSOLE_TTY "console=ttyS3,"
#elif CONFIG_SYS_UART_INDEX == 4
#define ARG_CONSOLE_TTY "console=ttyS4,"
#elif CONFIG_SYS_UART_INDEX == 5
#define ARG_CONSOLE_TTY "console=ttyS5,"
#elif CONFIG_SYS_UART_INDEX == 6
#define ARG_CONSOLE_TTY "console=ttyS6,"
#elif CONFIG_SYS_UART_INDEX == 7
#define ARG_CONSOLE_TTY "console=ttyS7,"
#else
#error "please add more define here"
#endif

/* boot args uart rate
 */
#if CONFIG_BAUDRATE == 115200
#define ARG_CONSOLE_RATE "115200n8"
#elif CONFIG_BAUDRATE == 3000000
#define ARG_CONSOLE_RATE "3000000n8"
#else
#error "please add more define here"
#endif

/* boot args console
 */
#define ARGS_CONSOLE ARG_CONSOLE_TTY ARG_CONSOLE_RATE

#ifdef CONFIG_ARG_NO_CONSOLE
#undef ARGS_CONSOLE
#define ARGS_CONSOLE "no_console"
#endif

#ifdef CONFIG_ARG_QUIET
#define ARGS_QUIET "quiet"
#else
#define ARGS_QUIET ""
#endif

/* boot args mem define
 */
#define CONFIG_SPL_AUTO_PROBE_ARGS_MEM
/* 这里内存占位字符长度需要确保足够大, 以保证存放预留内存描述字符不会溢出 */
#define ARGS_MEM_RESERVED "[mem-start-------------------------------------------------------------------------------------------------------------------------------------------------------------------mem-end]"
#ifndef CONFIG_RMEM_MB
#define CONFIG_RMEM_MB 0
#endif

#ifndef CONFIG_NMEM_MB
#define CONFIG_NMEM_MB 0
#endif

#ifndef CONFIG_RTOS_SIZE_MB
#define CONFIG_RTOS_SIZE_MB 0
#endif

#ifndef CONFIG_LCD_MEM_MB
#define CONFIG_LCD_MEM_MB 0
#endif

#ifndef CONFIG_SHARE_MEM_MB
#define CONFIG_SHARE_MEM_MB 0
#endif

#ifndef CONFIG_VPU_MEM_MB
#define CONFIG_VPU_MEM_MB 0
#endif

#ifndef CONFIG_RMEM_KB
#define CONFIG_RMEM_KB 0
#endif

#ifndef CONFIG_NMEM_KB
#define CONFIG_NMEM_KB 0
#endif

#ifndef CONFIG_RTOS_SIZE_KB
#define CONFIG_RTOS_SIZE_KB 0
#endif

#ifndef CONFIG_LCD_MEM_KB
#define CONFIG_LCD_MEM_KB 0
#endif

#ifndef CONFIG_SHARE_MEM_KB
#define CONFIG_SHARE_MEM_KB 0
#endif

#ifndef CONFIG_VPU_MEM_KB
#define CONFIG_VPU_MEM_KB 0
#endif

/* boot args init program
 */
#ifndef CONFIG_ROOTFS_INITRC
#define CONFIG_ROOTFS_INITRC "init=/linuxrc"
#endif

#ifndef CONFIG_ROOTFS2_INITRC
#define CONFIG_ROOTFS2_INITRC CONFIG_ROOTFS_INITRC
#endif

/* boot args rootfs
 */
#if defined(CONFIG_ROOTFS_EXT2)
#define ARG_ROOTFS_TYPE " ro" /* rootfstype=ext2 */
#elif defined(CONFIG_ROOTFS_UBI)
#define ARG_ROOTFS_TYPE "rootfstype=ubifs ro"
#elif defined(CONFIG_ROOTFS_SQUASHFS)
#define ARG_ROOTFS_TYPE "rootfstype=squashfs ro"
#elif defined(CONFIG_ROOTFS_RAMDISK)
#define ARG_ROOTFS_TYPE "rw"
#else
#error "please add more define here"
#endif

#ifndef CONFIG_ROOTFS_PARAM

#ifndef CONFIG_ROOTFS_DEV
#ifdef CONFIG_SPL_JZMMC_SUPPORT
#define CONFIG_ROOTFS_DEV "root=/dev/mmcblk0p2 rootwait"
#else
#define CONFIG_ROOTFS_DEV "root=/dev/mtdblock_bbt_ro2"
#endif /* CONFIG_SPL_JZMMC_SUPPORT */
#endif /* CONFIG_ROOTFS_DEV */

#ifdef CONFIG_SPL_JZMMC_SUPPORT
#define CONFIG_ROOTFS_PARAM CONFIG_ROOTFS_DEV
#else
#define CONFIG_ROOTFS_PARAM CONFIG_FLASH_TYPE " " CONFIG_ROOTFS_DEV
#endif

#endif

#define ARGS_ROOTFS CONFIG_ROOTFS_INITRC" "CONFIG_ROOTFS_PARAM" "ARG_ROOTFS_TYPE

/* boot args rootfs2
 */
#if defined(CONFIG_ROOTFS2_EXT2)
#define ARG_ROOTFS2_TYPE " ro" /* rootfstype=ext2 */
#elif defined(CONFIG_ROOTFS2_UBI)
#define ARG_ROOTFS2_TYPE "rootfstype=ubifs ro"
#elif defined(CONFIG_ROOTFS2_SQUASHFS)
#define ARG_ROOTFS2_TYPE "rootfstype=squashfs ro"
#elif defined(CONFIG_ROOTFS2_RAMDISK)
#define ARG_ROOTFS2_TYPE "rw"
#else
#error "please add more define here"
#endif

#ifndef CONFIG_ROOTFS2_PARAM

#ifndef CONFIG_ROOTFS2_DEV
#ifdef CONFIG_SPL_JZMMC_SUPPORT
#define CONFIG_ROOTFS2_DEV "root=/dev/mmcblk0p4 rootwait"
#else
#define CONFIG_ROOTFS2_DEV "root=/dev/mtdblock_bbt_ro4"
#endif /* CONFIG_SPL_JZMMC_SUPPORT */
#endif /* CONFIG_ROOTFS2_DEV */

#ifdef CONFIG_SPL_JZMMC_SUPPORT
#define CONFIG_ROOTFS2_PARAM CONFIG_ROOTFS2_DEV
#else
#define CONFIG_ROOTFS2_PARAM CONFIG_FLASH_TYPE " " CONFIG_ROOTFS2_DEV
#endif /* CONFIG_SPL_JZMMC_SUPPORT */
#endif /* CONFIG_ROOTFS2_PARAM */

#define ARGS_ROOTFS2 CONFIG_ROOTFS2_INITRC " " CONFIG_ROOTFS2_PARAM " " ARG_ROOTFS2_TYPE

#ifndef CONFIG_ARGS_EXTRA
#define CONFIG_ARGS_EXTRA ""
#endif

#define BOOTARGS_COMMON ARGS_CONSOLE " " ARGS_QUIET " " ARGS_MEM_RESERVED " " CONFIG_ARGS_EXTRA

#ifdef CONFIG_SPL_OS_OTA_BOOT
#define CONFIG_SPL_OS_BOOT
    #define CONFIG_SPL_OTA_NAME       "ota"
    #define CONFIG_SPL_OS_NAME2       "kernel2"
    #define CONFIG_SPL_ROOTFS_NAME2   "rootfs2"
    #define CONFIG_SPL_BOOTARGS2      BOOTARGS_COMMON " " ARGS_ROOTFS2
    #define CONFIG_SYS_SPL_ARGS_ADDR2    CONFIG_SPL_BOOTARGS2
#endif	/* CONFIG_SPL_OS_OTA_BOOT */

#ifdef CONFIG_SPL_OS_BOOT
    #define CONFIG_SPL_BOOTARGS	 BOOTARGS_COMMON " " ARGS_ROOTFS
    #define CONFIG_SPL_OS_NAME        "kernel" /* spi offset of xImage being loaded */
    #define CONFIG_SYS_SPL_ARGS_ADDR    CONFIG_SPL_BOOTARGS
#endif	/* CONFIG_SPL_OS_BOOT */

#define CONFIG_BOOTARGS ""

#endif/*END OF __X3000_BASE_COMMON_H__ */
