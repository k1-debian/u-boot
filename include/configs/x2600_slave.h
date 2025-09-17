/*
 * Ingenic X2670 configuration
 *
 * Copyright (c) 2016 Ingenic Semiconductor Co.,Ltd
 * Author: cxtan <chenxi.tan@ingenic.cn>
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
#ifndef __X2600_SLAVE_H_
#define	__X2600_SLAVE_H_
/**
 * Basic configuration(SOC, Cache, UART, DDR).
 */
#define CONFIG_MIPS32		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST2
#define CONFIG_SYS_LITTLE_ENDIAN
/* #define CONFIG_X2600_FPGA	/1* x2600 SoC *1/ */
/* #define CONFIG_FPGA		/1* x2600 FPGA *1/ */
#define CONFIG_X2600		/* x2600 SoC */

#include "x2670_ddr.h"

#define CONFIG_SYS_APLL_FREQ		1152000000	/*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ		1800000000	/*If MPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ		300000000	/*If MPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL		APLL
#define CONFIG_DDR_SEL_PLL		MPLL
#define CONFIG_SYS_CPU_FREQ		1152000000
#define CONFIG_SYS_MEM_FREQ		900000000

#define CONFIG_SYS_AHB0_FREQ		360000000
#define CONFIG_SYS_AHB2_FREQ		300000000	/*APB = AHB2/2*/

#define CONFIG_GLOBAL_PARAMS_OFFSET		0x14
/* #define CONFIG_USE_GLOBAL_SHARED_PARAMS */

/* Device Tree Configuration*/
/*#define CONFIG_OF_LIBFDT 1*/
#ifdef CONFIG_OF_LIBFDT
#define IMAGE_ENABLE_OF_LIBFDT  1
#define CONFIG_LMB
#endif

/* CLK CGU */
#define  CGU_CLK_SRC {				\
		{LCD, MPLL},			\
		{MSC0, MPLL},			\
		{SFC, MPLL},			\
		{MACPHY, MPLL},			\
		{SRC_EOF,SRC_EOF}		\
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


#define CONFIG_SYS_UART_INDEX		0
#define CONFIG_BAUDRATE			115200

#define CONFIG_CMD_PRICE

#define CONFIG_CMD_USB		/* USB host command */
#ifdef CONFIG_CMD_USB
#define CONFIG_CMD_USB_LOAD
#define CONFIG_USB_LOAD
#define CONFIG_USB_DWC2		/* DWC2 Host Driver. */
#define CONFIG_USB_DRV_VBUS	 GPIO_PE(18)
#define CONFIG_USB_DWC2_REG_ADDR USB_BASE
#endif

/**
 * Boot arguments definitions.
 */
#define CONFIG_BOOTARGS  ""
#define CONFIG_BOOTCOMMAND "usbprice"

/**
 * Boot command definitions.
 */
#define CONFIG_BOOTDELAY 0
#define CONFIG_SYS_CONSOLE_INFO_QUIET

#define PARTITION_NUM 10

#ifdef CONFIG_CMD_EJTAG
#define CONFIG_XBURST_TRAPS
#define CONFIG_INT_DEBUG
#endif

/**
 * Drivers configuration.
 */


/* SFC */
#if 0
#define CONFIG_SFC_V20
#define CONFIG_JZ_SFC_PD

/* sfc ota config */
#ifdef CONFIG_OTA_VERSION30
#ifdef CONFIG_SPL_SFC_NAND
#define CONFIG_KUNPENG_OTA_VERSION20
#else
#define CONFIG_JZSD_OTA_VERSION20
#endif
#endif /*end of ota*/

#define CONFIG_SFC_RATE			48000000

#ifdef CONFIG_SPL_SFC_NOR
#define CONFIG_JZ_SFC
#define CONFIG_CMD_SFC_NOR
#define CONFIG_JZ_SFC_NOR
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SFC_NOR_RATE	200000000	/* value <= 400000000(sfc 100Mhz)*/
#define CONFIG_SFC_QUAD
#define CONFIG_SPIFLASH_PART_OFFSET		0x5800
#define CONFIG_SPI_NORFLASH_PART_OFFSET		0x5874
#define CONFIG_NOR_MAJOR_VERSION_NUMBER		1
#define CONFIG_NOR_MINOR_VERSION_NUMBER		0
#define CONFIG_NOR_REVERSION_NUMBER		0
#define CONFIG_NOR_VERSION     (CONFIG_NOR_MAJOR_VERSION_NUMBER | (CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | (CONFIG_NOR_REVERSION_NUMBER <<16))
/*#define CONFIG_NOR_BUILTIN_PARAMS*/
/*#define CONFIG_NOR_COMMON_PARAMS*/
/*#define CONFIG_NOR_COMMON_PARAMS_COUNT          3*/
#endif

/* sfc nand config */
#ifdef  CONFIG_SPL_SFC_NAND
#define CONFIG_SFC_NAND_RATE    200000000	/* value <= 400000000(sfc 100Mhz)*/
#define CONFIG_SFC_QUAD
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SPIFLASH_PART_OFFSET		0x5800
#define CONFIG_SPI_NAND_BPP                     (2048 +64)      /*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB                     (64)            /*Page Per Block*/
#define CONFIG_JZ_SFC
/* #define CONFIG_CMD_SFCNAND */
#define CONFIG_CMD_NAND
#define CONFIG_SYS_MAX_NAND_DEVICE		1
#define CONFIG_SYS_NAND_BASE			0xb3441000
#define CONFIG_SYS_MAXARGS			16
/*#define CONFIG_NAND_BUILTIN_PARAMS*/

/* sfc nand env config */
#define CONFIG_MTD_DEVICE
/* #define CONFIG_CMD_SAVEENV		/\* saveenv *\/ */
/* #define CONFIG_CMD_UBI */
/* #define CONFIG_CMD_UBIFS */
/* #define CONFIG_CMD_MTDPARTS */
#define CONFIG_MTD_PARTITIONS
#define MTDIDS_DEFAULT                  "nand0:nand"
#define MTDPARTS_DEFAULT                "mtdparts=nand:1M(boot),8M(kernel),40M(rootfs),-(data)"
#define CONFIG_SYS_NAND_BLOCK_SIZE	(128 * 1024)
#endif

#define CONFIG_SYS_NAND_SELF_INIT
#endif
/* end of sfc */
#define CONFIG_SYS_MAXARGS 16

/* Ethernet: gmac*/

/* DEBUG ETHERNET */
#define CONFIG_SERVERIP		192.168.4.13
#define CONFIG_IPADDR		192.168.10.206
#define CONFIG_GATEWAYIP        192.168.10.1
#define CONFIG_NETMASK          255.255.255.0
#define CONFIG_ETHADDR          00:11:22:33:44:55


#define GMAC_PHY_RMII   2//4
#define CONFIG_SYS_RX_ETH_BUFFER 64


#ifdef CONFIG_NET_X2600
#define CONFIG_MAC_AHB_BUS

/* Select GMAC Controller */
#define CONFIG_GMAC0
/* Select GMAC Interface mode */

#define CONFIG_NET_GMAC_PHY_MODE GMAC_PHY_RMII

#ifdef CONFIG_GMAC0
#define CONFIG_GAMAC_MODE_CTRL_ADDR	0xb00000e4
#define JZ_GMAC_BASE			0xb34b0000
#define CONFIG_GMAC_CRLT_PORT GPIO_PORT_C
#define CONFIG_GMAC_CRLT_PORT_PINS (0x3ff << 15)
#define CONFIG_GMAC_CRTL_PORT_INIT_FUNC GPIO_FUNC_0
#define CONFIG_GMAC_PHY_RESET	GPIO_PC(15)
#define CONFIG_GMAC_TX_CLK_DELAY 0x3f
#define CONFIG_GMAC_RX_CLK_DELAY 0
#endif

#define CONFIG_GMAC_CRTL_PORT_SET_FUNC GPIO_INPUT
#define CONFIG_GMAC_PHY_RESET_ENLEVEL	0
#endif /* CONFIG_NET_X2600 */

/* GPIO */
#define CONFIG_JZ_GPIO

/* VDDIO_CIM/VDDIO_SD configurations */
#define CONFIG_VDD_CIM_VOLTAGE	GPIO_VOLTAGE_3V3
#define CONFIG_VDD_SD_VOLTAGE	GPIO_VOLTAGE_3V3

/**
 * Command configuration.
 */
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_CONSOLE      /* coninfo			*/

#define CONFIG_CMD_GPIO

/**
 * Serial download configuration
 */
#define CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "# "
#define CONFIG_SYS_CBSIZE 1024 /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_SYS_MONITOR_LEN		(512 * 1024)
#define CONFIG_SYS_MALLOC_LEN		(4 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)

#define CONFIG_SYS_SDRAM_BASE      0x80000000 + (40 *1024 * 1024) /* cached (KSEG0) address */
#define CONFIG_SYS_SDRAM_SIZE      (16 * 1024 * 1024)


#define CONFIG_SYS_SDRAM_MAX_TOP 0x90000000 /* don't run into IO space */

#define CONFIG_SYS_TEXT_BASE 0x82800000 /* 40 * 1024 * 1024 */

#define CONFIG_LAYOUT_UBOOT_START_OFF (40 * 1024 * 1024)
#define CONFIG_LAYOUT_SPL_BIN_OFF   (256 * 1024)
#define CONFIG_LAYOUT_KENEL_BIN_OFF (CONFIG_LAYOUT_SPL_BIN_OFF + 16 * 1024)
#define CONFIG_LAYOUT_SHARE_START   (5 * 1024 * 1024 + 512 * 1024)


#define CONFIG_SYS_INIT_SP_OFFSET (CONFIG_LAYOUT_UBOOT_START_OFF + CONFIG_LAYOUT_SPL_BIN_OFF)


#define CONFIG_SYS_LOAD_ADDR		0x88000000
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x88000000



#define CONFIG_SYS_SC_TEXT_BASE     0x80100004
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE

#define CONFIG_UBOOT_OFFSET             0x6000

/**
 * Environment
 */
#ifdef CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_SIZE			(32 << 10)
#define CONFIG_ENV_OFFSET		(CONFIG_SYS_MONITOR_LEN + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#elif defined(CONFIG_ENV_IS_IN_SFC)
#define CONFIG_CMD_SFC_NOR
#define CONFIG_ENV_SIZE                 (4 << 10)
#define CONFIG_ENV_OFFSET               0x3f000 /*write nor flash 252k address*/
#define CONFIG_CMD_SAVEENV

#else
#if 0
/* nand Environment variables */
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_SECT_SIZE	CONFIG_SYS_NAND_BLOCK_SIZE /* 128K */
#define SPI_NAND_BLK            CONFIG_SYS_NAND_BLOCK_SIZE /* the spi nand block size */
#define CONFIG_ENV_SIZE         SPI_NAND_BLK /* uboot is 1M but the last block size is the env */
#define CONFIG_ENV_OFFSET       (CONFIG_SYS_NAND_BLOCK_SIZE * 6) /* offset is 768k */
#define CONFIG_ENV_OFFSET_REDUND (CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_ENV_IS_IN_SFC_NAND
#endif
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE    512
#endif

#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SPL_SFC_SUPPORT
#define CONFIG_SPL_VERSION	1
#endif

/* LCD */
/* #define CONFIG_LCD */
#define CONFIG_GPIO_PWR_WAKE		GPIO_PB(31)
#define CONFIG_GPIO_PWR_WAKE_ENLEVEL	0
#define CONFIG_SYS_DCACHE_OFF

#ifdef CONFIG_LCD
#define CONFIG_LCD_FORMAT_X8B8G8R8
#define LCD_BPP             5

#define CONFIG_LCD_LOGO
/*#define CONFIG_LCD_INFO_BELOW_LOGO      	//display the console info on lcd panel for debugg*/
#define CONFIG_SYS_WHITE_ON_BLACK
#define CONFIG_SYS_PWM_PERIOD       10000 	/* Pwm period in ns */
#define CONFIG_SYS_PWM_CHN      15		/* Pwm channel ok*/
#define CONFIG_SYS_PWM_FULL     256
#define CONFIG_SYS_BACKLIGHT_LEVEL  200		/* Backlight brightness is (80 / 256) */
#define CONFIG_JZ_LCD_V14
#define CONFIG_JZ_PWM_V2

/*#define CONFIG_VIDEO_FW050*/
#define CONFIG_VIDEO_ZC502
#if defined(CONFIG_VIDEO_FW050) || defined(CONFIG_VIDEO_ZC502)
#define CONFIG_JZ_MIPI_DSI
#endif

#if defined(CONFIG_VIDEO_FW050) || defined(CONFIG_VIDEO_ZC502)
#define CONFIG_GPIO_LCD_VDD     GPIO_PC(11)
#define CONFIG_GPIO_LCD_PWM     GPIO_PC(14)
#define CONFIG_GPIO_LCD_RST     GPIO_PC(10)
#endif

#define CONFIG_SYS_CONSOLE_INFO_QUIET
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define CONFIG_LCD_ENABLE_RDMA_FB

#endif /* CONFIG_LCD */

/**
 * SPL configuration
 */
/* #define CONFIG_SPL */
/* #define CONFIG_SPL_FRAMEWORK */
#define CONFIG_SLAVE_CORE

#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH		"$(CPUDIR)/$(SOC)"
#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-nor-spl.lds"
#else
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-spl.lds"
#endif
#define CONFIG_SPL_PAD_TO		24576 /* equal to spl max size */

#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT

/* #define CONFIG_SPL_SERIAL_SUPPORT */
/* #define CONFIG_SPL_I2C_SUPPORT */
/* #define CONFIG_SPL_REGULATOR_SUPPORT */
/* #define CONFIG_SPL_CORE_VOLTAGE		1300 */
#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SPL_TEXT_BASE		0xba000000
#else
#define CONFIG_SPL_TEXT_BASE		0x80001000
#endif	/*CONFIG_SPL_NOR_SUPPORT*/
#define CONFIG_SPL_MAX_SIZE		(22 * 1024)


#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SYS_UBOOT_BASE		(CONFIG_SPL_TEXT_BASE + CONFIG_SPL_PAD_TO - 0x40)	//0x40 = sizeof (image_header)
#define CONFIG_SYS_OS_BASE		0
#define CONFIG_SYS_SPL_ARGS_ADDR	0
#define CONFIG_SYS_FDT_BASE		0
#endif

/* MMC  spl stage */
#if defined(CONFIG_SPL_MMC_SUPPORT) || defined(CONFIG_SPL_JZMMC_SUPPORT)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	82 /* 17k + 24k (17KB GPT offset and spl size CONFIG_SPL_PAD_TO) */
#define CONFIG_CMD_SAVEENV  /* saveenv */
/*#define CONFIG_SPL_JZ_MSC_BUS_8BIT	//only for emmc*/
  #ifdef CONFIG_SPL_JZMMC_SUPPORT
	#define CONFIG_SPL_JZSDHCI
  #endif
  #ifdef CONFIG_SPL_MMC_SUPPORT
	#define CONFIG_JZ_MMC_SPLMSC		//Configuration SPL stage msc controller use jz_sdhci driver
  #endif
#endif /* CONFIG_SPL_MMC_SUPPORT || CONFIG_SPL_JZMMC_SUPPORT */

/**
 * GPT configuration
 */
#ifdef CONFIG_GPT_CREATOR
#define CONFIG_GPT_TABLE_PATH	"$(TOPDIR)/board/$(BOARDDIR)"
#else
/* USE MBR + zero-GPT-table instead if no gpt table defined*/
#define CONFIG_MBR_P0_OFF	64mb
#define CONFIG_MBR_P0_END	556mb
#define CONFIG_MBR_P0_TYPE 	linux

#define CONFIG_MBR_P1_OFF	580mb
#define CONFIG_MBR_P1_END 	1604mb
#define CONFIG_MBR_P1_TYPE 	linux

#define CONFIG_MBR_P2_OFF	28mb
#define CONFIG_MBR_P2_END	58mb
#define CONFIG_MBR_P2_TYPE 	linux

#define CONFIG_MBR_P3_OFF	1609mb
#define CONFIG_MBR_P3_END	7800mb
#define CONFIG_MBR_P3_TYPE 	fat
#endif


/* Wrong keys. */
#define CONFIG_GPIO_RECOVERY           GPIO_PB(11)
#define CONFIG_GPIO_RECOVERY_ENLEVEL 0

#define CONFIG_BOARD_INFO_SILENT

/* #define CONFIG_JZ_SCBOOT */
/* #define CONFIG_JZ_CKEYAES */
/* #define CONFIG_JZ_SECURE_SUPPORT */
#endif/*END OF __X2600_SLAVE_H_ */
