/*
 * DDR driver for Synopsys DWC DDR PHY.
 * Used by Jz4775, JZ4780...
 *
 * Copyright (C) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
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
/* #define DEBUG_READ_WRITE */
#include <config.h>
#include <common.h>
#include <ddr/ddr_common.h>
#ifndef CONFIG_BURNER
//#include <generated/ddr_reg_values.h>
extern struct ddr_reg_value supported_ddr_reg_values[];
#endif

#include <asm/io.h>
#include <asm/arch/clk.h>

/*#define CONFIG_DWC_DEBUG 0*/
#define ddr_hang() do{						\
		debug("%s %d\n",__FUNCTION__,__LINE__);	\
		hang();						\
	}while(0)

DECLARE_GLOBAL_DATA_PTR;
extern struct ddr_reg_value *global_reg_value __attribute__ ((section(".data")));

#ifdef  CONFIG_DWC_DEBUG
#define FUNC_ENTER() debug("%s enter.\n",__FUNCTION__);
#define FUNC_EXIT() debug("%s exit.\n",__FUNCTION__);


static void dump_ddrp_register(void)
{
	debug("DDRP_INNOPHY_PHY_RST		0x%x\n", ddr_readl(DDRP_INNOPHY_PHY_RST));
	debug("DDRP_INNOPHY_MEM_CFG		0x%x\n", ddr_readl(DDRP_INNOPHY_MEM_CFG));
	debug("DDRP_INNOPHY_DQ_WIDTH		0x%x\n", ddr_readl(DDRP_INNOPHY_DQ_WIDTH));
	debug("DDRP_INNOPHY_CL			0x%x\n", ddr_readl(DDRP_INNOPHY_CL));
	debug("DDRP_INNOPHY_CWL		0x%x\n", ddr_readl(DDRP_INNOPHY_CWL));
	debug("DDRP_INNOPHY_PLL_FBDIV		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_FBDIV));
	debug("DDRP_INNOPHY_PLL_CTRL		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_CTRL));
	debug("DDRP_INNOPHY_PLL_PDIV		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_PDIV));
	debug("DDRP_INNOPHY_PLL_LOCK		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK));
	debug("DDRP_INNOPHY_TRAINING_CTRL	0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_CTRL));
	debug("DDRP_INNOPHY_CALIB_DONE		0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
	debug("DDRP_INNOPHY_CALIB_DELAY_AL	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL));
	debug("DDRP_INNOPHY_CALIB_DELAY_AH	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH));
	debug("DDRP_INNOPHY_CALIB_BYPASS_AL	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL));
	debug("DDRP_INNOPHY_CALIB_BYPASS_AH	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH));
	debug("DDRP_INNOPHY_INIT_COMP		0x%x\n", ddr_readl(DDRP_INNOPHY_INIT_COMP));
}


#else
#define FUNC_ENTER()
#define FUNC_EXIT()

#define dump_ddrc_register()
#define dump_ddrp_register()
#endif






static void ddrp_set_dq_odt(unsigned int pu, unsigned int pd)
{
	ddr_writel(pu, DDRP_INNOPHY_PU_ODT_DQ7_0);
	ddr_writel(pu, DDRP_INNOPHY_PU_ODT_DQ15_8);
	ddr_writel(pd, DDRP_INNOPHY_PD_ODT_DQ7_0);
	ddr_writel(pd, DDRP_INNOPHY_PD_ODT_DQ15_8);

}
static void ddrp_set_dq_drv(unsigned int pu, unsigned int pd)
{
	ddr_writel(pu, DDRP_INNOPHY_PU_DRV_DQ7_0);
	ddr_writel(pu, DDRP_INNOPHY_PU_DRV_DQ15_8);
	ddr_writel(pd, DDRP_INNOPHY_PD_DRV_DQ7_0);
	ddr_writel(pd, DDRP_INNOPHY_PD_DRV_DQ15_8);
}
static void ddrp_set_cmd_ck_drv(unsigned int pu, unsigned int pd)
{
	ddr_writel(pu, DDRP_INNOPHY_PU_DRV_CK);
	ddr_writel(pd, DDRP_INNOPHY_PD_DRV_CK);
	ddr_writel(pu, DDRP_INNOPHY_PU_DRV_CMD);
	ddr_writel(pd, DDRP_INNOPHY_PD_DRV_CMD);
}
/*

	该函数的作用主要用于校准 drv 和 odt 的pull up/down电阻.

	对于drv pull up/down电阻使用的是40欧姆标定.
	对于odt pull up/down电阻使用的是160欧姆标定.

	实际设置的 ODT阻值与寄存器里面有偏差:

	相关寄存器:

	setting =
	DDRP_INNOPHY_PD_DRV_DQ7_0
	DDRP_INNOPHY_PU_DRV_DQ7_0
	DDRP_INNOPHY_PD_DRV_DQ15_8
	DDRP_INNOPHY_PU_DRV_DQ15_8

	real = setting * (DDRP_INNOPHY_ZQ_CALIB_PU_ODT / 0x7)

	0x7: 对应的是160欧姆的寄存器值.


	DDRP_INNOPHY_PD_ODT_DQ7_0
	DDRP_INNOPHY_PU_ODT_DQ7_0
	DDRP_INNOPHY_PD_ODT_DQ15_8
	DDRP_INNOPHY_PU_ODT_DQ15_8

	real = setting * (DDRP_INNOPHY_ZQ_CALIB_PD_DRV  / 0x16)
	或
	real = setting * (DDRP_INNOPHY_ZQ_CALIB_PU_DRV  / 0x16)
	0x16: 是对应41.4欧姆的寄存器值.
*/
static void ddrp_zq_calibration(void)
{
	unsigned tmp;
	unsigned int pu_drv = 0;
	unsigned int pd_drv = 0;
	unsigned int pu_odt = 0;
	unsigned int pd_odt = 0;

	ddr_writel(0, DDRP_INNOPHY_ZQ_CALIB_EN);
	ddr_writel(1 << 5, DDRP_INNOPHY_ZQ_CALIB_EN);
	do{
		tmp = ddr_readl(DDRP_INNOPHY_ZQ_CALIB_DONE);
	}while(tmp != 1);

	ddr_writel(0, DDRP_INNOPHY_ZQ_CALIB_EN);

	pd_drv = ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PD_DRV_6C);
	pu_drv = ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PU_DRV_6D);
	pd_odt = ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PD_ODT_6E);
	pu_odt = ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PU_ODT_6F);

#if 1
	if(1) {

		// Choose ZQCAL value?
		ddr_writel(3 << 4, DDRP_INNOPHY_ZQ_CALIB_AL);
		ddr_writel(3 << 4, DDRP_INNOPHY_ZQ_CALIB_AH);
		tmp = ddr_readl(DDRP_INNOPHY_ZQ_CALIB_CMD);
		tmp |= 1 << 7;
		ddr_writel(tmp, DDRP_INNOPHY_ZQ_CALIB_CMD);	//Choose CMD pull up/down resistance. choose ZQCALIB value.

	} else {

		// Register value, 怎么补偿的？

		ddrp_set_dq_odt(pu_odt, pd_odt);
		ddrp_set_dq_drv(pu_drv, pd_drv);
		ddrp_set_cmd_ck_drv(pu_drv, pd_drv);
	}
#endif

	debug("DRP_INNOPHY_ZQ_CALIB_DONE : %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_DONE));
	printf("DRP_INNOPHY_ZQ_CALIB_AL: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_AL));
	printf("DRP_INNOPHY_ZQ_CALIB_AH: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_AH));
	printf("DRP_INNOPHY_ZQ_CALIB_PD_DRV_6C: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PD_DRV_6C));
	printf("DRP_INNOPHY_ZQ_CALIB_PU_DRV_6D: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PU_DRV_6D));
	printf("DRP_INNOPHY_ZQ_CALIB_PD_ODT_6E: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PD_ODT_6E));
	printf("DRP_INNOPHY_ZQ_CALIB_PU_ODT_6F: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_PU_ODT_6F));
	printf("DRP_INNOPHY_ZQ_CALIB_CMD: %x\n", ddr_readl(DDRP_INNOPHY_ZQ_CALIB_CMD));

	printf("DDRP_INNOPHY_PU_DRV_CMD:  %x\n", ddr_readl(DDRP_INNOPHY_PU_DRV_CMD));
	printf("DDRP_INNOPHY_PU_DRV_DQ7_0: %x\n", ddr_readl(DDRP_INNOPHY_PU_DRV_DQ7_0));
	printf("DDRP_INNOPHY_PU_DRV_DQ15_8: %x\n", ddr_readl(DDRP_INNOPHY_PU_DRV_DQ15_8));
	printf("DDRP_INNOPHY_PD_DRV_DQ7_0: %x\n", ddr_readl(DDRP_INNOPHY_PD_DRV_DQ7_0));
	printf("DDRP_INNOPHY_PD_DRV_DQ15_8: %x\n", ddr_readl(DDRP_INNOPHY_PU_DRV_DQ15_8));
	printf("DDRP_INNOPHY_PU_ODT_DQ7_0: %x\n", ddr_readl(DDRP_INNOPHY_PU_ODT_DQ7_0));
	printf("DDRP_INNOPHY_PU_ODT_DQ15_8: %x\n", ddr_readl(DDRP_INNOPHY_PU_ODT_DQ15_8));
	printf("DDRP_INNOPHY_PD_ODT_DQ7_0: %x\n", ddr_readl(DDRP_INNOPHY_PD_ODT_DQ7_0));
	printf("DDRP_INNOPHY_PD_ODT_DQ15_8: %x\n", ddr_readl(DDRP_INNOPHY_PD_ODT_DQ15_8));
}

void ddrp_wl_calibration(void)
{
	unsigned tmp = 0;

	printf("DDRP_INNOPHY_WL_L: 0x%x\n", ddr_readl(DDRP_INNOPHY_WL_L));
	printf("DDRP_INNOPHY_WL_H: 0x%x\n", ddr_readl(DDRP_INNOPHY_WL_H));

	unsigned int mr1 = global_reg_value->DDR_MR1_VALUE;

	mr1 = ((mr1 >> 16) & 0x7) << 13 | (mr1 & 0x1ffe); // 高三位BA2,BA1,BA0 低位地址线, 确保DLL ON， 否则后面会卡死.

	/*
		说明:
		这里做write leveling ，
		需要确保MR1寄存器里面的ZQ的设置和正常读写的一致，否则即使writeleveling 完成，
		但是实际上用的是不同的ZQ 配置，会影响后面系统的稳定性.!
	*/
	/*
		WL_MODE1:
		[7:0]: load mode [7:0];
	*/
	tmp = mr1 & 0x7f; // keep bit7 0. others mr1.
	ddr_writel(tmp, DDRP_INNOPHY_WL_MODE1);

	/*
		WL_MODE2:
		[7:6]: load mode select[1:0], 00:MR0, 01:MR1
		[5:0]: load mode [13:8]
	*/
	tmp = 0;
	tmp = 1 << 6 | ((mr1 >> 8) & 0x1f);	// 去掉高3位bank信息.取低5位.

	ddr_writel(tmp, DDRP_INNOPHY_WL_MODE2);

	tmp = 2 << 6 | 1 << 2;
	ddr_writel(tmp, DDRP_INNOPHY_TRAINING_CTRL);

	do{
		tmp = ddr_readl(DDRP_INNOPHY_WL_DONE);
	}while(tmp != 0x3);

	ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	ddr_writel(0, DDRP_INNOPHY_TRAINING_CTRL);

	printf("DDRP_INNOPHY_WL_L: 0x%x\n", ddr_readl(DDRP_INNOPHY_WL_L));
	printf("DDRP_INNOPHY_WL_H: 0x%x\n", ddr_readl(DDRP_INNOPHY_WL_H));
}

int tx_re_training(unsigned int cmd_skew)
{
	unsigned tmp, wl_l, wl_h;

	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A2);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A3);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A4);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A5);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A6);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A7);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A8);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A9);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A10);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A11);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A12);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A13);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A14);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A15);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_WEB);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CASB);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BA0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BA1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BA2);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BG1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKE);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CK0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKB0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CSB0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_ODT0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_RESETN);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_RASB);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CSB1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_ODT1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKE1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CK1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKB1);

	tmp = ddr_readl(DDRC_CGUC1);
	tmp &= ~(0xf << 4);
	ddr_writel(tmp, DDRC_CGUC1);

	ddr_writel(7 << 9 | 1 << 0, DDRC_LMR);
	udelay(200);

	tmp = 0 << 7 | 6;
	ddr_writel(tmp, DDRP_INNOPHY_WL_MODE1);
	tmp = 0x40;
	ddr_writel(tmp, DDRP_INNOPHY_WL_MODE2);

	tmp = ddr_readl(DDRC_CGUC1);
	tmp |= 0xf << 4;
	ddr_writel(tmp, DDRC_CGUC1);

	ddr_writel(2 << 6 | 1 << 2, DDRP_INNOPHY_TRAINING_CTRL);

	do{
		tmp = ddr_readl(DDRP_INNOPHY_WL_DONE);
	}while(tmp != 3);

	ddr_readl( DDRP_INNOPHY_TRAINING_CTRL);
	ddr_writel(0, DDRP_INNOPHY_TRAINING_CTRL);

	wl_l = ddr_readl(DDRP_INNOPHY_WL_L);
	wl_h = ddr_readl(DDRP_INNOPHY_WL_H);
	printf("write leveling low : 0x%x\n", ddr_readl(DDRP_INNOPHY_WL_L));
	printf("write leveling high: 0x%x\n", ddr_readl(DDRP_INNOPHY_WL_H));

	if((wl_h == 0x3f) && (wl_l == 0x3f))
		return -1;
	else
		return 0;
}

void tx_soft_training()
{
	unsigned int addr = 0xa1000000, val, reg;
	unsigned int dq_skew[64] = {0};
	unsigned int i, j, n, m = 0, finish = 0, de_skew;
	unsigned int cmd_skew = 32;
	unsigned int cnt = 256;
	unsigned int cmd_test_all = 0;
	volatile unsigned int val1 = 0;
	int ret;

	reg = ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	reg |= (DDRP_TRAINING_CTRL_WL_BP);
	reg &= ~(DDRP_TRAINING_CTRL_WL_START);
	ddr_writel(reg, DDRP_INNOPHY_TRAINING_CTRL);

	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A2);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A3);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A4);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A5);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A6);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A7);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A8);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A9);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A10);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A11);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A12);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A13);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A14);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_A15);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_WEB);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CASB);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BA0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BA1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BA2);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_BG1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKE);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CK0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKB0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CSB0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_ODT0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_RESETN);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_RASB);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CSB1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_ODT1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKE1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CK1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_CKB1);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_TX_DM0);
	ddr_writel(cmd_skew, DDRP_INNOPHY_PBDS_TX_DM1);

	do{
		for(i = 0; i < 64; i++){
			/* printf("i = %d-----\n", i); */
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ0);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ1);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ2);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ3);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ4);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ5);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ6);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ7);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ8);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ9);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ10);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ11);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ12);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ13);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ14);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DQ15);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DM0);
			ddr_writel(i, DDRP_INNOPHY_PBDS_TX_DM1);

			for (j = 0; j < cnt; j++) {
				volatile unsigned int val1;
				val = 0x12345678;
				*(volatile unsigned int *)(addr + j * 4) = val;
				val1 = *(volatile unsigned int *)(addr + j * 4);

				if (val1 != val) {
					/*printf("%s  val = 0x%x val1 = 0x%x addr = 0x%x\n", __func__, val, val1, addr + j * 4);*/
					break;
				}
			}

			if (j == cnt) {
				dq_skew[m] = i;
				m++;
			}

		}

		if ((m == 1) && (dq_skew[0] == 0)) {
			cmd_skew++;
			m = 0;
			tx_re_training(cmd_skew);
			if ((cmd_skew > 64) || (cmd_skew < 0))
				finish = 1;
			else
				finish = 0;
		} else if ((m == 1) && (dq_skew[0] == 63)) {
			cmd_skew--;
			m = 0;
			tx_re_training(cmd_skew);
			if ((cmd_skew > 64) || (cmd_skew < 0))
				finish = 1;
			else
				finish = 0;
		} else if (m == 0) {
			printf("%s no_data_found cmd_skew = %d\n", __func__, cmd_skew);
			if (cmd_test_all == 0) {
				cmd_skew = 0;
				cmd_test_all = 1;
			}
			if (cmd_test_all == 1) {
				cmd_skew++;
			}

			m = 0;
			do  {
				ret = tx_re_training(cmd_skew);
				if (ret)
					cmd_skew++;
			} while (ret);
			finish = 0;
		} else {
			de_skew = dq_skew[m / 2];
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ0);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ1);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ2);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ3);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ4);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ5);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ6);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ7);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ8);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ9);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ10);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ11);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ12);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ13);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ14);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DQ15);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DM0);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_TX_DM1);
         finish = 1;
      }

   } while(!finish);

}

void rx_soft_training()
{
	unsigned int addr = 0xa1000000, val;
	unsigned int dq_skew[64] = {0};
	unsigned int i, j, n, m = 0, finish = 0, de_skew;
	unsigned int dqs_skew = 32;
	unsigned int cnt = 256;
	unsigned int dqs_test_all = 0;
	volatile unsigned int val1 = 0;
	int ret;

	do{
		ddr_writel(dqs_skew, DDRP_INNOPHY_PBDS_RX_DQS0);
		ddr_writel(dqs_skew, DDRP_INNOPHY_PBDS_RX_DQS1);
		ddr_writel(dqs_skew, DDRP_INNOPHY_PBDS_RX_DQSB0);
		ddr_writel(dqs_skew, DDRP_INNOPHY_PBDS_RX_DQSB1);
		for(i = 0; i < 64; i++){
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ0);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ1);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ2);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ3);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ4);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ5);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ6);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ7);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ8);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ9);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ10);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ11);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ12);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ13);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ14);
			ddr_writel(i, DDRP_INNOPHY_PBDS_RX_DQ15);

			for (j = 0; j < cnt; j++) {
				volatile unsigned int val1;
				val = 0x12345678;
				*(volatile unsigned int *)(addr + j * 4) = val;
				val1 = *(volatile unsigned int *)(addr + j * 4);

				if (val1 != val) {
					/* printf("%s  val = 0x%x val1 = 0x%x addr 0x%x\n", __func__, val, val1, addr + j * 4); */
					break;
				}
			}

			if (j == cnt) {
				dq_skew[m] = i;
				m++;
			}

		}

		if ((m == 1) && (dq_skew[0] == 0)) {
			dqs_skew++;
			m = 0;
			if (dqs_skew == 64)
				finish = 1;
			else
				finish = 0;
		} else if ((m == 1) && (dq_skew[0] == 63)) {
			dqs_skew--;
			m = 0;
			if (dqs_skew < 0)
				finish = 1;
			else
				finish = 0;
		} else if (m == 0) {
			printf("%s no_data_found dqs_skew = %d\n", __func__, dqs_skew);
			if (dqs_test_all == 0) {
				dqs_skew = 0;
				dqs_test_all = 1;
			}
			if (dqs_test_all == 1) {
				dqs_skew++;
			}

			m = 0;
			finish = 0;
		} else {
			de_skew = dq_skew[m / 2];
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ0);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ1);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ2);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ3);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ4);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ5);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ6);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ7);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ8);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ9);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ10);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ11);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ12);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ13);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ14);
			ddr_writel(de_skew, DDRP_INNOPHY_PBDS_RX_DQ15);
			finish = 1;
		}

	} while(!finish);

}

void ddrp_pll_init(void)
{
	ddr_writel(0x0, DDRP_INNOPHY_PLL_FBDIV);
	ddr_writel(0x6, DDRP_INNOPHY_PLL_FBDIV_H);
	ddr_writel(0x41, DDRP_INNOPHY_PLL_PDIV);
	ddr_writel(0x0, DDRP_INNOPHY_PLL_CTRL);
	udelay(500);
}

void ddrp_cfg(struct ddr_reg_value *global_reg_value)
{
	unsigned int val;
#ifdef DEBUG_READ_WRITE
	ddr_writel(0, DDRP_INNOPHY_DQ_WIDTH_H);
	val = ddr_readl(DDRP_INNOPHY_DQ_WIDTH);
	val &= ~(0x3);
	val |= DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L;
	ddr_writel(val, DDRP_INNOPHY_DQ_WIDTH);

	val = ddr_readl(DDRP_INNOPHY_MEM_CFG);
	val &= ~(0x3 | 1 << 4);
	val |= 1 << 4 | 3;
	ddr_writel(val, DDRP_INNOPHY_MEM_CFG);

	debug("ddr_readl(DDRP_INNOPHY_CL)  %x\n", ddr_readl(DDRP_INNOPHY_CL));
	debug("ddr_readl(DDRP_INNOPHY_CWL)  %x\n", ddr_readl(DDRP_INNOPHY_CWL));
#else
	ddr_writel(0, DDRP_INNOPHY_DQ_WIDTH_H);
	ddr_writel(DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L, DDRP_INNOPHY_DQ_WIDTH);
	ddr_writel(global_reg_value->DDRP_MEMCFG_VALUE, DDRP_INNOPHY_MEM_CFG);
#endif

	val = ddr_readl(DDRP_INNOPHY_CL);
	val &= ~(0xf);
	val |= global_reg_value->DDRP_CL_VALUE;
	ddr_writel(val, DDRP_INNOPHY_CL);

	val = ddr_readl(DDRP_INNOPHY_CWL);
	val &= ~(0xf);
	val |= global_reg_value->DDRP_CWL_VALUE;
	ddr_writel(val, DDRP_INNOPHY_CWL);

	val = ddr_readl(DDRP_INNOPHY_AL);
	val &= ~(0xf);
	ddr_writel(val, DDRP_INNOPHY_AL);

	debug("ddr_readl(DDRP_INNOPHY_CL)   %x\n", ddr_readl(DDRP_INNOPHY_CL));
	debug("ddr_readl(DDRP_INNOPHY_CWL)  %x\n", ddr_readl(DDRP_INNOPHY_CWL));
	debug("ddr_readl(DDRP_INNOPHY_AL)   %x\n", ddr_readl(DDRP_INNOPHY_AL));
}

/*
 * Name     : ddrp_calibration()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
static void ddrp_rx_dqs_auto_calibration(void)
{
	unsigned int reg_val = ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	unsigned int timeout = 0xffffff;

	ddr_writel(0x0, DDRP_INNOPHY_CALIB_MODE);
	reg_val &= ~(DDRP_TRAINING_CTRL_DSCSE_BP);
	reg_val |= DDRP_TRAINING_CTRL_DSACE_START;
	ddr_writel(reg_val, DDRP_INNOPHY_TRAINING_CTRL);

	while(!((ddr_readl(DDRP_INNOPHY_CALIB_DONE) & 0x13) == 3) && --timeout) {

		udelay(1);
		printf("-----ddr_readl(DDRP_INNOPHY_CALIB_DONE): %x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
	}

	if(!timeout) {
		debug("ddrp_auto_calibration failed!\n");
		while(1);
	}

	debug("ddrp_auto_calibration success!\n");

	printf("DDRP_INNOPHY_CALIB_DONE: %x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
	printf("DDRP_INNOPHY_CALIB_ERR:	%X\n", ddr_readl(DDRP_INNOPHY_CALIB_ERR));
	printf("DDRP_INNOPHY_CALIB_L_C: %x\n", ddr_readl(DDRP_INNOPHY_CALIB_L_C));
	printf("DDRP_INNOPHY_CALIB_L_DO: %x\n", ddr_readl(DDRP_INNOPHY_CALIB_L_DO));
	printf("DDRP_INNOPHY_CALIB_R_C: %x\n", ddr_readl(DDRP_INNOPHY_CALIB_R_C));
	printf("DDRP_INNOPHY_CALIB_R_DO: %x\n", ddr_readl(DDRP_INNOPHY_CALIB_R_DO));

	if(ddr_readl(DDRP_INNOPHY_CALIB_ERR) & (1 << 6)) {
		printf("ddr pass but with error!\n");
		while(1);
	}



	ddr_writel(0, DDRP_INNOPHY_TRAINING_CTRL);
}

void ddrp_auto_calibration(void)
{

	//ddrp_zq_calibration();
	ddrp_wl_calibration();
	ddrp_rx_dqs_auto_calibration();


}

#ifdef CONFIG_DDRP_SOFTWARE_TRAINING

//#define DDR_CHOOSE_PARAMS	0
#ifdef DDR_CHOOSE_PARAMS
static int atoi(char *pstr)
{
	int value = 0;
	int sign = 1;
	int radix;

	if(*pstr == '-'){
		sign = -1;
		pstr++;
	}
	if(*pstr == '0' && (*(pstr+1) == 'x' || *(pstr+1) == 'X')){
		radix = 16;
		pstr += 2;
	}
	else
		radix = 10;
	while(*pstr){
		if(radix == 16){
			if(*pstr >= '0' && *pstr <= '9')
				value = value * radix + *pstr - '0';
			else if(*pstr >= 'A' && *pstr <= 'F')
				value = value * radix + *pstr - 'A' + 10;
			else if(*pstr >= 'a' && *pstr <= 'f')
				value = value * radix + *pstr - 'a' + 10;
		}
		else
			value = value * radix + *pstr - '0';
		pstr++;
	}
	return sign*value;
}


static int choose_params(int m)
{
	char buf[16];
	char ch;
	char *p = buf;
	int select_m;

	debug("Please select from [0 to %d]\n", m);

	debug(">>  ");

	while((ch = getc()) != '\r') {
		putc(ch);
		*p++ = ch;

		if((p - buf) > 16)
			break;
	}
	*p = '\0';

	debug("\n");

	select_m = atoi(buf);
	debug("slected: %d\n", select_m);

	return select_m;
}
#endif

struct ddrp_calib {
	union{
		uint8_t u8;
		struct{
			unsigned cyclesel:3;
			unsigned reserved:5;
		}b;
	}bypass_c;
	union{
		uint8_t u8;
		struct{
			unsigned ophsel:3;
			unsigned dllsel:5;
		}b;
	}bypass;
	union{
		uint8_t u8;
		struct{
			unsigned reserved:5;
			unsigned rx_dll:3;
		}b;
	}rx_dll;
};

/* struct ddrp_calib calib_val[8 * 4 * 8 * 5]; */
/*
 * Name     : ddrp_calibration_manual()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
struct ddrp_calib calib_val[8*8*8];

static void ddrp_software_calibration(void)
{

	int x, y, z, x1, y1;
	int c, o, d =0, r = 0;
	unsigned int addr = 0xa1000000, val;
	unsigned int i, n, m = 0;
	unsigned int reg;
	unsigned int tmp;
	volatile unsigned int val1 = 0;

	ddr_writel(0x20, DDRP_INNOPHY_TRAINING_CTRL);
	ddr_writel(0x0, DDRP_INNOPHY_CALIB_MODE);
	reg = ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	reg |= (DDRP_TRAINING_CTRL_DSCSE_BP);
	reg &= ~(DDRP_TRAINING_CTRL_DSACE_START);
	ddr_writel(reg, DDRP_INNOPHY_TRAINING_CTRL);

	for(c = 0; c < 8; c ++) {
		for(o = 0; o < 8; o++) {
			for (d = 0; d < 32; d++) {
				x = d << 3 | o;
				x1 = c;
				y = d << 3 | o;
				y1 = c;
				ddr_writel(x, DDRP_INNOPHY_CALIB_BYPASS_AL);
				ddr_writel(x1, DDRP_INNOPHY_CALIB_BYPASS_AL_C);
				ddr_writel(y, DDRP_INNOPHY_CALIB_BYPASS_AH);
				ddr_writel(y1, DDRP_INNOPHY_CALIB_BYPASS_AH_C);

				for(i = 0; i < 0xff; i++) {
					val = 0;
					for(n = 0; n < 4; n++ ) {
						val |= (i+n)<<(n * 8);
					}

					*(volatile unsigned int *)(addr + i * 4) = val;
					val1 = *(volatile unsigned int *)(addr + i * 4);

					if(val1 != val) {
						break;
					}
#if 0
					printf("val1 : 0x%x val : 0x%x, addr = 0x%x, bypass_l_c : %x bypass_l :%x bypass_h_c : %x bypass: %x\n", val1, val, addr + i * 4, \
						ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL_C),\
						ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL), \
						ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH_C),\
						ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH));
#endif
				}
				if(i == 0xff) {
					calib_val[m].bypass_c.b.cyclesel = c;
					calib_val[m].bypass.b.ophsel = o;
					calib_val[m].bypass.b.dllsel = d;
					calib_val[m].rx_dll.b.rx_dll = r;
					m++;
				}
			}
		}
	}

	if(!m) {
		printf("calib bypass fail\n");
		return ;
	}


	m = m  / 2;
	c = calib_val[m].bypass_c.b.cyclesel;
	o = calib_val[m].bypass.b.ophsel;
	d = calib_val[m].bypass.b.dllsel;
	r = calib_val[m].rx_dll.b.rx_dll;

	printf("m = %d   c = %d   o = %d   d = %d  r = %d\n", m, c, o, d, r);

	x = d << 3 | o;
	x1 = c;
	y = d << 3 | o;
	y1 = c;
	ddr_writel(x, DDRP_INNOPHY_CALIB_BYPASS_AL);
	ddr_writel(y, DDRP_INNOPHY_CALIB_BYPASS_AH);
	ddr_writel(x1, DDRP_INNOPHY_CALIB_BYPASS_AL_C);
	ddr_writel(y1, DDRP_INNOPHY_CALIB_BYPASS_AH_C);

}


#endif


