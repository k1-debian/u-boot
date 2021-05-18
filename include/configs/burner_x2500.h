/*
 * Ingenic burner_X2500 configuration
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
#ifndef __CONFIG_BURNER_X2500_H__
#define __CONFIG_BURNER_X2500_H__
/**
 * Basic configuration(SOC, Cache, UART, DDR).
 */
#define CONFIG_MIPS32		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST2
#define CONFIG_SYS_LITTLE_ENDIAN
#define CONFIG_X2500	/* x2500 SoC */


#define CONFIG_SYS_APLL_FREQ		900000000	/*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ		1000000000	/*If MPLL not use mast be set 0*/
#define CONFIG_SYS_VPLL_FREQ		1080000000	/*If VPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ		891000000	/*If EPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL		APLL
#define CONFIG_DDR_SEL_PLL		MPLL
#define CONFIG_SYS_CPU_FREQ		CONFIG_SYS_APLL_FREQ
#define CONFIG_SYS_MEM_FREQ		(CONFIG_SYS_MPLL_FREQ / 2)

#define CONFIG_SYS_AHB0_FREQ		234000000
#define CONFIG_SYS_AHB2_FREQ		234000000	/*APB = AHB2/2*/


/* Device Tree Configuration*/
/*#define CONFIG_OF_LIBFDT 1*/
#ifdef CONFIG_OF_LIBFDT
#define IMAGE_ENABLE_OF_LIBFDT	1
#define CONFIG_LMB
#endif


/* CLK CGU */
#define  CGU_CLK_SRC {				\
		{LCD, MPLL},			\
		{MSC0, MPLL},			\
		{SFC, MPLL},			\
		{SRC_EOF,SRC_EOF}		\
	}

#define CONFIG_SYS_EXTAL		24000000	/* EXTAL freq: 24 MHz */
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
#define CONFIG_SYS_SCACHE_SIZE		(128 * 1024)
#define CONFIG_SYS_SCACHELINE_SIZE	(64)
#define CONFIG_SYS_SCACHE_WAYS		(8)


#define CONFIG_SYS_UART_INDEX		1
#define CONFIG_BAUDRATE			115200
/*
#define CONFIG_DDR_TEST_CPU
#define CONFIG_DDR_TEST
#define CONFIG_DDR_TEST_DATALINE
#define CONFIG_DDR_TEST_ADDRLINE
*/
#define CONFIG_DDR_INNOPHY
#define CONFIG_DDR_DLL_OFF
#define CONFIG_DDR_PARAMS_CREATOR
#define CONFIG_DDR_HOST_CC
#define CONFIG_DDR_TYPE_DDR3
#define CONFIG_DDR_CS0			1	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1			0	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32			1	/* 1-32bit-width, 0-16bit-width */

#ifdef CONFIG_DDR_TYPE_DDR3
	/*#define CONFIG_DDR3_TSD34096M1333C9_E_FPGA*/
	#define CONFIG_DDR3_W631GU6NG
#endif

#define CONFIG_DDR_PHY_IMPEDANCE 40
#define CONFIG_DDR_PHY_ODT_IMPEDANCE 120

/*#define CONFIG_DDR_AUTO_SELF_REFRESH*/
#define CONFIG_DDR_AUTO_SELF_REFRESH_CNT 0

#define CONFIG_OPEN_KGD_DRIVER_STRENGTH
#ifdef CONFIG_OPEN_KGD_DRIVER_STRENGTH
#define CONFIG_DDR_DRIVER_OUT_STRENGTH
#define CONFIG_DDR_DRIVER_OUT_STRENGTH_1 1
#define CONFIG_DDR_DRIVER_OUT_STRENGTH_0 0
#endif

#define CONFIG_DDR_CHIP_ODT
#define CONFIG_DDR_CHIP_ODT_VAL
#ifdef CONFIG_DDR_CHIP_ODT_VAL
#define CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_9 0 /* RTT_Nom_9 is MR1 A9 bit */
#define CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_6 1 /* RTT_Nom_6 is MR1 A6 bit */
#define CONFIG_DDR_CHIP_ODT_VAL_RTT_NOM_2 0 /* RTT_Nom_2 is MR1 A2 bit */
#define CONFIG_DDR_CHIP_ODT_VAL_RTT_WR 0  /* RTT_WR is odt for KGD write of MR2*/
#endif

/*
 * #define CONFIG_DDR_CHIP_ODT
 * #define CONFIG_DDR_PHY_ODT
 * #define CONFIG_DDR_PHY_DQ_ODT
 * #define CONFIG_DDR_PHY_DQS_ODT
 * #define CONFIG_DDR_PHY_IMPED_PULLUP		0xe
 * #define CONFIG_DDR_PHY_IMPED_PULLDOWN	0xe
 */

#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SPL_SFC_SUPPORT
#define CONFIG_SPL_VERSION	1
#endif
/**
 * Boot command definitions.
 */
#define CONFIG_BOOTDELAY    0
#define CONFIG_BOOTCOMMAND "burn"

#define PARTITION_NUM 10


/**
 * Drivers configuration.
 */

/* MMC */
#ifdef CONFIG_JZ_MMC_MSC0
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
/*#define CONFIG_MMC_SPL_PARAMS*/
#define CONFIG_SDHCI
#define CONFIG_JZ_SDHCI
/*#define CONFIG_MMC_SDMA*/

/* MSC Command configuration */
#define CONFIG_CMD_MMC

/*#define CONFIG_JZ_MMC_MSC0_PB_4BIT   //set gpio*/
/*#define CONFIG_MMC_TRACE		// only for DEBUG*/
/*#define CONFIG_SDHCI_TRACE	// only for DEBUG*/
#endif

#ifdef CONFIG_JZ_MMC_MSC1
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
/*#define CONFIG_MMC_SPL_PARAMS*/
#define CONFIG_JZ_SDHCI
/*#define CONFIG_MMC_SDMA*/

/* MSC Command configuration */
#define CONFIG_CMD_MMC

/*#define CONFIG_JZ_MMC_MSC1_PC   //set gpio*/
/*#define CONFIG_MMC_TRACE		// only for DEBUG*/
/*#define CONFIG_SDHCI_TRACE	// only for DEBUG*/
#endif


/* SFC */

#define CONFIG_SFC_RATE			48000000
#define CONFIG_SFC_V20

/*#define CONFIG_JZ_SFC_PA*/

#ifdef CONFIG_CMD_SFC_NOR
#define CONFIG_SFC_NOR_RATE    150000000
#define CONFIG_MTD_SFCNOR
#define CONFIG_JZ_SFC
#define CONFIG_JZ_SFC_NOR
#define CONFIG_SFC_QUAD
#define CONFIG_SPIFLASH_PART_OFFSET         0x5800
#define CONFIG_SPI_NORFLASH_PART_OFFSET     0x5874
#define CONFIG_NOR_MAJOR_VERSION_NUMBER     1
#define CONFIG_NOR_MINOR_VERSION_NUMBER     0
#define CONFIG_NOR_REVERSION_NUMBER         0
#define CONFIG_NOR_VERSION     (CONFIG_NOR_MAJOR_VERSION_NUMBER | (CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | (CONFIG_NOR_REVERSION_NUMBER <<16))
#define CONFIG_SPL_VERSION_OFFSET   16
#define CONFIG_SPL_PAD_TO_BLOCK
#endif

/*
 *MTD
 */
#ifdef CONFIG_MTD_SFCNAND
#define CONFIG_SFC_NAND_RATE	    150000000
#define CONFIG_CMD_NAND
#define CONFIG_SYS_NAND_SELF_INIT

#define CONFIG_SPIFLASH_PART_OFFSET     0x5800

#define CONFIG_SPI_NAND_BPP                     (2048 +64)              /*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB                     (64)            /*Page Per Block*/

#define CONFIG_JZ_SFC
/*#define CONFIG_CMD_SFCNAND*/
#define CONFIG_SYS_MAX_NAND_DEVICE      1
#define CONFIG_SYS_NAND_BASE    0xb3441000
#define CONFIG_SYS_MAXARGS      16
#define CONFIG_SYS_MAX_NAND_DEVICE  1

#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_DEVICE
#define MTDIDS_DEFAULT                  "nand0=nand"


/*
 *  SPINAND MAC SN : the product of customer add partition of sequence code.
 */
#define CONFIG_JZ_SPINAND_MAC
#define CONFIG_MAC_SIZE	    (1 * 1024 * 1024)
#define CONFIG_JZ_SPINAND_SN
#define CONFIG_SN_SIZE	    (1 * 1024 * 1024)

#endif


/* end of sfc */

/*burner*/
#ifdef CONFIG_CMD_BURN
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_BURNER
/*#define CONFIG_JZ_SCBOOT*/
#define CONFIG_USB_GADGET
#define CONFIG_USB_JZ_BURNER_GADGET
#define CONFIG_JZ_VERDOR_BURN_EXTPOL
/*#define CONFIG_JZ_VERDOR_BURN_EP_TEST*/
#define CONFIG_JZ_VERDOR_BURN_FUNCTION
#define CONFIG_USB_JZ_DWC2_UDC_V1_1
#define CONFIG_USB_SELF_POLLING
#define CONFIG_USB_PRODUCT_ID  0xEAEF
#define CONFIG_USB_VENDOR_ID   0xa108
#define CONFIG_BURNER_CPU_INFO "X2500"
#define CONFIG_USB_GADGET_VBUS_DRAW 500
#define CONFIG_BURNER_PRIDUCT_INFO      "Ingenic USB BOOT DEVICE"
#endif  /* !CONFIG_CMD_BURN */

/* GPIO */
#define CONFIG_JZ_GPIO
#define CONFIG_INGENIC_SOFT_I2C

/**
 * Command configuration.
 */
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_CMD_DHCP 	/* DHCP support			*/
#define CONFIG_CMD_ECHO		/* echo arguments		*/
#define CONFIG_USE_XYZMODEM	/* xyzModem 			*/
#define CONFIG_CMD_LOAD		/* serial load support 		*/
#define CONFIG_CMD_LOADB	/* loadb			*/
#define CONFIG_CMD_LOADS	/* loads			*/
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#define CONFIG_CMD_RUN		/* run command in env variable	*/
#define CONFIG_CMD_SETGETDCR	/* DCR support on 4xx		*/
#define CONFIG_CMD_SOURCE	/* "source" command support	*/
#define CONFIG_CMD_GETTIME
/*#define CONFIG_CMD_GPIO*/
#define CONFIG_CMD_DATE
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FAT
#define CONFIG_EFI_PARTITION


#define CONFIG_CMD_DDR_TEST	/* DDR Test Command */

/**
 * Serial download configuration
 */
#define CONFIG_LOADS_ECHO	1	/* echo on for serial download */

/**
 * Miscellaneous configurable options
 */
#define CONFIG_DOS_PARTITION

#define CONFIG_LZO
#define CONFIG_RBTREE

#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_SYS_NO_FLASH
#define CONFIG_SYS_FLASH_BASE	0 /* init flash_base as 0 */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_MISC_INIT_R 1

#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAUL)

#define CONFIG_SYS_MAXARGS 16
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_PROMPT CONFIG_SYS_BOARD "# "
#define CONFIG_SYS_CBSIZE 1024 /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_SYS_MONITOR_LEN		(384 * 1024)
#define CONFIG_SYS_MALLOC_LEN		(16 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000 /* cached (KSEG0) address */
#define CONFIG_SYS_SDRAM_MAX_TOP	0x90000000 /* don't run into IO space */
#define CONFIG_SYS_INIT_SP_OFFSET	0x400000
#define CONFIG_SYS_LOAD_ADDR		0x88000000
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		0x88000000

#define CONFIG_SYS_TEXT_BASE		0x80100000
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE


/**
 * Environment
 */
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE			(32 << 10)


/**
 * SPL configuration
 */
#define CONFIG_SPL
#define CONFIG_SPL_FRAMEWORK

#define CONFIG_SPL_NO_CPU_SUPPORT_CODE
#define CONFIG_SPL_START_S_PATH		"$(CPUDIR)/$(SOC)"
#define CONFIG_SPL_LDSCRIPT		"$(TOPDIR)/board/$(BOARDDIR)/u-boot-spl.lds"
#define CONFIG_SPL_PAD_TO		24576 /* equal to spl max size */

#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT

#define CONFIG_SPL_GINFO_BASE		0x80001000
#define CONFIG_SPL_GINFO_SIZE		0x800


#define CONFIG_SPL_TEXT_BASE		0x80001800
#define CONFIG_SPL_MAX_SIZE		(18 * 1024)

/*rtc*/
#define CONFIG_RTC_JZ47XX

#endif/*END OF _BURNER_X2500__*/
