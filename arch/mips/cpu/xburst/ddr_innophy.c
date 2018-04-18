
/*
 * DDR driver for inno DDR PHY.
 * Used by x1630
 *
 * Copyright (C) 2017 Ingenic Semiconductor Co.,Ltd
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

/* #define DEBUG */
#include <config.h>
#include <common.h>
#include <ddr/ddr_common.h>
#include <generated/ddr_reg_values.h>
#include <asm/arch/clk.h>
#include "ddr_innophy.h"
#define CONFIG_DWC_DEBUG 1
#include "ddr_debug.h"

DECLARE_GLOBAL_DATA_PTR;
extern unsigned int sdram_size(int cs, struct ddr_params *p);

struct ddr_params *ddr_params_p = NULL;
extern void reset_dll(void);
#define BYPASS_ENABLE       1
#define BYPASS_DISABLE      0
#define IS_BYPASS_MODE(x)     (((x) & 1) == BYPASS_ENABLE)
#define DDR_TYPE_MODE(x)     (((x) >> 1) & 0xf)

static void dump_ddrc_register(void)
{
#ifdef CONFIG_DWC_DEBUG
	printf("DDRC_STATUS         0x%x\n", ddr_readl(DDRC_STATUS));
	printf("DDRC_CFG            0x%x\n", ddr_readl(DDRC_CFG));
	printf("DDRC_CTRL           0x%x\n", ddr_readl(DDRC_CTRL));
	printf("DDRC_LMR            0x%x\n", ddr_readl(DDRC_LMR));
	printf("DDRC_DLP            0x%x\n", ddr_readl(DDRC_DLP));
	printf("DDRC_TIMING1        0x%x\n", ddr_readl(DDRC_TIMING(1)));
	printf("DDRC_TIMING2        0x%x\n", ddr_readl(DDRC_TIMING(2)));
	printf("DDRC_TIMING3        0x%x\n", ddr_readl(DDRC_TIMING(3)));
	printf("DDRC_TIMING4        0x%x\n", ddr_readl(DDRC_TIMING(4)));
	printf("DDRC_TIMING5        0x%x\n", ddr_readl(DDRC_TIMING(5)));
	printf("DDRC_TIMING6        0x%x\n", ddr_readl(DDRC_TIMING(6)));
	printf("DDRC_REFCNT         0x%x\n", ddr_readl(DDRC_REFCNT));
	printf("DDRC_MMAP0          0x%x\n", ddr_readl(DDRC_MMAP0));
	printf("DDRC_MMAP1          0x%x\n", ddr_readl(DDRC_MMAP1));
	printf("DDRC_REMAP1         0x%x\n", ddr_readl(DDRC_REMAP(1)));
	printf("DDRC_REMAP2         0x%x\n", ddr_readl(DDRC_REMAP(2)));
	printf("DDRC_REMAP3         0x%x\n", ddr_readl(DDRC_REMAP(3)));
	printf("DDRC_REMAP4         0x%x\n", ddr_readl(DDRC_REMAP(4)));
	printf("DDRC_REMAP5         0x%x\n", ddr_readl(DDRC_REMAP(5)));
	printf("DDRC_AUTOSR_EN      0x%x\n", ddr_readl(DDRC_AUTOSR_EN));
	printf("INNO_DQ_WIDTH   :%X\n",phy_readl(INNO_DQ_WIDTH));
	printf("INNO_PLL_FBDIV  :%X\n",phy_readl(INNO_PLL_FBDIV));
	printf("INNO_PLL_PDIV   :%X\n",phy_readl(INNO_PLL_PDIV));
	printf("INNO_MEM_CFG    :%X\n",phy_readl(INNO_MEM_CFG));
	printf("INNO_PLL_CTRL   :%X\n",phy_readl(INNO_PLL_CTRL));
	printf("INNO_CHANNEL_EN :%X\n",phy_readl(INNO_CHANNEL_EN));
	printf("INNO_CWL        :%X\n",phy_readl(INNO_CWL));
	printf("INNO_CL         :%X\n",phy_readl(INNO_CL));
#endif
}

static void reset_controller(void)
{
        ddr_writel(0xf << 20, DDRC_CTRL);
        mdelay(5);
        ddr_writel(0, DDRC_CTRL);
        mdelay(5);
}

void ddr_controller_init(void)
{
	dwc_debug("DDR Controller init\n");
	ddr_writel(DDRC_CTRL_CKE | DDRC_CTRL_ALH, DDRC_CTRL);
	ddr_writel(0, DDRC_CTRL);
	/* DDRC CFG init*/
	ddr_writel(DDRC_CFG_VALUE, DDRC_CFG);
	/* DDRC timing init*/
	ddr_writel(DDRC_TIMING1_VALUE, DDRC_TIMING(1));
	ddr_writel(DDRC_TIMING2_VALUE, DDRC_TIMING(2));
	ddr_writel(DDRC_TIMING3_VALUE, DDRC_TIMING(3));
	ddr_writel(DDRC_TIMING4_VALUE, DDRC_TIMING(4));
	ddr_writel(DDRC_TIMING5_VALUE, DDRC_TIMING(5));
	ddr_writel(DDRC_TIMING6_VALUE, DDRC_TIMING(6));

	/* DDRC memory map configure*/
	ddr_writel(DDRC_MMAP0_VALUE, DDRC_MMAP0);
	ddr_writel(DDRC_MMAP1_VALUE, DDRC_MMAP1);
	ddr_writel(DDRC_CTRL_CKE | DDRC_CTRL_ALH, DDRC_CTRL);
	ddr_writel(DDRC_REFCNT_VALUE, DDRC_REFCNT);
	ddr_writel(DDRC_CTRL_VALUE & 0xffff8fff, DDRC_CTRL);
}

/*
 * Name     : phy_calibration()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay		= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
void phy_calibration(int al8_1x,int ah8_1x,int al8_2x,int ah8_2x)
{
	printf("X1630_0x5: %x\n",readl(PHY_BASE + 0x14));
	printf("X1630_0x15: %x\n",readl(PHY_BASE + 0x54));
	printf("X1630_0x4: %x\n",readl(PHY_BASE + 0x10));
	printf("X1630_0x14: %x\n",readl(PHY_BASE + 0x50));

	int m=phy_readl(INNO_TRAINING_CTRL);
	printf("INNO_TRAINING_CTRL 1: %x\n", phy_readl(INNO_TRAINING_CTRL));
	m=(0xa1);
	phy_writel(m,INNO_TRAINING_CTRL);
	printf("INNO_TRAINING_CTRL 2: %x\n", phy_readl(INNO_TRAINING_CTRL));
	while (0x3 != readl((PHY_BASE + 0xcc)));
	printf("X1630_cc: %x\n", readl((PHY_BASE + 0xcc)));
	phy_writel(0xa0,INNO_TRAINING_CTRL);
	printf("INNO_TRAINING_CTRL 3: %x\n", phy_readl(INNO_TRAINING_CTRL));
	printf("X1630_190: %x\n", readl((PHY_BASE + 0x190)));
	printf("X1630_194: %x\n", readl((PHY_BASE + 0x194)));
	printf("X1630_REG56: %x\n", readl(X1630_REG56));

}

void ddr_inno_phy_init(void)
{
	u32 reg = 0;
	printf("ddr_inno_phy_init ..!\n");

	phy_writel(0x14,INNO_PLL_FBDIV);
	phy_writel(0x1a,INNO_PLL_CTRL);
	phy_writel(0x5,INNO_PLL_PDIV);
	phy_writel(0x18,INNO_PLL_CTRL);

	phy_writel(0x0,INNO_TRAINING_CTRL);
	phy_writel(0x03,INNO_DQ_WIDTH);

	phy_writel(0x11,INNO_MEM_CFG);  // MEMSEL  =  DDR2  ,    BURSEL = burst8
	phy_writel(0x0d,INNO_CHANNEL_EN);
	phy_writel(((DDRP_MR0_VALUE&0xf0)>>4)-1, INNO_CWL);
	reg = ((DDRP_MR0_VALUE&0xf0)>>4);
	phy_writel(reg, INNO_CL);
	printf("phy reg = 0x%x, CL = 0x%x\n", reg, phy_readl(INNO_CL));
	phy_writel(0x00,INNO_AL);

        writel(0,DDR_APB_PHY_INIT); //start high
	while(!(readl(DDR_APB_PHY_INIT) & (1<<2)));//pll locked
	printf("ddr_inno_phy_init ..! 11:  %X\n", readl(DDR_APB_PHY_INIT));
        writel(0,REG_DDR_CTRL);

        while(!(readl(DDR_APB_PHY_INIT) & (1<<1))); //init_complete
	printf("ddr_inno_phy_init ..! 22:  %X\n", readl(DDR_APB_PHY_INIT));
        while(!readl(X1630_INIT_COMP));
	printf("ddr_inno_phy_init ..! 33:  %X\n", readl(DDR_APB_PHY_INIT));
	writel(0,REG_DDR_CTRL);

	writel(DDRC_CFG_VALUE,REG_DDR_CFG);// r=13 , c=10 , bank=4 , bitwidth=16 ,  0x0a688a40
	writel(0x0a,REG_DDR_CTRL);

	writel(0x211,REG_DDR_LMR);
        printf("REG_DDR_LMR: %x\n",readl(REG_DDR_LMR));
	writel(0,REG_DDR_LMR);

        writel(0x311,REG_DDR_LMR);
	printf("REG_DDR_LMR: %x\n", readl(REG_DDR_LMR));
	writel(0,REG_DDR_LMR);

	writel(0x111,REG_DDR_LMR);
	printf("REG_DDR_LMR: %x\n", readl(REG_DDR_LMR));
	writel(0,REG_DDR_LMR);

	reg = ((DDRP_MR0_VALUE)<<12)|0x011;
	writel(reg, REG_DDR_LMR);
	printf("REG_DDR_LMR, MR0: %x\n", reg);
	writel(0,REG_DDR_LMR);

        phy_calibration(0x1,0x1,0x1,0x1);

	writel(0x51,0xb3011004);
	writel(0x24,0xb3011028);
	dwc_debug("DDR PHY init OK\n");
}

void phy_dqs_delay(int delay_l,int delay_h)
{
	writel(delay_l,X1630_DQS_DELAY_L);
	writel(delay_h,X1630_DQS_DELAY_H);

	printf("X1630_DQS_DELAY_L: %x\n",readl(X1630_DQS_DELAY_L));
	printf("X1630_DQS_DELAY_H: %x\n",readl(X1630_DQS_DELAY_H));
}

/* DDR sdram init */
void sdram_init(void)
{
	int type = DDR2;
	unsigned int mode;
	unsigned int bypass = 0;
	unsigned int rate;

	dwc_debug("sdram init start\n");
	clk_set_rate(DDR, CONFIG_SYS_MEM_FREQ);
	reset_dll();
	rate = clk_get_rate(DDR);
	rate = CONFIG_SYS_MEM_FREQ;

        reset_controller();

#ifdef CONFIG_DDR_AUTO_SELF_REFRESH
	ddr_writel(0x0 ,DDRC_AUTOSR_EN);
#endif

	ddr_inno_phy_init();

        /* DDR Controller init*/
	ddr_controller_init();

	ddr_writel(ddr_readl(DDRC_STATUS) & ~DDRC_DSTATUS_MISS, DDRC_STATUS);

#ifdef CONFIG_DDR_AUTO_SELF_REFRESH
	if(!bypass)
		ddr_writel(0 , DDRC_DLP);
	ddr_writel(0x1 ,DDRC_AUTOSR_EN);
#endif
	ddr_writel(0 , DDRC_DLP);

	dump_ddrc_register();
	dwc_debug("sdram init finished\n");
#undef DDRTYPE
        (void)rate;(void)bypass;(void)mode;(void)type;
}

phys_size_t initdram(int board_type)
{
#ifndef EMC_LOW_SDRAM_SPACE_SIZE
#define EMC_LOW_SDRAM_SPACE_SIZE 0x10000000 /* 256M */
#endif /* EMC_LOW_SDRAM_SPACE_SIZE */
        unsigned int ram_size;
        ram_size = (unsigned int)(DDR_CHIP_0_SIZE) + (unsigned int)(DDR_CHIP_1_SIZE);
        if (ram_size > EMC_LOW_SDRAM_SPACE_SIZE)
                ram_size = EMC_LOW_SDRAM_SPACE_SIZE;

        return ram_size;
}
