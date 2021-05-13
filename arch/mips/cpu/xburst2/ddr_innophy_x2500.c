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

/*#define DEBUG*/
/* #define DEBUG_READ_WRITE */
/*#define CONFIG_DDRP_SOFTWARE_TRAINING	1*/
#include <config.h>
#include <common.h>
#include <ddr/ddr_common.h>
#ifndef CONFIG_BURNER
//#include <generated/ddr_reg_values.h>
extern struct ddr_reg_value supported_ddr_reg_values[];
#endif

#include <asm/cacheops.h>

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

static void dump_inno_driver_strength_register(void)
{
	printf("inno reg:0x42 = 0x%x\n", readl(0xb3011108));
	printf("inno reg:0x41 = 0x%x\n", readl(0xb3011104));
	printf("inno cmd io driver strenth pull_down                = 0x%x\n", readl(0xb30112c0));
	printf("inno cmd io driver strenth pull_up                  = 0x%x\n", readl(0xb30112c4));

	printf("inno clk io driver strenth pull_down                = 0x%x\n", readl(0xb30112c8));
	printf("inno clk io driver strenth pull_up                  = 0x%x\n", readl(0xb30112cc));

	printf("Channel A data io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(0xb3011300));
	printf("Channel A data io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(0xb3011304));
	printf("Channel A data io ODT DQ[15:8] pull_down               = 0x%x\n", readl(0xb3011340));
	printf("Channel A data io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(0xb3011344));
	printf("Channel B data io ODT DQ[7:0]  pull_down               = 0x%x\n", readl(0xb3011380));
	printf("Channel B data io ODT DQ[7:0]  pull_up                 = 0x%x\n", readl(0xb3011384));
	printf("Channel B data io ODT DQ[15:8] pull_down               = 0x%x\n", readl(0xb30113c0));
	printf("Channel B data io ODT DQ[15:8] pull_up                 = 0x%x\n", readl(0xb30113c4));

	printf("Channel A data io driver strenth DQ[7:0]  pull_down    = 0x%x\n", readl(0xb3011308));
	printf("Channel A data io driver strenth DQ[7:0]  pull_up      = 0x%x\n", readl(0xb301130c));
	printf("Channel A data io driver strenth DQ[15:8] pull_down    = 0x%x\n", readl(0xb3011348));
	printf("Channel A data io driver strenth DQ[15:8] pull_up      = 0x%x\n", readl(0xb301134c));
	printf("Channel B data io driver strenth DQ[7:0]  pull_down    = 0x%x\n", readl(0xb3011388));
	printf("Channel B data io driver strenth DQ[7:0]  pull_up      = 0x%x\n", readl(0xb301138c));
	printf("Channel B data io driver strenth DQ[15:8] pull_down    = 0x%x\n", readl(0xb30113c8));
	printf("Channel B data io driver strenth DQ[15:8] pull_up      = 0x%x\n", readl(0xb30113cc));

}




/*
 * Name     : ddrp_calibration()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
#if 0
static void ddrp_calibration(int al8_1x,int ah8_1x,int al8_2x,int ah8_2x)
{
	ddr_writel(ddr_readl(DDRP_INNOPHY_TRAINING_CTRL) | DDRP_TRAINING_CTRL_DSCSE_BP, DDRP_INNOPHY_TRAINING_CTRL);

	int x = ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL);
	int y = ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH);
	x = (x & ~(0xf << 3)) | (al8_1x << DDRP_CALIB_BP_CYCLESELBH_BIT) | (al8_2x << DDRP_CALIB_BP_OPHCSELBH_BIT);
	y = (y & ~(0xf << 3)) | (ah8_1x << DDRP_CALIB_BP_CYCLESELBH_BIT) | (ah8_2x << DDRP_CALIB_BP_OPHCSELBH_BIT);
	ddr_writel(x, DDRP_INNOPHY_CALIB_BYPASS_AL);
	ddr_writel(y, DDRP_INNOPHY_CALIB_BYPASS_AH);
}
#endif


struct ddrp_calib {
	union{
		uint8_t u8;
		struct{
			uint8_t dllsel:3;
			uint8_t ophsel:1;
			uint8_t cyclesel:3;
		}b;
	}bypass;
	union{
		uint8_t u8;
		struct{
			uint8_t reserved:5;
			uint8_t rx_dll:3;
		}b;
	}rx_dll;
};

/*
 * Name     : ddrp_calibration_manual()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */



void ddrp_auto_calibration(void)
{
	unsigned int val;
	unsigned int timeout = 1000000;


	ddr_writel(ddr_readl(DDRP_INNOPHY_TRAINING_CTRL)|0x1, DDRP_INNOPHY_TRAINING_CTRL);
	do
	{
		val = ddr_readl(DDRP_INNOPHY_CALIB_DONE);
	} while (((val & 0xf) != 0xf) && timeout--);

	if(!timeout) {
		printf("timeout:INNOPHY_CALIB_DONE %x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
		hang();
	}
	ddr_writel(ddr_readl(DDRP_INNOPHY_TRAINING_CTRL)&(~0x1), DDRP_INNOPHY_TRAINING_CTRL);

	{
		struct ddrp_calib al, ah, bl, bh;
		al.bypass.u8 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL);
		printf("auto :CALIB_AL: dllsel %x, ophsel %x, cyclesel %x\n", al.bypass.b.dllsel, al.bypass.b.ophsel, al.bypass.b.cyclesel);
		ah.bypass.u8 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH);
		printf("auto:CAHIB_AH: dllsel %x, ophsel %x, cyclesel %x\n", ah.bypass.b.dllsel, ah.bypass.b.ophsel, ah.bypass.b.cyclesel);
		bl.bypass.u8 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BL);
		printf("auto :CALIB_BL: dllsel %x, ophsel %x, cyclesel %x\n", bl.bypass.b.dllsel, bl.bypass.b.ophsel, bl.bypass.b.cyclesel);
		bh.bypass.u8 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_BH);
		printf("auto:CAHIB_BH: dllsel %x, ophsel %x, cyclesel %x\n", bh.bypass.b.dllsel, bh.bypass.b.ophsel, bh.bypass.b.cyclesel);
	}
//	printf("hard A_calib 0x74 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x74 << 2)));
//	printf("hard A_calib 0x75 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x75 << 2)));
//	printf("hard B_calib 0xa4 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0xa4 << 2)));
//	printf("hard B_calib 0xa5 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0xa5 << 2)));
//	printf("hard A_bypass 0x56 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x56 << 2)));
//	printf("hard A_bypass 0x66 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x66 << 2)));
//	printf("hard B_bypass 0x86 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x86 << 2)));
//	printf("hard B_bypass 0x96 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x96 << 2)));

//	printf("hard A_rdll 0x58 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x58 << 2)));
//	printf("hard A_rdll 0x68 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x68 << 2)));
//	printf("hard B_rdll 0x88 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x88 << 2)));
//	printf("hard B_rdll 0x98 : 0x%x\n", *(volatile unsigned int *)(0xb3011000 + (0x98 << 2)));


#if 1
	{
		unsigned int cycsel, tmp;
		unsigned int read_data0, read_data1;
		unsigned int read_data2, read_data3;
		unsigned int c0, c1, c2, c3;
		unsigned int max;

		read_data0 = *(volatile unsigned int *)(0xb3011000 + (0x74 << 2));
		read_data1 = *(volatile unsigned int *)(0xb3011000 + (0x75 << 2));
		read_data2 = *(volatile unsigned int *)(0xb3011000 + (0xa4 << 2));
		read_data3 = *(volatile unsigned int *)(0xb3011000 + (0xa5 << 2));
		c0 = (read_data0 >> 4) & 0x7;
		c1 = (read_data1 >> 4) & 0x7;
		c2 = (read_data0 >> 4) & 0x7;
		c3 = (read_data1 >> 4) & 0x7;

		max = max(max(c0, c1), max(c2, c3));

		cycsel = max + 3;

		tmp = *(volatile unsigned int *)(0xb3011000 + (0xa << 2));
		tmp &= ~(7 << 1);
		tmp |= cycsel << 1;
		*(volatile unsigned int *)(0xb3011000 + (0xa << 2)) = tmp;


		tmp = *(volatile unsigned int *)(0xb3011000 + 0x4);
		tmp |= 1 << 6;
		*(volatile unsigned int *)(0xb3011000 + (0x1 << 2)) = tmp;
	}

	printf("ddr calib test finish\n");
#endif


}

void ddr_phyreg_set_range(u32 offset, u32 startbit, u32 bitscnt, u32 value)
{
	u32 reg = 0;
	u32 mask = 0;
	mask = ((0xffffffff>>startbit)<<(startbit))&((0xffffffff<<(32 - startbit - bitscnt))>>(32 - startbit - bitscnt));
	reg = readl(DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
	reg = (reg&(~mask))|((value<<startbit)&mask);
	//printf("value = %x, reg = %x, mask = %x", value, reg, mask);
	writel(reg, DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
}
static void ddr_phy_cfg_driver_odt(void)
{
	/* ddr phy driver strength and  ODT config */

	/* cmd */
	ddr_phyreg_set_range(0xb0, 0, 5, 0xf);
	ddr_phyreg_set_range(0xb1, 0, 5, 0xf);
	/* ck */
	ddr_phyreg_set_range(0xb2, 0, 5, 0x11);//pull down
	ddr_phyreg_set_range(0xb3, 0, 5, 0x11);//pull up

	/* DQ ODT config */
	u32 dq_odt = 0x3;

	ddr_phyreg_set_range(0xc0, 0, 5, dq_odt);	//A low  pull down
	ddr_phyreg_set_range(0xc1, 0, 5, dq_odt);	//A low  pull up

	ddr_phyreg_set_range(0xd0, 0, 5, dq_odt);	//A high pull down
	ddr_phyreg_set_range(0xd1, 0, 5, dq_odt);	//A high pull up

	ddr_phyreg_set_range(0xe0, 0, 5, dq_odt);	//B low  pull down
	ddr_phyreg_set_range(0xe1, 0, 5, dq_odt);	//B low  pull up

	ddr_phyreg_set_range(0xf0, 0, 5, dq_odt);	//B high pull down
	ddr_phyreg_set_range(0xf1, 0, 5, dq_odt);	//B high pull up

	/* driver strength config */
	ddr_phyreg_set_range(0xc2, 0, 5, 0x11);	//A low  pull down
	ddr_phyreg_set_range(0xc3, 0, 5, 0x11);	//A low  pull up
	ddr_phyreg_set_range(0xd2, 0, 5, 0x11);	//A high pull down
	ddr_phyreg_set_range(0xd3, 0, 5, 0x11);	//A hign pull up

	ddr_phyreg_set_range(0xe2, 0, 5, 0x11);	//B low  pull down
	ddr_phyreg_set_range(0xe3, 0, 5, 0x11);	//B low  pull up
	ddr_phyreg_set_range(0xf2, 0, 5, 0x11);	//B high pull down
	ddr_phyreg_set_range(0xf3, 0, 5, 0x11);	//B hign pull up
}


void ddrp_cfg(struct ddr_reg_value *global_reg_value)
{
	unsigned int val;

	/* reg1f; bit[7:4]reserved; bit[3:0]DQ width
	 * DQ width bit[3:0]:0x1:8bit,0x3:16bit,0x7:24bit,0xf:32bit */
	val = ddr_readl(DDRP_INNOPHY_DQ_WIDTH);
	val &= ~(0xf);
#if CONFIG_DDR_DW32
	val |= 0xf; /* 32bit */
#else
	val |= DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L; /* 0x3:16bit */
#endif
	ddr_writel(val, DDRP_INNOPHY_DQ_WIDTH);

	/* reg1; bit[7:5]reserved; bit[4]burst type; bit[3:2]reserved; bit[1:0]DDR mode */
	val = ddr_readl(DDRP_INNOPHY_MEM_CFG);
	val &= ~(0xff);
	val |= 0x10; /* burst 8 */
	ddr_writel(val, DDRP_INNOPHY_MEM_CFG);

	/* CWL */
	val = ddr_readl(DDRP_INNOPHY_CWL);
	val &= ~(0xf);
	val |= global_reg_value->DDRP_CWL_VALUE;
	ddr_writel(val, DDRP_INNOPHY_CWL);

	/* CL */
	val = ddr_readl(DDRP_INNOPHY_CL);
	val &= ~(0xf);
	val |= global_reg_value->DDRP_CL_VALUE;
	ddr_writel(val, DDRP_INNOPHY_CL);

	/* AL */
	val = ddr_readl(DDRP_INNOPHY_AL);
	val &= ~(0xf);
	val |= 0x0;
	ddr_writel(val, DDRP_INNOPHY_AL);


	ddr_phy_cfg_drive();
}


void ddrp_pll_init(void)
{
	unsigned int val;
	unsigned int timeout = 1000000;

	/* fbdiv={reg21[0],reg20[7:0]};
	 * pre_div = reg22[4:0];
	 * post_div = 2^reg22[7:5];
	 * fpll_2xclk = fpll_refclk * fbdiv / ( post_div * pre_div);
	 *			  = fpll_refclk * 20 / ( 1 * 10 ) = fpll_refclk * 2;
	 * fpll_1xclk = fpll_2xclk / 2 = fpll_refclk;
	 * Use: IN:fpll_refclk, OUT:fpll_1xclk */

	/* reg20; bit[7:0]fbdiv M[7:0] */
	val = ddr_readl(DDRP_INNOPHY_PLL_FBDIV);
	val &= ~(0xff);
	val |= 0x10;
	ddr_writel(val, DDRP_INNOPHY_PLL_FBDIV);

	/* reg21; bit[7:2]reserved; bit[1]PLL reset; bit[0]fbdiv M8 */
	val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
	val &= ~(0xff);
	val |= 0x1a;
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);

	/* reg22; bit[7:5]post_div; bit[4:0]pre_div */
	val = ddr_readl(DDRP_INNOPHY_PLL_PDIV);
	val &= ~(0xff);
	val |= 0x4;
	ddr_writel(val, DDRP_INNOPHY_PLL_PDIV);

	/* reg21; bit[7:2]reserved; bit[1]PLL reset; bit[0]fbdiv M8 */
	val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
	val &= ~(0xff);
	val |= 0x18;
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);


#if 0
	/* bit[3]reset digital core; bit[2]reset analog logic; bit[0]Reset Initial status
	 * other reserved */
	val = ddr_readl(DDRP_INNOPHY_PHY_RST);
	val &= ~(0xff);
	val |= 0x0d;
	ddr_writel(val, DDRP_INNOPHY_PHY_RST);
#endif


	while(!(ddr_readl(DDRP_INNOPHY_PLL_LOCK) & 1 << 3) && timeout--);
	if(!timeout) {
		printf("DDRP_INNOPHY_PLL_LOCK time out!!!\n");
		hang();
	}
//	mdelay(20);
}
void ddr_phy_cfg_drive(void)
{
	FUNC_ENTER();

	u32 idx;
	u32  val;
#if 0
	/* bit[3]reset digital core; bit[2]reset analog logic; bit[0]Reset Initial status
	 * other reserved */
	val = ddr_readl(DDRP_INNOPHY_PHY_RST);
	val &= ~(0xff);
	mdelay(2);
	val |= 0x0d;
	ddr_writel(val, DDRP_INNOPHY_PHY_RST);
#endif
	ddr_phy_cfg_driver_odt();


	writel(0xc, 0xb3011000 + (0x15)*4);//default 0x4 cmd
	writel(0x1, 0xb3011000 + (0x16)*4);//default 0x0 ck


	writel(0xc, 0xb3011000 + (0x54)*4);//default 0x4  byte0 dq dll
	writel(0xc, 0xb3011000 + (0x64)*4);//default 0x4  byte1 dq dll
	writel(0xc, 0xb3011000 + (0x84)*4);//default 0x4  byte2 dq dll
	writel(0xc, 0xb3011000 + (0x94)*4);//default 0x4  byte3 dq dll

	writel(0x1, 0xb3011000 + (0x55)*4);//default 0x0  byte0 dqs dll
	writel(0x1, 0xb3011000 + (0x65)*4);//default 0x0  byte1 dqs dll
	writel(0x1, 0xb3011000 + (0x85)*4);//default 0x0  byte2 dqs dll
	writel(0x1, 0xb3011000 + (0x95)*4);//default 0x0  byte3 dqs dll



	//ddr_phy_cfg_vref()
	unsigned int i = 0;
	u32 value = 8;
	/* write leveling dq delay time config */
	//cmd
	for (i = 0; i <= 0x1e;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x100+i)*4);///cmd
	}
	ddr_writel(value, DDR_PHY_OFFSET + (0x100+0x16)*4);///ck
	ddr_writel(value, DDR_PHY_OFFSET + (0x100+0x17)*4);///ckb
	//tx DQ
	for (i = 0; i <= 0x8;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
		ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
	}

	for (i = 0xb; i <= 0x13;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
		ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
	}

	//addr
	for (i = 0; i <= 0xf;i++) {
		ddr_writel(value, DDR_PHY_OFFSET + (0x100+i)*4);//addr
	}
	/* CK Pre bit skew */
	for (idx = 0x16; idx <= 0x17; idx++) {
		writel(value, 0xb3011000+((0x100+idx)*4));//CK Pre bit skew
	}


	//enable bypass write leveling
	//open manual per bit de-skew
	//printf("PHY REG-02 :  0x%x \n",readl(0xb3011008));
	writel((readl(0xb3011008))|(0x8), 0xb3011008);
	//printf("PHY REG-02 :  0x%x \n",readl(0xb3011008));





	FUNC_EXIT();
}





