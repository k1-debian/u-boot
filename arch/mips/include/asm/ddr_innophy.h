/*
 * JZ4780 ddr definitions
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
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

#ifndef __DDR_INNOPHY_H__
#define __DDR_INNOPHY_H__

#include <asm/arch/base.h>
#include <ddr/chips-v2/ddr_chip.h>
/*
 * DDR Controller Registers
 **/
#define DDR_MEM_PHY_BASE		0x20000000

#define CPM_DRCG			0xb00000d0

#ifdef CONFIG_X1600
#define DDRC_STATUS			0x0
#define DDRC_CFG			0x4
#define DDRC_CTRL			0x8
#define DDRC_LMR			0xc
#define DDRC_DLP			0xbc
#define DDRC_AUTOSR_EN			0x304
#define DDRC_AUTOSR_CNT			0x308
#define DDRC_REFCNT			0x18
#define DDRC_DBGINFO			0xE8
#define DDRC_TIMING(n)			(0x60 + 4 * (n - 1))
#define DDRC_MMAP0			0x24
#define DDRC_MMAP1			0x28
#define DDRC_HREGPRO			0xd8
#define DDRC_DWCFG			(DDRC_APB_OFFSET + 0x00)
#define DDRC_DWSTATUS			(DDRC_APB_OFFSET + 0x04)
#define DDRC_REMAP(n)			(0x9c + 4 * (n - 1))
#define DDRC_CGUC0			(DDRC_APB_OFFSET + 0x64)
#define DDRC_CGUC1			(DDRC_APB_OFFSET + 0x68)
#define DDRC_PREGPRO			(DDRC_APB_OFFSET + 0x6c)
#define DDRC_PHYRST_CFG			(DDRC_APB_OFFSET + 0x80)
#define DDRC_PHY_INIT			(DDRC_APB_OFFSET + 0x8c)
#elif defined(CONFIG_X2580)
#define DDRC_STATUS			0x0
#define DDRC_CFG			0x8
#define DDRC_CTRL			0x10
#define DDRC_LMR			0x18
#define DDRC_DLP			0x20
#define DDRC_AUTOSR_EN			0x28
#define DDRC_AUTOSR_CNT			0x30
#define DDRC_REFCNT			0x38
#define DDRC_DBGINFO			0xE8
#define DDRC_TIMING(n)			(0x40 + 8 * (n - 1))
#define DDRC_MMAP0			0x78
#define DDRC_MMAP1			0x80
#define DDRC_BWCFG			0x88
#define DDRC_BWSTP			0x90
#define DDRC_BWP0WR			0x98
#define DDRC_BWP1WR			0xa8
#define DDRC_BWP2WR			0xb8
#define DDRC_BWP3WR			0xc8
#define DDRC_HREGPRO			0xd8
#define DDRC_DBGEN	    	0xE0
#define DDRC_DBGINFO	   	0xE8
#define DDRC_DWCFG			(DDRC_APB_OFFSET + 0x00)
#define DDRC_DWSTATUS			(DDRC_APB_OFFSET + 0x04)
#define DDRC_REMAP(n)			(DDRC_APB_OFFSET + 0x08 + 4 * (n - 1))
#define DDRC_CPAC			(DDRC_APB_OFFSET + 0x1c)
#define DDRC_CCHC(n)		(DDRC_APB_OFFSET + 0x20 + 4 * (n - 1))
#define DDRC_CSCHC(n)		(DDRC_APB_OFFSET + 0x40 + 4 * (n - 1))
#define DDRC_CMONC(n)		(DDRC_APB_OFFSET + 0x50 + 4 * (n - 1))
#define DDRC_CGUC0			(DDRC_APB_OFFSET + 0x64)
#define DDRC_CGUC1			(DDRC_APB_OFFSET + 0x68)
#define DDRC_PREGPRO		(DDRC_APB_OFFSET + 0x6c)
#define DDRC_BUFCFG	    	(DDRC_APB_OFFSET + 0x70)
#elif defined(CONFIG_X2600) || defined(CONFIG_AD100)
#define DDRC_STATUS			0x0
#define DDRC_CFG			0x8
#define DDRC_CTRL			0x10
#define DDRC_LMR			0x18
#define DDRC_DLP			0x20
#define DDRC_AUTOSR_EN			0x28
#define DDRC_AUTOSR_CNT			0x30
#define DDRC_REFCNT			0x38
#define DDRC_DBGINFO			0xE8
#define DDRC_TIMING(n)			(0x40 + 8 * (n - 1))
#define DDRC_MMAP0			0x78
#define DDRC_MMAP1			0x80
#define DDRC_HREGPRO			0xd8
#define DDRC_DWCFG			(DDRC_APB_OFFSET + 0x00)
#define DDRC_DWSTATUS			(DDRC_APB_OFFSET + 0x04)
#define DDRC_REMAP(n)			(DDRC_APB_OFFSET + 0x08 + 4 * (n - 1))
#define DDRC_CGUC0			(DDRC_APB_OFFSET + 0x68)
#define DDRC_CGUC1			(DDRC_APB_OFFSET + 0x6C)
#define DDRC_PREGPRO			(DDRC_APB_OFFSET + 0x70)
#else
#define DDRC_STATUS			0x0
#define DDRC_CFG			0x8
#define DDRC_CTRL			0x10
#define DDRC_LMR			0x18
#define DDRC_DLP			0x20
#define DDRC_AUTOSR_EN			0x28
#define DDRC_AUTOSR_CNT			0x30
#define DDRC_REFCNT			0x38
#define DDRC_DBGINFO			0xE8
#define DDRC_TIMING(n)			(0x40 + 8 * (n - 1))
#define DDRC_MMAP0			0x78
#define DDRC_MMAP1			0x80
#define DDRC_HREGPRO			0xd8
#define DDRC_DWCFG			(DDRC_APB_OFFSET + 0x00)
#define DDRC_DWSTATUS			(DDRC_APB_OFFSET + 0x04)
#define DDRC_REMAP(n)			(DDRC_APB_OFFSET + 0x08 + 4 * (n - 1))
#define DDRC_CGUC0			(DDRC_APB_OFFSET + 0x64)
#define DDRC_CGUC1			(DDRC_APB_OFFSET + 0x68)
#define DDRC_PREGPRO			(DDRC_APB_OFFSET + 0x6c)
#endif

/*
 * DDR Innophy registers
 * */
#define DDRP_INNOPHY_PHY_RST		(DDR_PHY_OFFSET + 0x000)
#define DDRP_INNOPHY_MEM_CFG		(DDR_PHY_OFFSET + 0x004)
#ifdef CONFIG_X2580
#define DDRP_REG_OFFSET_RAINING_CTRL        (0x2)
#define DDRP_INNOPHY_TRAINING_CTRL  (DDR_PHY_OFFSET + DDRP_REG_OFFSET_RAINING_CTRL*4)
#define DDRP_INNOPHY_CALIB_DELAY_AL_RESULT1 (DDR_PHY_OFFSET + 0x26c)//0x9b left for cyc_dll result
#define DDRP_INNOPHY_CALIB_DELAY_AL_RESULT2 (DDR_PHY_OFFSET + 0x270)//0x9c left for dll and oph result
#define DDRP_INNOPHY_CALIB_DELAY_AH_RESULT1 (DDR_PHY_OFFSET + 0x274)//0x9d right for cyc_dll result
#define DDRP_INNOPHY_CALIB_DELAY_AH_RESULT2 (DDR_PHY_OFFSET + 0x278)//0x9e right for dll and oph result
#else
#define DDRP_INNOPHY_TRAINING_CTRL	(DDR_PHY_OFFSET + 0x008)
#define DDRP_INNOPHY_INNO_WR_LEVEL1	(DDR_PHY_OFFSET + 0x00c)
#define DDRP_INNOPHY_INNO_WR_LEVEL2	(DDR_PHY_OFFSET + 0x010)
#endif
/*
#define DDRP_INNOPHY_WL_MODE1		(DDR_PHY_OFFSET + 0x00c)
#define DDRP_INNOPHY_WL_MODE2		(DDR_PHY_OFFSET + 0x010)
*/


#define DDRP_INNOPHY_CL				(DDR_PHY_OFFSET + 0x014)
#define DDRP_INNOPHY_AL				(DDR_PHY_OFFSET + 0x018)
#define DDRP_INNOPHY_CWL			(DDR_PHY_OFFSET + 0x01c)


#if defined(CONFIG_X2500)
#define DDRP_INNOPHY_DQ_WIDTH		(DDR_PHY_OFFSET + 0x07c)
#define DDRP_INNOPHY_PLL_FBDIV		(DDR_PHY_OFFSET + 0x080)
#define DDRP_INNOPHY_PLL_CTRL		(DDR_PHY_OFFSET + 0x084)
#define DDRP_INNOPHY_PLL_PDIV		(DDR_PHY_OFFSET + 0x088)
#define DDRP_INNOPHY_WL_DONE		(DDR_PHY_OFFSET + 0x100)	//0x40
#define DDRP_INNOPHY_DLL_LOCK		(DDR_PHY_OFFSET + 0x104)	//0x41
#define DDRP_INNOPHY_PLL_LOCK		(DDR_PHY_OFFSET + 0x108)	//0x42
#define DDRP_INNOPHY_CALIB_DONE		(DDR_PHY_OFFSET + 0x10c)	//0x43
#define DDRP_INNOPHY_INIT_COMP		(DDR_PHY_OFFSET + 0x110)	//0x44
#define DDRP_INNOPHY_BIST_RES		(DDR_PHY_OFFSET + 0x118)	//0x46
#define DDRP_INNOPHY_CALIB_BYPASS_AL	(DDR_PHY_OFFSET + 0x158)	//0x56
#define DDRP_INNOPHY_CALIB_BYPASS_AH	(DDR_PHY_OFFSET + 0x198)	//0x66
#define DDRP_INNOPHY_CALIB_BYPASS_BL	(DDR_PHY_OFFSET + 0x218)	//0x86
#define DDRP_INNOPHY_CALIB_BYPASS_BH	(DDR_PHY_OFFSET + 0x258)	//0x96
#define DDRP_INNOPHY_CALIB_DELAY_AL	(DDR_PHY_OFFSET + 0x1d0)	//0x74
#define DDRP_INNOPHY_CALIB_DELAY_AH	(DDR_PHY_OFFSET + 0x1d4)	//0x75
#define DDRP_INNOPHY_CALIB_DELAY_BL	(DDR_PHY_OFFSET + 0x410)	//0x104
#define DDRP_INNOPHY_CALIB_DELAY_BH	(DDR_PHY_OFFSET + 0x414)	//0x105
#define DDRP_INNOPHY_RXDLL_DELAY_BL             (DDR_PHY_OFFSET + (0x88 << 2))
#define DDRP_INNOPHY_RXDLL_DELAY_BH             (DDR_PHY_OFFSET + (0x98 << 2))

#elif defined (CONFIG_X1600)
#define DDRP_INNOPHY_DQ_WIDTH		(DDR_PHY_OFFSET + 0x07c)
#define DDRP_INNOPHY_PLL_FBDIV		(DDR_PHY_OFFSET + 0x080)
#define DDRP_INNOPHY_PLL_CTRL		(DDR_PHY_OFFSET + 0x084)
#define DDRP_INNOPHY_PLL_PDIV		(DDR_PHY_OFFSET + 0x088)
#define DDRP_INNOPHY_WL_DONE		(DDR_PHY_OFFSET + 0x100)	//0x40
#define DDRP_INNOPHY_DLL_LOCK		(DDR_PHY_OFFSET + 0x104)	//0x41
#define DDRP_INNOPHY_PLL_LOCK		(DDR_PHY_OFFSET + 0x108)	//0x42
#define DDRP_INNOPHY_CALIB_DONE		(DDR_PHY_OFFSET + 0x10c)	//0x43
#define DDRP_INNOPHY_INIT_COMP		(DDR_PHY_OFFSET + 0x110)	//0x44
#define DDRP_INNOPHY_BIST_RES		(DDR_PHY_OFFSET + 0x118)	//0x46
#define DDRP_INNOPHY_CALIB_DELAY_AL	(DDR_PHY_OFFSET + 0x1d0)	//0x74
#define DDRP_INNOPHY_CALIB_DELAY_AH	(DDR_PHY_OFFSET + 0x1d4)	//0x75
#define DDRP_INNOPHY_CALIB_BYPASS_AL	(DDR_PHY_OFFSET + 0x158)	//0x56
#define DDRP_INNOPHY_CALIB_BYPASS_AH	(DDR_PHY_OFFSET + 0x198)	//0x66

#elif defined (CONFIG_X2580)
#define DDRP_INNOPHY_INNO_PHY_RST   (DDR_PHY_OFFSET + 0x000)
#define DDRP_INNOPHY_DQ_WIDTH       (DDR_PHY_OFFSET + 0x034)
#define DDRP_INNOPHY_PLL_FBDIVL     (DDR_PHY_OFFSET + 0x140)
#define DDRP_INNOPHY_PLL_FBDIVH     (DDR_PHY_OFFSET + 0x144)
#define DDRP_INNOPHY_PLL_PDIV       (DDR_PHY_OFFSET + 0x148)
#define DDRP_INNOPHY_PLL_CTRL       (DDR_PHY_OFFSET + 0x14c)
#define DDRP_REG_OFFSET_WL_DONE             (0x62)//not find in fpga config
#define DDRP_INNOPHY_WL_DONE        (DDR_PHY_OFFSET + DDRP_REG_OFFSET_WL_DONE*4)
#define DDRP_INNOPHY_PLL_LOCK       (DDR_PHY_OFFSET + 0x180)
#define DDRP_INNOPHY_CALIB_DONE     (DDR_PHY_OFFSET + 0x184)
#define DDRP_INNOPHY_INIT_COMP      (DDR_PHY_OFFSET + 0x110)

#elif defined (CONFIG_X2600) || defined(CONFIG_AD100)
#define DDRP_INNOPHY_DQ_WIDTH       (DDR_PHY_OFFSET + 0x034)
#define DDRP_INNOPHY_DQ_WIDTH_H     (DDR_PHY_OFFSET + 0x030)
#define DDRP_INNOPHY_WL_MODE1       (DDR_PHY_OFFSET + 0x00c)
#define DDRP_INNOPHY_WL_MODE2       (DDR_PHY_OFFSET + 0x010)
#define DDRP_INNOPHY_INVDELAYSEL_DQCMD (DDR_PHY_OFFSET + 0x020)
#define DDRP_INNOPHY_WL_L	    (DDR_PHY_OFFSET + 0x258)
#define DDRP_INNOPHY_WL_H	    (DDR_PHY_OFFSET + 0x25c)
#define DDRP_INNOPHY_PLL_FBDIV      (DDR_PHY_OFFSET + 0x140)	//0x50
#define DDRP_INNOPHY_PLL_LOCK       (DDR_PHY_OFFSET + 0x180)	//0x60
#define DDRP_INNOPHY_WL_DONE        (DDR_PHY_OFFSET + 0x188)
#define DDRP_INNOPHY_CALIB_DONE     (DDR_PHY_OFFSET + 0x184)	//0x61
#define DDRP_INNOPHY_CALIB_ERR     (DDR_PHY_OFFSET + 0x1a4)	//0x69
#define DDRP_INNOPHY_PLL_FBDIV_H    (DDR_PHY_OFFSET + 0x144)	//0x51
#define DDRP_INNOPHY_PLL_CTRL       (DDR_PHY_OFFSET + 0x14c)	//0x53
#define DDRP_INNOPHY_PLL_PDIV       (DDR_PHY_OFFSET + 0x148)	//0x52
#define DDRP_INNOPHY_CALIB_L_C	    (DDR_PHY_OFFSET + 0x26c)	//0x9b
#define DDRP_INNOPHY_CALIB_L_DO	    (DDR_PHY_OFFSET + 0x270)	//0x9c
#define DDRP_INNOPHY_CALIB_R_C	    (DDR_PHY_OFFSET + 0x274)	//0x9d
#define DDRP_INNOPHY_CALIB_R_DO	    (DDR_PHY_OFFSET + 0x278)	//0x9e
#define DDRP_INNOPHY_CALIB_MODE     (DDR_PHY_OFFSET + 0x054)
#define DDRP_INNOPHY_CALIB_BYPASS_AL_C  (DDR_PHY_OFFSET + 0x1c0)
#define DDRP_INNOPHY_CALIB_BYPASS_AL    (DDR_PHY_OFFSET + 0x1c4)
#define DDRP_INNOPHY_CALIB_BYPASS_AH_C  (DDR_PHY_OFFSET + 0x200)
#define DDRP_INNOPHY_CALIB_BYPASS_AH    (DDR_PHY_OFFSET + 0x204)
#define DDRP_INNOPHY_ZQ_CALIB_EN	(DDR_PHY_OFFSET + 0x50)
#define DDRP_INNOPHY_ZQ_CALIB_DONE	(DDR_PHY_OFFSET + 0x1ac)
#define DDRP_INNOPHY_ZQ_CALIB_AL	(DDR_PHY_OFFSET + 0x514)
#define DDRP_INNOPHY_ZQ_CALIB_AH	(DDR_PHY_OFFSET + 0x554)
#define DDRP_INNOPHY_ZQ_CALIB_CMD	(DDR_PHY_OFFSET + 0x4d0)
#define DDRP_INNOPHY_ZQ_CALIB_PD_DRV_6C	(DDR_PHY_OFFSET + 0x1b0)	//The calibration value of 40 Ohm pull down resistance.
#define DDRP_INNOPHY_ZQ_CALIB_PU_DRV_6D	(DDR_PHY_OFFSET + 0x1b4)	//The calibration value of 40 Ohm pull up resistance.
#define DDRP_INNOPHY_ZQ_CALIB_PD_ODT_6E	(DDR_PHY_OFFSET + 0x1b8)	//The calibration value of 160 Ohm pull down resistance.
#define DDRP_INNOPHY_ZQ_CALIB_PU_ODT_6F	(DDR_PHY_OFFSET + 0x1bc)	//The calibration value of 160 Ohm pull up resistance.
#define DDRP_INNOPHY_PD_DRV_CMD		(DDR_PHY_OFFSET + 0x4c0)	//0x130
#define DDRP_INNOPHY_PU_DRV_CMD		(DDR_PHY_OFFSET + 0x4c4)	//0x131
#define DDRP_INNOPHY_PD_DRV_CK		(DDR_PHY_OFFSET + 0x4c8)	//0x132
#define DDRP_INNOPHY_PU_DRV_CK		(DDR_PHY_OFFSET + 0x4cc)	//0x133
#define DDRP_INNOPHY_PD_ODT_DQ7_0	(DDR_PHY_OFFSET + 0x500)	//0x140
#define DDRP_INNOPHY_PU_ODT_DQ7_0	(DDR_PHY_OFFSET + 0x504)	//0x141
#define DDRP_INNOPHY_PD_DRV_DQ7_0	(DDR_PHY_OFFSET + 0x508)	//0x142
#define DDRP_INNOPHY_PU_DRV_DQ7_0	(DDR_PHY_OFFSET + 0x50c)	//0x143
#define DDRP_INNOPHY_PD_ODT_DQ15_8	(DDR_PHY_OFFSET + 0x540)	//0X150
#define DDRP_INNOPHY_PU_ODT_DQ15_8	(DDR_PHY_OFFSET + 0x544)	//0x151
#define DDRP_INNOPHY_PD_DRV_DQ15_8	(DDR_PHY_OFFSET + 0x548)	//0x152
#define DDRP_INNOPHY_PU_DRV_DQ15_8	(DDR_PHY_OFFSET + 0x54c)	//0x153
#define DDRP_INNOPHY_PBDS_A0		(DDR_PHY_OFFSET + 0xd00)
#define DDRP_INNOPHY_PBDS_A1		(DDR_PHY_OFFSET + 0xd04)
#define DDRP_INNOPHY_PBDS_A2		(DDR_PHY_OFFSET + 0xd08)
#define DDRP_INNOPHY_PBDS_A3		(DDR_PHY_OFFSET + 0xd0c)
#define DDRP_INNOPHY_PBDS_A4		(DDR_PHY_OFFSET + 0xd10)
#define DDRP_INNOPHY_PBDS_A5		(DDR_PHY_OFFSET + 0xd14)
#define DDRP_INNOPHY_PBDS_A6		(DDR_PHY_OFFSET + 0xd18)
#define DDRP_INNOPHY_PBDS_A7		(DDR_PHY_OFFSET + 0xd1c)
#define DDRP_INNOPHY_PBDS_A8		(DDR_PHY_OFFSET + 0xd20)
#define DDRP_INNOPHY_PBDS_A9		(DDR_PHY_OFFSET + 0xd24)
#define DDRP_INNOPHY_PBDS_A10		(DDR_PHY_OFFSET + 0xd28)
#define DDRP_INNOPHY_PBDS_A11		(DDR_PHY_OFFSET + 0xd2c)
#define DDRP_INNOPHY_PBDS_A12		(DDR_PHY_OFFSET + 0xd30)
#define DDRP_INNOPHY_PBDS_A13		(DDR_PHY_OFFSET + 0xd34)
#define DDRP_INNOPHY_PBDS_A14		(DDR_PHY_OFFSET + 0xd38)
#define DDRP_INNOPHY_PBDS_A15		(DDR_PHY_OFFSET + 0xd3c)
#define DDRP_INNOPHY_PBDS_WEB		(DDR_PHY_OFFSET + 0xd40)
#define DDRP_INNOPHY_PBDS_CASB		(DDR_PHY_OFFSET + 0xd44)
#define DDRP_INNOPHY_PBDS_BA0		(DDR_PHY_OFFSET + 0xd48)
#define DDRP_INNOPHY_PBDS_BA1		(DDR_PHY_OFFSET + 0xd4c)
#define DDRP_INNOPHY_PBDS_BA2		(DDR_PHY_OFFSET + 0xd50)
#define DDRP_INNOPHY_PBDS_BG1		(DDR_PHY_OFFSET + 0xd54)
#define DDRP_INNOPHY_PBDS_CKE		(DDR_PHY_OFFSET + 0xd58)
#define DDRP_INNOPHY_PBDS_CK0		(DDR_PHY_OFFSET + 0xd5c)
#define DDRP_INNOPHY_PBDS_CKB0		(DDR_PHY_OFFSET + 0xd60)
#define DDRP_INNOPHY_PBDS_CSB0		(DDR_PHY_OFFSET + 0xd64)
#define DDRP_INNOPHY_PBDS_ODT0		(DDR_PHY_OFFSET + 0xd68)
#define DDRP_INNOPHY_PBDS_RESETN	(DDR_PHY_OFFSET + 0xd6c)
#define DDRP_INNOPHY_PBDS_RASB		(DDR_PHY_OFFSET + 0xd70)
#define DDRP_INNOPHY_PBDS_CSB1		(DDR_PHY_OFFSET + 0xd78)
#define DDRP_INNOPHY_PBDS_ODT1		(DDR_PHY_OFFSET + 0xd7c)
#define DDRP_INNOPHY_PBDS_CKE1		(DDR_PHY_OFFSET + 0xdc0)
#define DDRP_INNOPHY_PBDS_CK1		(DDR_PHY_OFFSET + 0xdc4)
#define DDRP_INNOPHY_PBDS_CKB1		(DDR_PHY_OFFSET + 0xdc8)
#define DDRP_INNOPHY_PBDS_RX_DM0	(DDR_PHY_OFFSET + 0x700)
#define DDRP_INNOPHY_PBDS_TX_DM0	(DDR_PHY_OFFSET + 0x704)
#define DDRP_INNOPHY_PBDS_RX_DQ0	(DDR_PHY_OFFSET + 0x708)
#define DDRP_INNOPHY_PBDS_TX_DQ0	(DDR_PHY_OFFSET + 0x70c)
#define DDRP_INNOPHY_PBDS_RX_DQ1	(DDR_PHY_OFFSET + 0x710)
#define DDRP_INNOPHY_PBDS_TX_DQ1	(DDR_PHY_OFFSET + 0x714)
#define DDRP_INNOPHY_PBDS_RX_DQ2	(DDR_PHY_OFFSET + 0x718)
#define DDRP_INNOPHY_PBDS_TX_DQ2	(DDR_PHY_OFFSET + 0x71c)
#define DDRP_INNOPHY_PBDS_RX_DQ3	(DDR_PHY_OFFSET + 0x720)
#define DDRP_INNOPHY_PBDS_TX_DQ3	(DDR_PHY_OFFSET + 0x724)
#define DDRP_INNOPHY_PBDS_RX_DQ4	(DDR_PHY_OFFSET + 0x728)
#define DDRP_INNOPHY_PBDS_TX_DQ4	(DDR_PHY_OFFSET + 0x72c)
#define DDRP_INNOPHY_PBDS_RX_DQ5	(DDR_PHY_OFFSET + 0x730)
#define DDRP_INNOPHY_PBDS_TX_DQ5	(DDR_PHY_OFFSET + 0x734)
#define DDRP_INNOPHY_PBDS_RX_DQ6	(DDR_PHY_OFFSET + 0x738)
#define DDRP_INNOPHY_PBDS_TX_DQ6	(DDR_PHY_OFFSET + 0x73c)
#define DDRP_INNOPHY_PBDS_RX_DQ7	(DDR_PHY_OFFSET + 0x740)
#define DDRP_INNOPHY_PBDS_TX_DQ7	(DDR_PHY_OFFSET + 0x744)
#define DDRP_INNOPHY_PBDS_RX_DQS0	(DDR_PHY_OFFSET + 0x748)
#define DDRP_INNOPHY_PBDS_TX_DQS0	(DDR_PHY_OFFSET + 0x74c)
#define DDRP_INNOPHY_PBDS_TX_DQSB0	(DDR_PHY_OFFSET + 0x750)
#define DDRP_INNOPHY_PBDS_RX_DM1	(DDR_PHY_OFFSET + 0x754)
#define DDRP_INNOPHY_PBDS_TX_DM1	(DDR_PHY_OFFSET + 0x758)
#define DDRP_INNOPHY_PBDS_RX_DQ8	(DDR_PHY_OFFSET + 0x75c)
#define DDRP_INNOPHY_PBDS_TX_DQ8	(DDR_PHY_OFFSET + 0x760)
#define DDRP_INNOPHY_PBDS_RX_DQ9	(DDR_PHY_OFFSET + 0x764)
#define DDRP_INNOPHY_PBDS_TX_DQ9	(DDR_PHY_OFFSET + 0x768)
#define DDRP_INNOPHY_PBDS_RX_DQ10	(DDR_PHY_OFFSET + 0x76c)
#define DDRP_INNOPHY_PBDS_TX_DQ10	(DDR_PHY_OFFSET + 0x770)
#define DDRP_INNOPHY_PBDS_RX_DQ11	(DDR_PHY_OFFSET + 0x774)
#define DDRP_INNOPHY_PBDS_TX_DQ11	(DDR_PHY_OFFSET + 0x778)
#define DDRP_INNOPHY_PBDS_RX_DQ12	(DDR_PHY_OFFSET + 0x77c)
#define DDRP_INNOPHY_PBDS_TX_DQ12	(DDR_PHY_OFFSET + 0x780)
#define DDRP_INNOPHY_PBDS_RX_DQ13	(DDR_PHY_OFFSET + 0x784)
#define DDRP_INNOPHY_PBDS_TX_DQ13	(DDR_PHY_OFFSET + 0x788)
#define DDRP_INNOPHY_PBDS_RX_DQ14	(DDR_PHY_OFFSET + 0x78c)
#define DDRP_INNOPHY_PBDS_TX_DQ14	(DDR_PHY_OFFSET + 0x790)
#define DDRP_INNOPHY_PBDS_RX_DQ15	(DDR_PHY_OFFSET + 0x794)
#define DDRP_INNOPHY_PBDS_TX_DQ15	(DDR_PHY_OFFSET + 0x798)
#define DDRP_INNOPHY_PBDS_RX_DQS1	(DDR_PHY_OFFSET + 0x79c)
#define DDRP_INNOPHY_PBDS_TX_DQS1	(DDR_PHY_OFFSET + 0x7a0)
#define DDRP_INNOPHY_PBDS_TX_DQSB1	(DDR_PHY_OFFSET + 0x7a4)
#define DDRP_INNOPHY_PBDS_RX_DQSB0	(DDR_PHY_OFFSET + 0x7a8)
#define DDRP_INNOPHY_PBDS_RX_DQSB1	(DDR_PHY_OFFSET + 0x7ac)

#else
#define DDRP_INNOPHY_DQ_WIDTH		(DDR_PHY_OFFSET + 0x07c)
#define DDRP_INNOPHY_PLL_FBDIV		(DDR_PHY_OFFSET + 0x080)
#define DDRP_INNOPHY_PLL_CTRL		(DDR_PHY_OFFSET + 0x084)
#define DDRP_INNOPHY_PLL_PDIV		(DDR_PHY_OFFSET + 0x088)
#define DDRP_INNOPHY_WL_DONE		(DDR_PHY_OFFSET + 0x0c0)	//0x30
#define DDRP_INNOPHY_PLL_LOCK		(DDR_PHY_OFFSET + 0x0c8)	//0x32
#define DDRP_INNOPHY_CALIB_DONE		(DDR_PHY_OFFSET + 0x0cc)	//0x33
#define DDRP_INNOPHY_CALIB_BYPASS_AL	(DDR_PHY_OFFSET + 0x118)	//0x46
#define DDRP_INNOPHY_CALIB_BYPASS_AH	(DDR_PHY_OFFSET + 0x158)	//0x56
#define DDRP_INNOPHY_CALIB_DELAY_AL	(DDR_PHY_OFFSET + 0x190)	//0x64
#define DDRP_INNOPHY_CALIB_DELAY_AH	(DDR_PHY_OFFSET + 0x194)	//0x65
#define DDRP_INNOPHY_INIT_COMP		(DDR_PHY_OFFSET + 0x0d0)	//0x34

#endif


#if defined(CONFIG_X1600)
#define DDRP_INNOPHY_RXDLL_DELAY_AL		(DDR_PHY_OFFSET + (0x58 << 2))
#define DDRP_INNOPHY_RXDLL_DELAY_AH		(DDR_PHY_OFFSET + (0x68 << 2))
#elif defined (CONFIG_X2600) || defined(CONFIG_AD100)
#define DDRP_INNOPHY_RXDLL_DELAY_AL     (DDR_PHY_OFFSET + (0x15 << 2))
#define DDRP_INNOPHY_RXDLL_DELAY_AH     (DDR_PHY_OFFSET + (0x16 << 2))
#else
#define DDRP_INNOPHY_RXDLL_DELAY_AL		(DDR_PHY_OFFSET + (0x48 << 2))
#define DDRP_INNOPHY_RXDLL_DELAY_AH		(DDR_PHY_OFFSET + (0x58 << 2))
#endif

#ifdef CONFIG_X2580
#define DDRP_REG_OFFSET_RAINING_CTRL        (0x2)
#define DDRP_INNOPHY_PLL_FBDIVL		(DDR_PHY_OFFSET + 0x140)
#define DDRP_INNOPHY_PLL_FBDIVH		(DDR_PHY_OFFSET + 0x144)
#define DDRP_INNOPHY_DQ_WIDTH		(DDR_PHY_OFFSET + 0x034)
#define DDRP_INNOPHY_PLL_PDIV		(DDR_PHY_OFFSET + 0x148)
#define DDRP_INNOPHY_PLL_CTRL		(DDR_PHY_OFFSET + 0x14c)
#define DDRP_INNOPHY_PLL_LOCK		(DDR_PHY_OFFSET + 0x180)
#define DDRP_INNOPHY_CALIB_DONE		(DDR_PHY_OFFSET + 0x184)
#define DDRP_INNOPHY_WL_MODE1		(DDR_PHY_OFFSET + 0x00c)
#define DDRP_INNOPHY_WL_MODE2		(DDR_PHY_OFFSET + 0x010)
#define DDRP_INNOPHY_WL_DONE		(DDR_PHY_OFFSET + DDRP_REG_OFFSET_WL_DONE*4)
#define DDRP_INNOPHY_TRAINING_2c	(DDR_PHY_OFFSET + 0x0b0)
#define DDRP_INNOPHY_TRAINING_3c	(DDR_PHY_OFFSET + 0x0f0)
#define DDRP_INNOPHY_TRAINING_4c	(DDR_PHY_OFFSET + 0x130)
#define DDRP_INNOPHY_TRAINING_5c	(DDR_PHY_OFFSET + 0x170)
#define DDRP_INNOPHY_TRAINING_CTRL  (DDR_PHY_OFFSET + DDRP_REG_OFFSET_RAINING_CTRL*4)
#define DDRP_INNOPHY_CALIB_DELAY_AL_RESULT1 (DDR_PHY_OFFSET + 0x26c)//0x9b left for cyc_dll result
#define DDRP_INNOPHY_CALIB_DELAY_AL_RESULT2 (DDR_PHY_OFFSET + 0x270)//0x9c left for dll and oph result
#define DDRP_INNOPHY_CALIB_DELAY_AH_RESULT1 (DDR_PHY_OFFSET + 0x274)//0x9d right for cyc_dll result
#define DDRP_INNOPHY_CALIB_DELAY_AH_RESULT2 (DDR_PHY_OFFSET + 0x278)//0x9e right for dll and oph result
#endif

/*
 * DDRC REGISTER BITS DEFINE
 * */

/* DDRC Status Register */
#define DDRC_DSTATUS_MISS	(1 << 6)
#define DDRC_ST_DPDN		(1 << 5) /* 0 DDR memory is NOT in deep-power-down state
					    1 DDR memory is in deep-power-down state */
#define DDRC_ST_PDN		(1 << 4) /* 0 DDR memory is NOT in power-down state
					    1 DDR memory is in power-down state */
#define DDRC_ST_AREF		(1 << 3) /* 0 DDR memory is NOT in auto-refresh state
					    1 DDR memory is in auto-refresh state */
#define DDRC_ST_SREF		(1 << 2) /* 0 DDR memory is NOT in self-refresh state
					    1 DDR memory is in self-refresh state */
#define DDRC_ST_CKE1		(1 << 1) /* 0 CKE1 Pin is low
					    1 CKE1 Pin is high */
#define DDRC_ST_CKE0		(1 << 0) /* 0 CKE0 Pin is low
					    1 CKE0 Pin is high */

/* DDRC Configure Register */
#define DDRC_CFG_ROW1_BIT	29 /* Row Address width. */
#define DDRC_CFG_ROW1_MASK	(0x7 << DDRC_CFG_ROW1_BIT)
#define DDRC_CFG_COL1_BIT	26 /* Row Address width. */
#define DDRC_CFG_COL1_MASK	(0x7 << DDRC_CFG_COL1_BIT)
#define DDRC_CFG_BA1		25 /* Bank Address width of DDR memory */
#define DDRC_CFG_IMBA		(1 << 16)
#define DDRC_CFG_ROW0_BIT	13 /* Row Address width. */
#define DDRC_CFG_ROW0_MASK	(0x7 << DDRC_CFG_ROW0_BIT)
#define DDRC_CFG_COL0_BIT	10 /* Row Address width. */
#define DDRC_CFG_COL0_MASK	(0x7 << DDRC_CFG_COL1_BIT)
#define DDRC_CFG_BA0		9 /* Bank Address width of DDR memory */

#define DDRC_CFG_TYPE_BIT	3
#define DDRC_CFG_TYPE_MASK	(0x7 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR1	(2 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_MDDR	(3 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR2	(4 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_LPDDR2	(5 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR3	(6 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_LPDDR3	(7 << DDRC_CFG_TYPE_BIT)

#define DDRC_CFG_ODTEN		(1 << 2)  /* ODT EN */
#define DDRC_CFG_CS1EN		(1 << 1)  /* DDR Chip-Select-1 Enable */
#define DDRC_CFG_CS0EN		(1 << 0)  /* DDR Chip-Select-0 Enable */

/* DDRC Control Register */
#define DDRC_CTRL_DFI_RST	(1 << 23)
#define DDRC_CTRL_DLL_RST	(1 << 22)
#define DDRC_CTRL_CTL_RST	(1 << 21)
#define DDRC_CTRL_CFG_RST	(1 << 20)
#define DDRC_CTRL_ACTPD		(1 << 15) /* 0 Precharge all banks before entering power-down
					     1 Do not precharge banks before entering power-down */
#define DDRC_CTRL_PDT_BIT	12 /* Power-Down Timer */
#define DDRC_CTRL_PDT_MASK	(0x7 << DDRC_CTRL_PDT_BIT)
#define DDRC_CTRL_PDT_DIS	(0 << DDRC_CTRL_PDT_BIT) /* power-down disabled */
#define DDRC_CTRL_PDT_8		(1 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 8 tCK idle */
#define DDRC_CTRL_PDT_16	(2 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 16 tCK idle */
#define DDRC_CTRL_PDT_32	(3 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 32 tCK idle */
#define DDRC_CTRL_PDT_64	(4 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 64 tCK idle */
#define DDRC_CTRL_PDT_128	(5 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 128 tCK idle */

#define DDRC_CTRL_ACTSTP	(1 << 11)
#define DDRC_CTRL_PD_CCE	(1 << 7) /* Power down clk freq change enable */
#define DDRC_CTRL_DPD		(1 << 6) /* 1 Drive external MDDR device entering Deep-Power-Down mode */
#define DDRC_CTRL_SR		(1 << 5) /* 1 Drive external DDR device entering self-refresh mode
					    0 Drive external DDR device exiting self-refresh mode */
#define DDRC_CTRL_SR_CCE	(1 << 4) /* Self refresh clk stop request enable */
#define DDRC_CTRL_CKE		(1 << 1) /* 0 Not set CKE Pin High, 1 Set CKE Pin HIGH */
#define DDRC_CTRL_RESET		(1 << 0) /* 0 End resetting ddrc_controller, 1 Resetting ddrc_controller */

/* DDRC DFI low power handshake control register */
#define DDRC_DDLP_TCTLUDP_BIT	24
#define DDRC_DDLP_TCTLUDP_FF	(0xff << DDRC_DDLP_TCTLUDP_BIT)

#define DDRC_DDLP_FSR		(1 << 2)
#define DDRC_DDLP_FPD		(1 << 1)
#define DDRC_DDLP_LPEN		(1 << 0)

/* DDRC Load-Mode-Register */
#define DDRC_LMR_DDR_ADDR_BIT	12 /* When performing a DDR command, DDRC_ADDR[13:0]
					      corresponding to external DDR address Pin A[13:0] */
#define DDRC_LMR_DDR_ADDR_MASK	(0xfffff << DDRC_LMR_DDR_ADDR_BIT)

#define DDRC_LMR_MA_BIT		16 /* FOR LPDDR2, MA[9:0] */
#define DDRC_LMR_OP_BIT		24 /* FOR LPDDR2, OP[9:0] */

#define DDRC_LMR_BA_BIT		9 /* When performing a DDR command, BA[2:0]
				     corresponding to external DDR address Pin BA[2:0]. */
#define DDRC_LMR_BA_MASK	(0x7 << DDRC_LMR_BA_BIT)
#if defined (CONFIG_X1600)
#define DDRC_LMR_CMD_BIT	3
#else
#define DDRC_LMR_CMD_BIT	6
#endif
#define DDRC_LMR_CMD_MASK	(0x7 << DDRC_LMR_CMD_BIT)
#define DDRC_LMR_CMD_PREC	(0 << DDRC_LMR_CMD_BIT)/* Precharge one bank/All banks */
#define DDRC_LMR_CMD_AUREF	(1 << DDRC_LMR_CMD_BIT)/* Auto-Refresh */
#define DDRC_LMR_CMD_LMR	(2 << DDRC_LMR_CMD_BIT)/* Load Mode Register */
#define DDRC_LMR_CMD_ZQCL_CS0	(3 << DDRC_LMR_CMD_BIT)/* ZQCL for DDR3 on CS0 */
#define DDRC_LMR_CMD_ZQCL_CS1	(4 << DDRC_LMR_CMD_BIT)/* ZQCL for DDR3 on CS1 */
#define DDRC_LMR_CMD_ZQCS_CS0	(5 << DDRC_LMR_CMD_BIT)/* ZQCS for DDR3 on CS0 */
#define DDRC_LMR_CMD_ZQCS_CS1	(6 << DDRC_LMR_CMD_BIT)/* ZQCS for DDR3 on CS1 */

#define DDRC_LMR_TMRD_BIT	1
#define DDRC_LMR_TMRD_MASK	(0x1f << DDRC_LMR_TMRD_BIT)
#define DDRC_LMR_START		(1 << 0) /* 0 No command is performed
						    1 On the posedge of START, perform a command
						    defined by CMD field */

/* DDRC  Auto-Refresh Counter */
#define DDRC_REFCNT_REF_EN		(1 << 0) /* Enable Refresh Counter */
#define DDRC_REFCNT_CLK_DIV_BIT		1  /* Clock Divider for auto-refresh counter. */
#define DDRC_REFCNT_CLK_DIV_MASK	(0x7 << DDRC_REFCNT_CLKDIV_BIT)

#define DDRC_REFCNT_PREREF_CNT_BIT	4
#define DDRC_REFCNT_PREREF_CNT_MASK	(0xf << DDRC_REFCNT_PREREF_CNT_BIT)
#define DDRC_REFCNT_PREREF_CNT(val)	(val << DDRC_REFCNT_PREREF_CNT_BIT)
#define DDRC_REFCNT_PREREF_CNT_DEFAULT	DDRC_REFCNT_PREREF_CNT(8)

#define DDRC_REFCNT_CNT_BIT		8  /* 8-bit counter */
#define DDRC_REFCNT_CNT_MASK		(0xff << DDRC_REFCNT_CNT_BIT)

#define DDRC_REFCNT_CON_BIT		16 /* Constant value used to compare with CNT value. */
#define DDRC_REFCNT_CON_MASK		(0xff << DDRC_REFCNT_CON_BIT)

#define DDRC_REFCNT_TRFC_BIT		24
#define DDRC_REFCNT_TRFC_MASK		(0x3f << DDRC_REFCNT_TRFC_BIT)

#define DDRC_REFCNT_PREREF_EN_BIT	30
#define DDRC_REFCNT_PREREF_EN		(1 << DDRC_REFCNT_PREREF_EN_BIT)

#define DDRC_REFCNT_PBREF_EN_BIT	31
#define DDRC_REFCNT_PBREF_EN		(1 << DDRC_REFCNT_PBREF_EN_BIT)

/* DDRC Memory Map Config Register */
#define DDRC_MMAP_BASE_BIT		8 /* base address */
#define DDRC_MMAP_BASE_MASK		(0xff << DDRC_MMAP_BASE_BIT)
#define DDRC_MMAP_MASK_BIT		0 /* address mask */
#define DDRC_MMAP_MASK_MASK		(0xff << DDRC_MMAP_MASK_BIT)

#define DDRC_MMAP0_BASE			(0x20 << DDRC_MMAP_BASE_BIT)
#define DDRC_MMAP1_BASE_64M		(0x24 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/
#define DDRC_MMAP1_BASE_128M		(0x28 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/
#define DDRC_MMAP1_BASE_256M		(0x30 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/

#define DDRC_MMAP_MASK_64_64		(0xfc << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/
#define DDRC_MMAP_MASK_128_128		(0xf8 << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/
#define DDRC_MMAP_MASK_256_256		(0xf0 << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/

/* DDR device data width configure register */
#define DDRC_DWCFG_DFI_INIT_START	(1 << 3)

/* DDR device status register */
#define DDRC_DWSTATUS_DFI_INIT_COMP	(1 << 0)

/* DDRC AHB Bus Register Protection Register */
#define DDRC_HREGPRO_HPRO_EN		(1 << 0)

/* DDRC APB Bus Register Protection Register */
#define DDRC_PREGPRO_PPRO_EN		(1 << 0)


/* DDRC clock gate unit configure 0 */
#define DDRC_CGU_PORT7		(1 << 28)
#define DDRC_CGU_PORT6		(1 << 24)
#define DDRC_CGU_PORT5		(1 << 20)
#define DDRC_CGU_PORT4		(1 << 16)
#define DDRC_CGU_PORT3		(1 << 12)
#define DDRC_CGU_PORT2		(1 << 8)
#define DDRC_CGU_PORT1		(1 << 4)
#define DDRC_CGU_PORT0		(1 << 0)

/* DDRC clock gate unit configure 1 */
#define DDRC_CGU_BWM		(1 << 8)
#define DDRC_CGU_PCTRL		(1 << 4)
#define DDRC_CGU_SCH		(1 << 1)
#define DDRC_CGU_PA		(1 << 0)



/*
 * DDR INNOPHY REGISTER BITS DEFINE
 * */

/* DDRP DQ Width Register */
#define DDRP_DQ_WIDTH_DQ_H		(1 << 1)
#define DDRP_DQ_WIDTH_DQ_L		(1 << 0)

/* DDRP Pll Ctrl Register */
#define DDRP_PLL_CTRL_PLLPDEN		(1 << 1)

/* DDRP Training Ctrl Register */
#define DDRP_TRAINING_CTRL_WL_BP	(1 << 3)
#define DDRP_TRAINING_CTRL_WL_START	(1 << 2)
#define DDRP_TRAINING_CTRL_DSCSE_BP	(1 << 1)
#define DDRP_TRAINING_CTRL_DSACE_START	(1 << 0)

/* DDRP Training Done Register */
#define DDRP_CALIB_DONE_HDQCFB 		(1 << 3)
#define DDRP_CALIB_DONE_LDQCFB 		(1 << 2)
#define DDRP_CALIB_DONE_HDQCFA 		(1 << 1)
#define DDRP_CALIB_DONE_LDQCFA 		(1 << 0)
#define DDRP_CALIB_DONE_HWRLFB 		(1 << 3)
#define DDRP_CALIB_DONE_LWRLFB 		(1 << 2)
#define DDRP_CALIB_DONE_HWRLFA 		(1 << 1)
#define DDRP_CALIB_DONE_LWRLFA 		(1 << 0)

/* DDRP CALIB BP Register */
#define DDRP_CALIB_BP_CYCLESELBH_BIT	4
#define DDRP_CALIB_BP_OPHCSELBH_BIT	3
#define DDRP_CALIB_BP_DLLSELBH_BIT	0

/* DDRP Init Complete Register */
#define DDRP_INIT_COMP			(1 << 0)

#define DDRP_PLL_LOCK			(1 << 3)

#define ddr_writel(value, reg)	writel((value), DDRC_BASE + reg)
#define ddr_readl(reg)		readl(DDRC_BASE + reg)

extern unsigned int __ps_per_tck;

#define DDR_SELECT_MAX__tCK_ps(tck, ps)						\
({										\
	unsigned int value;							\
	value = (tck * __ps_per_tck > ps) ? (tck * __ps_per_tck) : ps;   	\
	value;									\
})

#define DDR__ns(ns)   (ns * 1000)
#define DDR__ps(ps)   (ps)
#define DDR__tck(tck) ((tck) * __ps_per_tck)

#ifdef CONFIG_DDR_INNOPHY
struct ddr_reg_header {
	unsigned char name[32];
	unsigned int id;	/*DDR ID: */
	unsigned int type;	/*DDR type*/
	unsigned int freq;	/*ddr freqency.*/
};
#ifdef CONFIG_X1600
struct ddr_reg_value {
	struct ddr_reg_header h;
	unsigned int DDRC_CFG_VALUE;
	unsigned int DDRC_CTRL_VALUE;
	unsigned int DDRC_DLMR_VALUE;
	unsigned int DDRC_DDLP_VALUE;
	unsigned int DDRC_MMAP0_VALUE;
	unsigned int DDRC_MMAP1_VALUE;
	unsigned int DDRC_REFCNT_VALUE;
	unsigned int DDRC_TIMING1_VALUE;
	unsigned int DDRC_TIMING2_VALUE;
	unsigned int DDRC_TIMING3_VALUE;
	unsigned int DDRC_TIMING4_VALUE;
	unsigned int DDRC_TIMING5_VALUE;
	unsigned int DDRC_TIMING6_VALUE;
	unsigned int DDRC_AUTOSR_EN_VALUE;
	unsigned int DDRP_MEMCFG_VALUE;
	unsigned int DDRP_CL_VALUE;
	unsigned int DDRP_CWL_VALUE;
	unsigned int DDR_MR0_VALUE;
	unsigned int DDR_MR1_VALUE;
	unsigned int DDR_MR2_VALUE;
	unsigned int DDR_MR3_VALUE;
	unsigned int DDR_MR10_VALUE;
	unsigned int DDR_MR11_VALUE;
	unsigned int DDR_MR63_VALUE;
	unsigned int DDR_CHIP_0_SIZE;
	unsigned int DDR_CHIP_1_SIZE;
	unsigned int REMMAP_ARRAY[5];
        struct phy_drvodt_config phy_drvodt;
        struct phy_deskew_config phy_deskew;
};
#elif defined(CONFIG_X2600) ||defined(CONFIG_AD100)
struct ddr_reg_value {
	struct ddr_reg_header h;
	unsigned int DDRC_CFG_VALUE;
	unsigned int DDRC_CTRL_VALUE;
	unsigned int DDRC_DLMR_VALUE;
	unsigned int DDRC_DDLP_VALUE;
	unsigned int DDRC_MMAP0_VALUE;
	unsigned int DDRC_MMAP1_VALUE;
	unsigned int DDRC_REFCNT_VALUE;
	unsigned int DDRC_TIMING1_VALUE;
	unsigned int DDRC_TIMING2_VALUE;
	unsigned int DDRC_TIMING3_VALUE;
	unsigned int DDRC_TIMING4_VALUE;
	unsigned int DDRC_TIMING5_VALUE;
	unsigned int DDRC_AUTOSR_CNT_VALUE;
	unsigned int DDRC_AUTOSR_EN_VALUE;
	unsigned int DDRC_HREGPRO_VALUE;
	unsigned int DDRC_PREGPRO_VALUE;
	unsigned int DDRC_CGUC0_VALUE;
	unsigned int DDRC_CGUC1_VALUE;
	unsigned int DDRP_MEMCFG_VALUE;
	unsigned int DDRP_CL_VALUE;
	unsigned int DDRP_CWL_VALUE;
	unsigned int DDR_MR0_VALUE;
	unsigned int DDR_MR1_VALUE;
	unsigned int DDR_MR2_VALUE;
	unsigned int DDR_MR3_VALUE;
	unsigned int DDR_MR10_VALUE;
	unsigned int DDR_MR11_VALUE;
	unsigned int DDR_MR63_VALUE;
	unsigned int DDR_CHIP_0_SIZE;
	unsigned int DDR_CHIP_1_SIZE;
	unsigned int REMMAP_ARRAY[6];
        struct phy_drvodt_config phy_drvodt;
        struct phy_deskew_config phy_deskew;
};
#else
struct ddr_reg_value {
	struct ddr_reg_header h;
	unsigned int DDRC_CFG_VALUE;
	unsigned int DDRC_CTRL_VALUE;
	unsigned int DDRC_DLMR_VALUE;
	unsigned int DDRC_DDLP_VALUE;
	unsigned int DDRC_MMAP0_VALUE;
	unsigned int DDRC_MMAP1_VALUE;
	unsigned int DDRC_REFCNT_VALUE;
	unsigned int DDRC_TIMING1_VALUE;
	unsigned int DDRC_TIMING2_VALUE;
	unsigned int DDRC_TIMING3_VALUE;
	unsigned int DDRC_TIMING4_VALUE;
	unsigned int DDRC_TIMING5_VALUE;
	unsigned int DDRC_AUTOSR_CNT_VALUE;
	unsigned int DDRC_AUTOSR_EN_VALUE;
	unsigned int DDRC_HREGPRO_VALUE;
	unsigned int DDRC_PREGPRO_VALUE;
	unsigned int DDRC_CGUC0_VALUE;
	unsigned int DDRC_CGUC1_VALUE;
	unsigned int DDRP_MEMCFG_VALUE;
	unsigned int DDRP_CL_VALUE;
	unsigned int DDRP_CWL_VALUE;
	unsigned int DDR_MR0_VALUE;
	unsigned int DDR_MR1_VALUE;
	unsigned int DDR_MR2_VALUE;
	unsigned int DDR_MR3_VALUE;
	unsigned int DDR_MR10_VALUE;
	unsigned int DDR_MR11_VALUE;
	unsigned int DDR_MR63_VALUE;
	unsigned int DDR_CHIP_0_SIZE;
	unsigned int DDR_CHIP_1_SIZE;
	unsigned int REMMAP_ARRAY[5];
        struct phy_drvodt_config phy_drvodt;
        struct phy_deskew_config phy_deskew;
};
#endif

extern struct ddr_reg_value *g_ddr_param;
#endif

struct jzsoc_ddr_hook
{
	void (*prev_ddr_init)(enum ddr_type type);
	void (*post_ddr_init)(enum ddr_type type);
};
void register_ddr_hook(struct jzsoc_ddr_hook * hook);
#endif /* __DDR_H__ */
