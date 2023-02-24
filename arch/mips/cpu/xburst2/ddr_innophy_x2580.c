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

static void ddr_phy_cfg_drive(struct ddr_reg_value *global_reg_value);

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


void ddrp_auto_calibration(void)
{
	unsigned int val;
	unsigned int timeout = 1000000;
	ddr_writel(0x0, DDRP_INNOPHY_TRAINING_CTRL);
    ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	ddr_writel(0x1, DDRP_INNOPHY_TRAINING_CTRL);
	do
	{
		val = ddr_readl(DDRP_INNOPHY_CALIB_DONE);
	} while (((val & 0xf) != 0x3) && timeout--);

	if(!timeout) {
		debug("timeout:INNOPHY_CALIB_DONE %x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
		hang();
	}

	ddr_writel(0x0, DDRP_INNOPHY_TRAINING_CTRL);
	{
		int reg1, reg2;
		debug("ddrp rx hard calibration:\n");
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL_RESULT1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL_RESULT2);
		debug("CALIB_AL: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
		reg1 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH_RESULT1);
		reg2 = ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH_RESULT2);
		debug("CALIB_AH: cyc %d, oph %d, dll %d\n", reg1, reg2&0x3, reg2>>3&0x1f);
	}
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


void ddrp_cfg(struct ddr_reg_value *global_reg_value, unsigned int rate)
{
	unsigned int val;

	/***
	 * pllprediv[4:0]:0x52[4:0]() must set 1
	 * pllfbdiv[8:0]:0x51[0],0x50[7:0] must set 1
	 * pllpostdiven:0x51[7] must set 1,使能postdiv
	 * pllpostdiv:0x53[7:5] 不同频率配置不同参数，具体如下(0x53寄存器)：
	 * 默认配置0x20: 533MHz ~ 1066 MHz
	 *         0x40: 256MHz ~ 533MHz
	 *         0x60: 132MHz ~ 256MHz
	 *         0xc0: 66MHz  ~ 132MHz
	 *         0xe0: 33MHz  ~ 66MHz
	 ***/
	/* pllfbdiv */
	val = ddr_readl(DDRP_INNOPHY_PLL_FBDIVL);
	val &= ~(0xff);
	val |= 0x1;
	ddr_writel(val, DDRP_INNOPHY_PLL_FBDIVL);

	/* pllpostdiven; pllfbdiv */
	val = ddr_readl(DDRP_INNOPHY_PLL_FBDIVH);
	val &= ~(0xff);
	val |= 0x80;
	ddr_writel(val, DDRP_INNOPHY_PLL_FBDIVH);

	/* pllpostdiv */
	val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
	val &= ~(0xff);
	if(rate > 533000000)
		val |= 0x28;
	else
		val |= 0x48;
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);

	/* pllprediv */
	val = ddr_readl(DDRP_INNOPHY_PLL_PDIV);
	val &= ~(0x1f);
	val |= 0x1;
	ddr_writel(val, DDRP_INNOPHY_PLL_PDIV);

	if(rate > 533000000)
		val = 0x20;
	else
		val = 0x40;
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);
	while(!(ddr_readl(DDRP_INNOPHY_PLL_LOCK) & 1 << 2));
	
	/* reg1; bit[7:5]reserved; bit[4]burst type; bit[3:2]reserved; bit[1:0]DDR mode */
	val = ddr_readl(DDRP_INNOPHY_MEM_CFG);
	val &= ~(0xff);
	val |= 0x0000000a; /* burst 8 */
	ddr_writel(val, DDRP_INNOPHY_MEM_CFG);

	val = DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L; /* 0x3:16bit */
	ddr_writel(val, DDRP_INNOPHY_DQ_WIDTH);
	
	val = ddr_readl(DDRP_INNOPHY_INNO_PHY_RST);
	val &= ~(0xff);
	val |= 0x0d;
	ddr_writel(val, DDRP_INNOPHY_INNO_PHY_RST);

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


	val = 0x0;
	ddr_writel(val, DDRP_INNOPHY_AL);
}

static void ddr_phy_cfg_drive(struct ddr_reg_value *global_reg_value)
{
	FUNC_ENTER();

	int type = 0;
	int i = 0;
	u32 value = 0;
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

	type = global_reg_value->h.type;
	switch(type) {
	case DDR3:

		//ddr_phy_cfg_vref()
		i = 0;
		value = 8;
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

		break;
	case DDR2:

		i = 0;
		value = 5;
		/* write leveling dq delay time config */
		//cmd
		for (i = 0; i <= 0x1e;i++) {
			ddr_writel(value, DDR_PHY_OFFSET + (0x100+i)*4);///cmd
		}
		//tx DQ
		for (i = 0; i <= 0x8;i++) {
			ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
			ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
		}
		//tx DQ
		for (i = 0xb; i <= 0x13;i++) {
			ddr_writel(value, DDR_PHY_OFFSET + (0x120+i)*4);//DQ0-DQ15
			ddr_writel(value, DDR_PHY_OFFSET + (0x1a0+i)*4);//DQ15-DQ31
		}
		ddr_writel(2, DDR_PHY_OFFSET + (0x120+0x9)*4);//DQS0-A
		ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0x9)*4);//DQS0-B
		ddr_writel(2, DDR_PHY_OFFSET + (0x120+0xa)*4);//DQS0B-A
		ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0xa)*4);//DQS0B-B
		ddr_writel(2, DDR_PHY_OFFSET + (0x120+0x14)*4);//DQS1-A
		ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0x14)*4);//DQS1-B
		ddr_writel(2, DDR_PHY_OFFSET + (0x120+0x15)*4);//DQS1B-A
		ddr_writel(2, DDR_PHY_OFFSET + (0x1a0+0x15)*4);//DQS1B-B

		writel((readl(0xb3011008))|(0x8), 0xb3011008);
		//printf("PHY REG-02 :  0x%x \n",readl(0xb3011008));

		break;

	default:
		type = UNKOWN;
		printf(" ##unsupport ddr type!\n");
		ddr_hang();
		break;
	}

	FUNC_EXIT();
}





