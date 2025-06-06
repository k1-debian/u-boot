 /*
 * Copyright (c) 2020 Ingenic Semiconductor Co.,Ltd
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
#ifndef __GANDER__
#define __GANDER__

/**
 * Basic configuration(SOC, Cache, UART, DDR).
 */
#define CONFIG_MIPS32		/* MIPS32 CPU core */
#define CONFIG_CPU_XBURST2
#define CONFIG_SYS_LITTLE_ENDIAN
#define CONFIG_X2500	/* x2500 SoC */


#define CONFIG_SYS_APLL_FREQ		1200000000	/*If APLL not use mast be set 0*/
#define CONFIG_SYS_MPLL_FREQ		1300000000	/*If MPLL not use mast be set 0*/
#define CONFIG_SYS_VPLL_FREQ		1080000000	/*If VPLL not use mast be set 0*/
#define CONFIG_SYS_EPLL_FREQ		891000000	/*If EPLL not use mast be set 0*/
#define CONFIG_CPU_SEL_PLL		APLL
#define CONFIG_DDR_SEL_PLL		MPLL
#define CONFIG_SYS_CPU_FREQ		1200000000
#define CONFIG_SYS_MEM_FREQ		650000000

#define CONFIG_SYS_AHB0_FREQ		300000000
#define CONFIG_SYS_AHB2_FREQ		300000000	/*APB = AHB2/2*/


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
		{MSC1, MPLL},			\
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
/*#define CONFIG_DDR_TYPE_LPDDR3*/
/*#define CONFIG_DDR_TYPE_LPDDR2*/
#define CONFIG_DDR_CS0			1	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_CS1			0	/* 1-connected, 0-disconnected */
#define CONFIG_DDR_DW32			1	/* 1-32bit-width, 0-16bit-width */
/*#define CONFIG_DDR3_TSD34096M1333C9_E*/

#ifdef CONFIG_DDR_TYPE_LPDDR2
	/*#define CONFIG_LPDDR2_FMT4D32UAB_25LI_FPGA*/
	/* #define CONFIG_LPDDR2_AD210032F_AB_FPGA */
	#define CONFIG_LPDDR2_W97BV6MK
	#define CONFIG_LPDDR2_W97BV6MK_MEM_FREQ		500000000
#endif

#ifdef CONFIG_DDR_TYPE_DDR3
	/*#define CONFIG_DDR3_TSD34096M1333C9_E_FPGA*/
	#define CONFIG_DDR3_W631GU6NG
	#define CONFIG_DDR3_NK5CC128M8HKX
#endif

#ifdef CONFIG_DDR_TYPE_LPDDR3
	/* #define CONFIG_LPDDR3_MT52L256M32D1PF_FPGA*/
	/* #define CONFIG_LPDDR3_AD310032C_AB_FPGA */
	#define CONFIG_LPDDR3_W63AH6NKB_BI
	#define CONFIG_LPDDR3_W63AH6NKB_BI_MEM_FREQ	750000000
#endif

#define CONFIG_DDR_PHY_IMPEDANCE 40
#define CONFIG_DDR_PHY_ODT_IMPEDANCE 120
/* #define CONFIG_FPGA_TEST */
/*#define CONFIG_DDR_AUTO_REFRESH_TEST*/

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

/*pmu slp pin*/
#define CONFIG_REGULATOR
#ifdef  CONFIG_REGULATOR
/*#define CONFIG_JZ_PMU_SLP_OUTPUT1*/
/*#define CONFIG_INGENIC_SOFT_I2C*/
#define CONFIG_PMU_D2041
/*#define CONFIG_RICOH61X_I2C_SCL	GPIO_PC(25)*/
/*#define CONFIG_RICOH61X_I2C_SDA	GPIO_PC(26)*/
/*#define CONFIG_SOFT_I2C_READ_REPEATED_START*/
#endif


#if defined(CONFIG_SPL_SFC_NOR) || defined(CONFIG_SPL_SFC_NAND)
#define CONFIG_SPL_SFC_SUPPORT
#define CONFIG_SPL_VERSION	1
#endif

/**
 * Boot arguments definitions.
 */

/* #define BOOTARGS_COMMON "console=ttyS3,115200 mem=96M@0x0 rmem=32M@0x6000000"*/
#define CONFIG_BOOTARGS_AUTO_MODIFY	0	/*auto detect memory size, and modify bootargs for kernel.*/
#define CONFIG_BOOTARGS_MEM_64M			"mem=64M@0x0"	/* customize bootargs for default env.*/
#define CONFIG_BOOTARGS_MEM_128M		"mem=128M@0x0"
#define CONFIG_BOOTARGS_MEM_256M		"mem=256M@0x0"

#define BOOTARGS_COMMON " console=ttyS1,115200n8 mem=199M@0x0 rmem=1M@0xC700000 nmem=56M@0xC800000 "

#if defined(CONFIG_JZ_MMC_MSC0)
	#define MSC_BOOTARGS "rootfstype=ext4 root=/dev/mmcblk0p7 rootdelay=3 rw"
#elif defined(CONFIG_JZ_MMC_MSC2)
	#define MSC_BOOTARGS "rootfstype=ext4 root=/dev/mmcblk2p7 rootdelay=3 rw"
#endif

#ifdef CONFIG_BOOT_ANDROID
  #if defined(CONFIG_SPL_NOR_SUPPORT)
    #define CONFIG_BOOTARGS BOOTARGS_COMMON " ip=192.168.10.211:192.168.10.1:192.168.10.1:255.255.255.0 nfsroot=192.168.4.13:/home/nfsroot/fpga/user/wqshao/android/root rw noinitrd init=/init"
  #else
    #define CONFIG_BOOTARGS BOOTARGS_COMMON " ip=off root=/dev/ram0 rw rdinit=/init androidboot.hardware=dorado androidboot.selinux=permissive"
  #endif
#else
  #if defined(CONFIG_SPL_JZMMC_SUPPORT) || defined(CONFIG_SPL_MMC_SUPPORT)
	#define CONFIG_BOOTARGS BOOTARGS_COMMON MSC_BOOTARGS
  #elif defined(CONFIG_SPL_NOR_SUPPORT)
  /*#define CONFIG_BOOTARGS  BOOTARGS_COMMON " ip=192.168.10.210:192.168.10.1:192.168.10.1:255.255.255.0 nfsroot=192.168.4.13:/home/nfsroot/fpga/user/bliu/root_ok rw" */
    /*#define CONFIG_BOOTARGS  BOOTARGS_COMMON " ip=off root=/dev/ram0 rw rdinit=/linuxrc"*/
    #define CONFIG_BOOTARGS BOOTARGS_COMMON " ip=192.168.10.207:192.168.10.1:192.168.10.1:255.255.255.0 nfsroot=192.168.4.13:/home/nfsroot/fpga/user/pzqi/rootfs-tst rw"
  #elif defined(CONFIG_SPL_SFC_NOR)
	#define CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=jffs2 root=/dev/mtdblock2 rw flashtype=nor"
	/*#define CONFIG_BOOTARGS BOOTARGS_COMMON "init=/linuxrc rootfstype=squashfs root=/dev/mtdblock2 rw mtdparts=jz_sfc:256k(boot),2560k(kernel),2048k(root),-(appfs)"*/
  #elif defined(CONFIG_SPL_SFC_NAND)
	#define CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ubifs rw flashtype=nand"
	/*#define CONFIG_BOOTARGS BOOTARGS_COMMON "ip=off init=/linuxrc ubi.mtd=3 root=ubi0:rootfs ubi.mtd=4 rootfstype=ubifs rw"*/ /*dtb support*/
  #else
    #define CONFIG_BOOTARGS BOOTARGS_COMMON " ubi.mtd=1 root=ubi0:root rootfstype=ubifs rw"
  #endif
#endif



/**
 * Boot command definitions.
 */
#define CONFIG_BOOTDELAY 1

#ifdef CONFIG_BOOT_ANDROID
  #if defined(CONFIG_SPL_JZMMC_SUPPORT)
    #define CONFIG_BOOTCOMMAND "boota mmc 0 0x80f00000 6144"
    #define CONFIG_NORMAL_BOOT CONFIG_BOOTCOMMAND
    #define CONFIG_RECOVERY_BOOT "boota mmc 0 0x80f00000 24576"
  #elif defined(CONFIG_SPL_NOR_SUPPORT)
    #define CONFIG_BOOTCOMMAND "tftpboot 0x80600000 user/wqshao/android/uImage; bootm 0x80600000"
    #define CONFIG_NORMAL_BOOT CONFIG_BOOTCOMMAND
    #define CONFIG_RECOVERY_BOOT "tftpboot 0x80f00000 user/wqshao/android/uImage; bootm 0x80f00000"
  #else
    #define CONFIG_BOOTCOMMAND "nand_zm read ndboot 0 0x400000 0x80f00000;boota mem 0x80f00000"
    #define CONFIG_NORMAL_BOOT CONFIG_BOOTCOMMAND
    #define CONFIG_RECOVERY_BOOT "boota nand 0 0x80f00000 24576"
  #endif
#else  /* CONFIG_BOOT_ANDROID */
  #if defined(CONFIG_SPL_NOR_SUPPORT)
    #define CONFIG_BOOTCOMMAND "tftpboot 0x80600000 user/pzqi/uImage; bootm 0x80600000"
    /*#define CONFIG_BOOTCOMMAND "loady 0x80600000; bootm 0x80600000"*/
  #elif defined(CONFIG_SPL_JZMMC_SUPPORT) || defined(CONFIG_SPL_MMC_SUPPORT)
    #define CONFIG_BOOTCOMMAND "mmc dev 0; mmc read 0x80600000 0x1800 0x2800; bootm 0x80600000"
    /*#define CONFIG_BOOTCOMMAND "set dtb 0x83000000; set uImage 0x80600000; mmc dev 0;mmc read ${uImage} 0x1800 0x2800; mmc read ${dtb} 0x5800 0x100; bootm ${uImage} - ${dtb}"*/ /*dtb support*/
  #elif defined(CONFIG_SPL_SFC_NOR)
	#define CONFIG_BOOTCOMMAND "sfcnor read 0x40000 0x480000 0x80800000 ;bootm 0x80800000"
  #elif defined(CONFIG_SPL_SFC_NAND)
	#define CONFIG_BOOTCOMMAND "sfcnand read 0x100000 0x500000 0x80600000 ;bootm 0x80600000"
	/*#define CONFIG_BOOTCOMMAND "set uImage 0x80600000; set dtb 0x83000000; sfcnand read 0x100000 0x500000 $(uImage); sfcnand read 0x900000 0x20000 $(dtb); bootm $(uImage) - ${dtb}"*/  /*dtb support*/
  #else
    #define CONFIG_BOOTCOMMAND						\
	"mtdparts default; ubi part system; ubifsmount ubi:boot; "	\
	"ubifsload 0x80f00000 vmlinux.ub; bootm 0x80f00000"
  #endif
#endif /* CONFIG_BOOT_ANDROID */


#ifdef CONFIG_SPL_OS_BOOT
    #ifdef  CONFIG_SPL_SFC_NOR
	     #define CONFIG_SPL_BOOTARGS	BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=jffs2 root=/dev/mtdblock2 rw"
    #else
	     #define CONFIG_SPL_BOOTARGS	BOOTARGS_COMMON "ip=off init=/linuxrc ubi.mtd=2 root=ubi0:rootfs ubi.mtd=3 rootfstype=ubifs rw"
    #endif
    #ifdef CONFIG_OTA_VERSION30
		#define CONFIG_PAT_KERNEL_NAME	  "kernel"
		#define CONFIG_PAT_RECOVERY_NAME  "recovery"
		#define CONFIG_PAT_NV_NAME        "nv"
		#undef CONFIG_SPL_BOOTARGS
		#ifdef CONFIG_SPL_SFC_NOR
			#define CONFIG_PAT_USERFS_NAME   "userfs"
			#define CONFIG_PAT_UPDATEFS_NAME "updatefs"
            		#define CONFIG_SPL_BOOTARGS    BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock5 rw"
		#else
        		#define CONFIG_SPL_BOOTARGS    BOOTARGS_COMMON "ip=off init=/linuxrc ubi.mtd=4 root=ubi0:system ubi.mtd=5 rootfstype=ubifs ro"
        		#define CONFIG_SPL_OTA_BOOTARGS    BOOTARGS_COMMON "ip=off ubi.mtd=4 ubi.mtd=5 root=/dev/ram0 rw rdinit=/linuxrc"
		#endif
    #else
		#ifdef CONFIG_BOOT_VMLINUX
			#undef CONFIG_SPL_BOOTARGS
			#define CONFIG_SPL_BOOTARGS         BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock2 rw"
		#endif /* CONFIG_BOOT_VMLINUX */

        #define CONFIG_SOFT_BURNER
        #define CONFIG_AUDIO_CAL_DIV
        #define CONFIG_AUDIO_APLL CONFIG_SYS_APLL_FREQ
        #define CONFIG_AUDIO_MPLL CONFIG_SYS_MPLL_FREQ
    #endif /*CONFIG_OTA_VERSION30*/

     #ifdef CONFIG_BOOT_VMLINUX
             #undef CONFIG_SPL_BOOTARGS
             #define CONFIG_SPL_BOOTARGS         BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock2 rw"
     #endif /* CONFIG_BOOT_VMLINUX */

     #define CONFIG_SPL_OS_NAME        "kernel" /* spi offset of xImage being loaded */
     #define CONFIG_SYS_SPL_ARGS_ADDR    CONFIG_SPL_BOOTARGS
     #define CONFIG_SYS_SPL_OTA_ARGS_ADDR    CONFIG_SPL_OTA_BOOTARGS
     #define CONFIG_BOOTX_BOOTARGS       BOOTARGS_COMMON "ip=off init=/linuxrc rootfstype=cramfs root=/dev/mtdblock6 rw"
     #undef  CONFIG_BOOTCOMMAND
     #define CONFIG_BOOTCOMMAND    "bootx sfc 0x80f00000"
     #ifdef CONFIG_BOOT_RTOS
             #define CONFIG_LOAD_ADDR	0x80004000
     #else
             #define CONFIG_LOAD_ADDR	0x80001000
     #endif
#endif	/* CONFIG_SPL_OS_BOOT */


#define PARTITION_NUM 10


/**
 * Drivers configuration.
 */

/* MMC */
#ifdef CONFIG_JZ_MMC_MSC0
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_MMC_SPL_PARAMS
#define CONFIG_SDHCI
#define CONFIG_JZ_SDHCI
/*#define CONFIG_MMC_SDMA*/

/* MSC Command configuration */
#define CONFIG_CMD_MMC
#define CONFIG_SPL_JZ_MSC_BUS_4BIT

#define CONFIG_JZ_MMC_MSC0_PB_4BIT   //set gpio
/*#define CONFIG_MMC_TRACE		// only for DEBUG*/
/*#define CONFIG_SDHCI_TRACE	// only for DEBUG*/
#endif

#ifdef CONFIG_JZ_MMC_MSC1
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_MMC_SPL_PARAMS
#define CONFIG_JZ_SDHCI
/*#define CONFIG_MMC_SDMA*/

/* MSC Command configuration */
#define CONFIG_CMD_MMC

#define CONFIG_JZ_MMC_MSC1_PC   //set gpio
/*#define CONFIG_MMC_TRACE		// only for DEBUG*/
/*#define CONFIG_SDHCI_TRACE	// only for DEBUG*/
#endif


/* SFC */
#define CONFIG_SFC_V20

/* sfc gpio */
/* #define CONFIG_JZ_SFC_PD_4BIT */
/* #define CONFIG_JZ_SFC_PD_8BIT */
/* #define CONFIG_JZ_SFC_PD_8BIT_PULL */
#define CONFIG_JZ_SFC_PA

/* sfc ota config */
#ifdef CONFIG_OTA_VERSION30
#define CONFIG_KUNPENG_OTA_VERSION20
#endif

/* sfc nor config */
#ifdef CONFIG_SPL_SFC_NOR
#define CONFIG_JZ_SFC
#define CONFIG_CMD_SFC_NOR
#define CONFIG_JZ_SFC_NOR
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SFC_NOR_RATE	400000000	/* value <= 400000000(sfc 100Mhz)*/
#define CONFIG_SFC_QUAD
#define CONFIG_SPIFLASH_PART_OFFSET		0x5800
#define CONFIG_SPI_NORFLASH_PART_OFFSET		0x5874
#define CONFIG_NOR_MAJOR_VERSION_NUMBER		1
#define CONFIG_NOR_MINOR_VERSION_NUMBER		0
#define CONFIG_NOR_REVERSION_NUMBER		0
#define CONFIG_NOR_VERSION     (CONFIG_NOR_MAJOR_VERSION_NUMBER | (CONFIG_NOR_MINOR_VERSION_NUMBER << 8) | (CONFIG_NOR_REVERSION_NUMBER <<16))
/*#define CONFIG_NOR_BUILTIN_PARAMS*/
#define CONFIG_SPL_PAD_TO_BLOCK
#endif

/* sfc nand config */
#ifdef  CONFIG_SPL_SFC_NAND
#define CONFIG_SFC_NAND_RATE    400000000	/* value <= 400000000(sfc 100Mhz)*/
#define CONFIG_SFC_QUAD
#define CONFIG_SPI_SPL_CHECK
#define CONFIG_SPIFLASH_PART_OFFSET		0x5800
#define CONFIG_SPI_NAND_BPP                     (2048 +64)      /*Bytes Per Page*/
#define CONFIG_SPI_NAND_PPB                     (64)            /*Page Per Block*/
#define CONFIG_JZ_SFC
#define CONFIG_CMD_SFCNAND
#define CONFIG_CMD_NAND
#define CONFIG_SYS_MAX_NAND_DEVICE		1
#define CONFIG_SYS_NAND_BASE			0xb3441000
#define CONFIG_SYS_MAXARGS			16
/*#define CONFIG_NAND_BUILTIN_PARAMS*/
#define CONFIG_SPL_PAD_TO_BLOCK


/* sfc nand env config */
#define CONFIG_MTD_DEVICE
#define CONFIG_CMD_SAVEENV		/* saveenv */
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define MTDIDS_DEFAULT                  "nand0:nand"
#define MTDPARTS_DEFAULT                "mtdparts=nand:1M(boot),8M(kernel),40M(rootfs),-(data)"
#define CONFIG_SYS_NAND_BLOCK_SIZE	(128 * 1024)
#endif

#define CONFIG_SYS_NAND_SELF_INIT

/* end of sfc */

/* Ethernet: gmac*/

/* DEBUG ETHERNET */
#define CONFIG_SERVERIP		192.168.4.13
#define CONFIG_IPADDR		192.168.4.145
#define CONFIG_GATEWAYIP        192.168.4.1
#define CONFIG_NETMASK          255.255.255.0
#define CONFIG_ETHADDR          00:11:22:33:44:55


#define GMAC_PHY_MII    1//0
#define GMAC_PHY_RMII   2//4
#define GMAC_PHY_GMII   3//0
#define GMAC_PHY_RGMII  4//1
#define CONFIG_SYS_RX_ETH_BUFFER 64

#define CONFIG_NET_X2500
#ifdef CONFIG_NET_X2500

#define CONFIG_MAC_AHB_BUS

/* Select GMAC Controller */
#define CONFIG_GMAC0

/* Select GMAC Interface mode */

#define CONFIG_NET_GMAC_PHY_MODE GMAC_PHY_RMII

#ifdef CONFIG_GMAC0
#define CONFIG_GAMAC_MODE_CTRL_ADDR	0xb0000054
#define JZ_GMAC_BASE			0xb34b0000
#define CONFIG_GMAC_CRLT_PORT GPIO_PORT_B
#define CONFIG_GMAC_CRLT_PORT_PINS (0x7BF << 6)
#define CONFIG_GMAC_CRTL_PORT_INIT_FUNC GPIO_FUNC_0
#define CONFIG_GMAC_PHY_RESET	GPIO_PC(7)
#define CONFIG_GMAC_TX_CLK_DELAY 0x3f
#define CONFIG_GMAC_RX_CLK_DELAY 0
#endif

#define CONFIG_GMAC_CRTL_PORT_SET_FUNC GPIO_INPUT
#define CONFIG_GMAC_PHY_RESET_ENLEVEL	0
#endif /* CONFIG_NET_X2500 */
/* end of gmac */

/* GPIO */
#define CONFIG_JZ_GPIO

/**
 * Command configuration.
 */
#define CONFIG_CMD_BOOTD	/* bootd			*/
#define CONFIG_CMD_CONSOLE	/* coninfo			*/
#define CONFIG_CMD_DHCP 	/* DHCP support			*/
#define CONFIG_CMD_ECHO		/* echo arguments		*/
#define CONFIG_CMD_EXT4 	/* ext4 support			*/
#define CONFIG_CMD_FAT		/* FAT support			*/
#define CONFIG_USE_XYZMODEM	/* xyzModem 			*/
#define CONFIG_CMD_LOAD		/* serial load support 		*/
#define CONFIG_CMD_LOADB	/* loadb			*/
#define CONFIG_CMD_LOADS	/* loads			*/
#define CONFIG_CMD_MEMORY	/* md mm nm mw cp cmp crc base loop mtest */
#define CONFIG_CMD_MISC		/* Misc functions like sleep etc*/
#define CONFIG_CMD_NET		/* networking support			*/
#define CONFIG_CMD_PING
#define CONFIG_CMD_RUN		/* run command in env variable	*/
#define CONFIG_CMD_SETGETDCR	/* DCR support on 4xx		*/
#define CONFIG_CMD_SOURCE	/* "source" command support	*/
#define CONFIG_CMD_GETTIME
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FAT
#define CONFIG_EFI_PARTITION
/*#define CONFIG_SOFT_BURNER*/


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
#define CONFIG_LZMA


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
/* nand Environment variables */
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
#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-nor-spl.lds"
#else
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/$(SOC)/u-boot-spl.lds"
#endif
#define CONFIG_SPL_PAD_TO		24576 /* equal to spl max size */

#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
/* #define CONFIG_SPL_I2C_SUPPORT */
/* #define CONFIG_SPL_REGULATOR_SUPPORT */
/* #define CONFIG_SPL_CORE_VOLTAGE		1300 */
#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SPL_TEXT_BASE		0xba000000
#else
#define CONFIG_SPL_TEXT_BASE		0x80001000
#endif	/*CONFIG_SPL_NOR_SUPPORT*/
#define CONFIG_SPL_MAX_SIZE		(18 * 1024)




#ifdef CONFIG_SPL_NOR_SUPPORT
#define CONFIG_SYS_UBOOT_BASE		(CONFIG_SPL_TEXT_BASE + CONFIG_SPL_PAD_TO - 0x40)	//0x40 = sizeof (image_header)
#define CONFIG_SYS_OS_BASE		0
#define CONFIG_SYS_SPL_ARGS_ADDR	0
#define CONFIG_SYS_FDT_BASE		0
#endif

/* MMC  spl stage */
#if defined(CONFIG_SPL_MMC_SUPPORT) || defined(CONFIG_SPL_JZMMC_SUPPORT)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	82 /* 17k + 24k (17KB GPT offset and spl size CONFIG_SPL_PAD_TO)*/
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

#endif /*END OF __ZEBRA__*/
