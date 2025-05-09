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
#include <generated/ddr_reg_values.h>
#endif

#include <asm/io.h>
#include <asm/arch/clk.h>
#include "efuse_ddr.h"

/*#define CONFIG_DWC_DEBUG 0*/
#define ddr_hang() do{						\
		debug("%s %d\n",__FUNCTION__,__LINE__);	\
		hang();						\
	}while(0)

DECLARE_GLOBAL_DATA_PTR;
struct ddr_reg_value *global_reg_value __attribute__ ((section(".data")));


#ifdef  CONFIG_DWC_DEBUG
#define FUNC_ENTER() debug("%s enter.\n",__FUNCTION__);
#define FUNC_EXIT() debug("%s exit.\n",__FUNCTION__);

static void dump_ddrc_register(void)
{
	debug("DDRC_STATUS         0x%x\n", ddr_readl(DDRC_STATUS));
	debug("DDRC_CFG            0x%x\n", ddr_readl(DDRC_CFG));
	debug("DDRC_CTRL           0x%x\n", ddr_readl(DDRC_CTRL));
	debug("DDRC_LMR            0x%x\n", ddr_readl(DDRC_LMR));
	debug("DDRC_DLP            0x%x\n", ddr_readl(DDRC_DLP));
	debug("DDRC_TIMING1        0x%x\n", ddr_readl(DDRC_TIMING(1)));
	debug("DDRC_TIMING2        0x%x\n", ddr_readl(DDRC_TIMING(2)));
	debug("DDRC_TIMING3        0x%x\n", ddr_readl(DDRC_TIMING(3)));
	debug("DDRC_TIMING4        0x%x\n", ddr_readl(DDRC_TIMING(4)));
	debug("DDRC_TIMING5        0x%x\n", ddr_readl(DDRC_TIMING(5)));
	debug("DDRC_REFCNT         0x%x\n", ddr_readl(DDRC_REFCNT));
	debug("DDRC_AUTOSR_CNT     0x%x\n", ddr_readl(DDRC_AUTOSR_CNT));
	debug("DDRC_AUTOSR_EN      0x%x\n", ddr_readl(DDRC_AUTOSR_EN));
	debug("DDRC_MMAP0          0x%x\n", ddr_readl(DDRC_MMAP0));
	debug("DDRC_MMAP1          0x%x\n", ddr_readl(DDRC_MMAP1));
	debug("DDRC_REMAP1         0x%x\n", ddr_readl(DDRC_REMAP(1)));
	debug("DDRC_REMAP2         0x%x\n", ddr_readl(DDRC_REMAP(2)));
	debug("DDRC_REMAP3         0x%x\n", ddr_readl(DDRC_REMAP(3)));
	debug("DDRC_REMAP4         0x%x\n", ddr_readl(DDRC_REMAP(4)));
	debug("DDRC_REMAP5         0x%x\n", ddr_readl(DDRC_REMAP(5)));
	debug("DDRC_DWCFG          0x%x\n", ddr_readl(DDRC_DWCFG));
	debug("DDRC_HREGPRO        0x%x\n", ddr_readl(DDRC_HREGPRO));
	debug("DDRC_PREGPRO        0x%x\n", ddr_readl(DDRC_PREGPRO));
	debug("DDRC_CGUC0          0x%x\n", ddr_readl(DDRC_CGUC0));
	debug("DDRC_CGUC1          0x%x\n", ddr_readl(DDRC_CGUC1));
}

static void dump_ddrp_register(void)
{
	debug("DDRP_INNOPHY_PHY_RST		0x%x\n", ddr_readl(DDRP_INNOPHY_PHY_RST));
	debug("DDRP_INNOPHY_MEM_CFG		0x%x\n", ddr_readl(DDRP_INNOPHY_MEM_CFG));
	debug("DDRP_INNOPHY_DQ_WIDTH		0x%x\n", ddr_readl(DDRP_INNOPHY_DQ_WIDTH));
	debug("DDRP_INNOPHY_CL			0x%x\n", ddr_readl(DDRP_INNOPHY_CL));
	debug("DDRP_INNOPHY_CWL		0x%x\n", ddr_readl(DDRP_INNOPHY_CWL));
	// debug("DDRP_INNOPHY_PLL_FBDIV		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_FBDIV));
	debug("DDRP_INNOPHY_PLL_CTRL		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_CTRL));
	debug("DDRP_INNOPHY_PLL_PDIV		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_PDIV));
	debug("DDRP_INNOPHY_PLL_LOCK		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK));
	debug("DDRP_INNOPHY_TRAINING_CTRL	0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_CTRL));
	debug("DDRP_INNOPHY_CALIB_DONE		0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
	// debug("DDRP_INNOPHY_CALIB_DELAY_AL	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL));
	// debug("DDRP_INNOPHY_CALIB_DELAY_AH	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH));
	// debug("DDRP_INNOPHY_CALIB_BYPASS_AL	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL));
	// debug("DDRP_INNOPHY_CALIB_BYPASS_AH	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH));
	debug("DDRP_INNOPHY_INIT_COMP		0x%x\n", ddr_readl(DDRP_INNOPHY_INIT_COMP));
}

#else
#define FUNC_ENTER()
#define FUNC_EXIT()

#define dump_ddrc_register()
#define dump_ddrp_register()
#endif

static void ddrp_set_drv_odt(unsigned int *ddr_drv_config)
{
    writel(ddr_drv_config[ODT_PD], 0xb3011000 + 4 * 0x140);    // [7:0]ODT pull down
    writel(ddr_drv_config[ODT_PU], 0xb3011000 + 4 * 0x141);    // [7:0]ODT pull up
    writel(ddr_drv_config[ODT_PD], 0xb3011000 + 4 * 0x150);    // [15:8]ODT pull down
    writel(ddr_drv_config[ODT_PU], 0xb3011000 + 4 * 0x151);    // [15:8]ODT pull up

    writel(ddr_drv_config[CMD_RC_PD], 0xb3011000 + 4 * 0x130); // CMD pull down
    writel(ddr_drv_config[CMD_RC_PU], 0xb3011000 + 4 * 0x131); // CMD pull up
    writel(ddr_drv_config[CLK_RC_PD], 0xb3011000 + 4 * 0x132); // CLK pull down
    writel(ddr_drv_config[CLK_RC_PU], 0xb3011000 + 4 * 0x133); // CLK pull up

    writel(ddr_drv_config[DQX_RC_PD], 0xb3011000 + 4 * 0x142); // [7:0]DQ pull down
    writel(ddr_drv_config[DQX_RC_PU], 0xb3011000 + 4 * 0x143); // [7:0]DQ pull up
    writel(ddr_drv_config[DQX_RC_PD], 0xb3011000 + 4 * 0x152); // [15:8]DQ pull down
    writel(ddr_drv_config[DQX_RC_PU], 0xb3011000 + 4 * 0x153); // [15:8]DQ pull up
}


#ifdef CONFIG_DDR_SELF_REFRESH
void sumalte_ddr_self_refresh(void)
{
    unsigned int tmp = 0,ddrc_ctrl;
    *(volatile unsigned int*)(0xb34f0000 + 0xD8) = 0;
    ddr_writel(0, DDRC_AUTOSR_EN);
    tmp = *(volatile unsigned int *)0xa0000000;
    /* DDR self refresh */
    ddrc_ctrl = ddr_readl(DDRC_CTRL);
    ddrc_ctrl |= 1 << 5;
    ddr_writel(ddrc_ctrl, DDRC_CTRL);
    while(!(ddr_readl(DDRC_STATUS) & (1<<2)));

}

void sumalte_ddr_exit_self_refresh(void)
{
    unsigned int tmp = 0,ddrc_ctrl;
    *(volatile unsigned int*)(0xb34f0000 + 0xD8) = 0;
    ddrc_ctrl = *(volatile unsigned int *)0xb34f0010;

    /* exit ddr self refresh */
    ddrc_ctrl &= ~(1<<5);
    ddrc_ctrl |= 1 << 1;
    *(volatile unsigned int*)0xb34f0010 = ddrc_ctrl;

    while((*(volatile unsigned int *)0xb34f0000) & (1<<2));
}
#endif

static void mem_remap(void)
{
	int i;
	unsigned int *remap;

	remap = global_reg_value->REMMAP_ARRAY;


	for(i = 0;i < ARRAY_SIZE(global_reg_value->REMMAP_ARRAY);i++)
	{
		ddr_writel(remap[i], DDRC_REMAP(i+1));
	}
}

static void ddrc_post_init(void)
{
	FUNC_ENTER();

	ddr_writel(global_reg_value->DDRC_REFCNT_VALUE, DDRC_REFCNT);

#ifdef CONFIG_DDR_TYPE_DDR3
	mem_remap();
#endif

    ddr_writel(global_reg_value->DDRC_CTRL_VALUE, DDRC_CTRL);

	ddr_writel(global_reg_value->DDRC_CGUC0_VALUE, DDRC_CGUC0);

    ddr_writel(global_reg_value->DDRC_CGUC1_VALUE, DDRC_CGUC1);

	FUNC_EXIT();
}

static void ddrc_prev_init(void)
{
	FUNC_ENTER();
	ddr_writel(global_reg_value->DDRC_TIMING1_VALUE, DDRC_TIMING(1));
	ddr_writel(global_reg_value->DDRC_TIMING2_VALUE, DDRC_TIMING(2));
	ddr_writel(global_reg_value->DDRC_TIMING3_VALUE, DDRC_TIMING(3));
	ddr_writel(global_reg_value->DDRC_TIMING4_VALUE, DDRC_TIMING(4));
	ddr_writel(global_reg_value->DDRC_TIMING5_VALUE, DDRC_TIMING(5));

	/* DDRC memory map configure*/
	ddr_writel(global_reg_value->DDRC_MMAP0_VALUE, DDRC_MMAP0);
	ddr_writel(global_reg_value->DDRC_MMAP1_VALUE, DDRC_MMAP1);

	/* ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); */
	ddr_writel(global_reg_value->DDRC_CTRL_VALUE & ~(7 << 12), DDRC_CTRL);

	FUNC_EXIT();
}

static enum ddr_type get_ddr_type(void)
{
	int type;

    type = global_reg_value->h.type;
    switch(type){
	case DDR2:
        debug("DDR: %s type is : DDR2\n",global_reg_value->h.name);
		break;
	case DDR3:
        debug("DDR: %s type is : DDR3\n",global_reg_value->h.name);
		break;
	case LPDDR2:
        debug("DDR: %s type is : LPDDR2\n",global_reg_value->h.name);
		break;
	case LPDDR3:
        debug("DDR: %s type is : LPDDR3\n",global_reg_value->h.name);
		break;
	default:
		type = UNKOWN;
		debug("unsupport ddr type!\n");
		ddr_hang();
	}
	return type;
}

static void ddrc_reset_phy(void)
{
	FUNC_ENTER();
	ddr_writel(0xf << 20, DDRC_CTRL);
	mdelay(1);
	ddr_writel(0x8 << 20, DDRC_CTRL);  //dfi_reset_n low for innophy
	mdelay(1);
	FUNC_EXIT();
}

static void ddrp_hardware_calibration(void)
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

void dump_generated_reg(struct ddr_reg_value *reg)
{
	int i;
	serial_debug("name		      = %s\n", reg->h.name);
	serial_debug("id		      = %x\n", reg->h.id);
	serial_debug("type		      = %x\n", reg->h.type);
	serial_debug("freq		      = %x\n", reg->h.freq);
	serial_debug("DDRC_CFG_VALUE        = %x\n", reg->DDRC_CFG_VALUE);
	serial_debug("DDRC_CTRL_VALUE       = %x\n", reg->DDRC_CTRL_VALUE);
	serial_debug("DDRC_DLMR_VALUE       = %x\n", reg->DDRC_DLMR_VALUE);
	serial_debug("DDRC_DDLP_VALUE       = %x\n", reg->DDRC_DDLP_VALUE);
	serial_debug("DDRC_MMAP0_VALUE      = %x\n", reg->DDRC_MMAP0_VALUE);
	serial_debug("DDRC_MMAP1_VALUE      = %x\n", reg->DDRC_MMAP1_VALUE);
	serial_debug("DDRC_REFCNT_VALUE     = %x\n", reg->DDRC_REFCNT_VALUE);
	serial_debug("DDRC_TIMING1_VALUE    = %x\n", reg->DDRC_TIMING1_VALUE);
	serial_debug("DDRC_TIMING2_VALUE    = %x\n", reg->DDRC_TIMING2_VALUE);
	serial_debug("DDRC_TIMING3_VALUE    = %x\n", reg->DDRC_TIMING3_VALUE);
	serial_debug("DDRC_TIMING4_VALUE    = %x\n", reg->DDRC_TIMING4_VALUE);
	serial_debug("DDRC_TIMING5_VALUE    = %x\n", reg->DDRC_TIMING5_VALUE);
	serial_debug("DDRC_AUTOSR_CNT_VALUE = %x\n", reg->DDRC_AUTOSR_CNT_VALUE);
	serial_debug("DDRC_AUTOSR_EN_VALUE  = %x\n", reg->DDRC_AUTOSR_EN_VALUE);
	serial_debug("DDRC_HREGPRO_VALUE    = %x\n", reg->DDRC_HREGPRO_VALUE);
	serial_debug("DDRC_PREGPRO_VALUE    = %x\n", reg->DDRC_PREGPRO_VALUE);
	serial_debug("DDRC_CGUC0_VALUE      = %x\n", reg->DDRC_CGUC0_VALUE);
	serial_debug("DDRC_CGUC1_VALUE      = %x\n", reg->DDRC_CGUC1_VALUE);
	serial_debug("DDRP_MEMCFG_VALUE     = %x\n", reg->DDRP_MEMCFG_VALUE);
	serial_debug("DDRP_CL_VALUE         = %x\n", reg->DDRP_CL_VALUE);
	serial_debug("DDRP_CWL_VALUE        = %x\n", reg->DDRP_CWL_VALUE);
	serial_debug("DDR_MR0_VALUE         = %x\n", reg->DDR_MR0_VALUE);
	serial_debug("DDR_MR1_VALUE         = %x\n", reg->DDR_MR1_VALUE);
	serial_debug("DDR_MR2_VALUE         = %x\n", reg->DDR_MR2_VALUE);
	serial_debug("DDR_MR3_VALUE         = %x\n", reg->DDR_MR3_VALUE);
	serial_debug("DDR_MR10_VALUE        = %x\n", reg->DDR_MR10_VALUE);
	serial_debug("DDR_MR11_VALUE        = %x\n", reg->DDR_MR11_VALUE);
	serial_debug("DDR_MR63_VALUE        = %x\n", reg->DDR_MR63_VALUE);
	serial_debug("DDR_CHIP_0_SIZE       = %x\n", reg->DDR_CHIP_0_SIZE);
	serial_debug("DDR_CHIP_1_SIZE       = %x\n", reg->DDR_CHIP_1_SIZE);
	for(i = 0; i < 5; i++) {
		serial_debug("REMMAP_ARRAY[%d] = %x\n", i, reg->REMMAP_ARRAY[i]);
	}

}

#ifndef CONFIG_BURNER

__weak unsigned int check_socid(void)
{
        return -1;
}


int get_ddr_params_socid(void)
{
	int i;
	int found = 0;
	uint32_t ddrid = 0;
	uint32_t mask = ~(7 << 3);

	ddrid = check_socid();
	if ((int)ddrid < 0) {
		debug("Check socid return invalid ddr id %x\n",ddrid);
		return -1;
	}

	for(i = 0; i < ARRAY_SIZE(supported_ddr_reg_values); i++) {
		global_reg_value = &supported_ddr_reg_values[i];
		if((ddrid & mask) == (global_reg_value->h.id & mask)) {
			found = 1;
			break;
		}
	}

	if(found == 0) {
		debug("Check socid not match to %x\n",ddrid);
		return -1;
	}

	return 0;
}

void get_ddr_params_normal(void)
{
	int found = 0;
	int size = 0;
	int i;
	unsigned int burned_ddr_id = *(volatile unsigned int *)(CONFIG_SPL_TEXT_BASE + 128);
	uint32_t mask = ~(7 << 3);

	if((burned_ddr_id & 0xffff) != (burned_ddr_id >> 16)) {
		debug("invalid burned ddr id\n");
	}

	burned_ddr_id &= 0xffff;

	for(i = 0; i < ARRAY_SIZE(supported_ddr_reg_values); i++) {
		global_reg_value = &supported_ddr_reg_values[i];
		if((burned_ddr_id & mask) == (global_reg_value->h.id & mask)) {
			found = 1;
			break;
		}
	}

	if(found == 0) {
		debug("No match to %x\n",burned_ddr_id);
	}

}
#else
void get_ddr_params_burner(void)
{
	/* keep ddr_reg_value inc ddr_innophy.h
	 * with ddr_registers the same
	 * */
	global_reg_value = g_ddr_param;
}
#endif

void get_ddr_params(void)
{
#ifndef CONFIG_BURNER
	if(ARRAY_SIZE(supported_ddr_reg_values) == 1)
		global_reg_value = &supported_ddr_reg_values[0];
	else if (get_ddr_params_socid() < 0)
		get_ddr_params_normal();
#else
	get_ddr_params_burner();
#endif
	//dump_generated_reg(global_reg_value);

}

unsigned int get_ddr_size(void)
{
	return (global_reg_value->DDR_CHIP_0_SIZE) >> 20;
}

/*
 * Name     : ddrp_calibration_manual()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
static void ddrp_reg_set_range(u32 offset, u32 startbit, u32 bitscnt, u32 value)
{
	u32 reg = 0;
	u32 mask = 0;
	mask = ((0xffffffff>>startbit)<<(startbit))&((0xffffffff<<(32-startbit-bitscnt))>>(32-startbit-bitscnt));
	reg = readl(DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
	reg = (reg&(~mask))|((value<<startbit)&mask);
	//debug("value = %x, reg = %x, mask = %x", value, reg, mask);
	writel(reg, DDRC_BASE+DDR_PHY_OFFSET+(offset*4));
}

#ifdef CONFIG_DDRP_SOFTWARE_TRAINING
static void ddrp_software_calibration(void)
{
	int c;
	unsigned int addr = 0xa1000000;
	unsigned int i, m = 0;
	unsigned int sel = 0;
	int ret = -1;
	int q;

	unsigned int ddbuf[8] = {0};
	unsigned short calv[0x3ff] = {0};
	ddrp_reg_set_range(0x2, 1, 1, 1);

    for(c = 0; c <=0x3ff; c++) {
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AL1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AL2);
		ddr_writel((c>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AH1);
		ddr_writel((c&0x1f)<<3 | ((c>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AH2);
		unsigned int value = 0x12345678;
		for(i = 0; i < 4 * 1024; i += 8) {
			*(volatile unsigned int *)(addr + (i + 0) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 1) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 2) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 3) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 4) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 5) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 6) * 4) = value;
			*(volatile unsigned int *)(addr + (i + 7) * 4) = value;
		}
		for(i = 0; i < 4 * 1024; i += 8) {
			ddbuf[0] = *(volatile unsigned int *)(addr + (i + 0) * 4);
			ddbuf[1] = *(volatile unsigned int *)(addr + (i + 1) * 4);
			ddbuf[2] = *(volatile unsigned int *)(addr + (i + 2) * 4);
			ddbuf[3] = *(volatile unsigned int *)(addr + (i + 3) * 4);
			ddbuf[4] = *(volatile unsigned int *)(addr + (i + 4) * 4);
			ddbuf[5] = *(volatile unsigned int *)(addr + (i + 5) * 4);
			ddbuf[6] = *(volatile unsigned int *)(addr + (i + 6) * 4);
			ddbuf[7] = *(volatile unsigned int *)(addr + (i + 7) * 4);
			for(q = 0; q < 8; q++) {
				if ((ddbuf[q]&0xffff0000) != (value&0xffff0000)) {
					;//debug("#####################################   high error want 0x%x get 0x%x\n", value, ddbuf[q]);
				}
				if ((ddbuf[q]&0xffff) != (value&0xffff)) {
					//debug("SET AL,AH %x q[%d] fail want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = -1;
					break;
				} else {
					//debug("SET AL,AH %x q[%d] pass want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					//debug("SET %d  AL[%d] pass want 0x%x  get 0x%x \n", c, q, value, ddbuf[q]);
					ret = 0;
				}
			}
			if (ret) {
				break;
			}
		}

		if(i == 4 * 1024) {
			calv[m] = c;
			m++;
			debug("calib a once idx = %d,  value = %x\n", m, c);
		}
	}

	if(!m) {
		debug("####################### AL a calib bypass fail\n");
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_AL1);
		ddr_writel(0x1c, DDRP_INNOPHY_CALIB_DELAY_AH1);
		//		return;
	} else {
		/* choose the middle parameter */
		sel = m * 1 / 2;
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AL1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AL2);
		ddr_writel((calv[sel]>>7)&0x7, DDRP_INNOPHY_CALIB_DELAY_AH1);
		ddr_writel((calv[sel]&0x1f)<<3 | ((calv[sel]>>5)&0x3), DDRP_INNOPHY_CALIB_DELAY_AH2);
	}
}
#endif

static void ddr_phy_init(unsigned int rate, int pll_sel)
{
	unsigned int val;
	int i = 0;
	/***
	 * pllprediv[4:0]:0x52[4:0]() must set 1
	 * pllfbdiv[8:0]:0x51[0],0x50[7:0] must set 1
	 * pllpostdiven:0x51[7] must set 1,使能postdiv
	 * pllpostdiv:0x53[7:5] 不同频率配置不同参数，具体如下(0x53寄存器)：
	 * 推荐配置	0x20: 625MHz ~ 1066 MHz
	 *			0x40: 280MHz ~ 625MHz
	 *			0x60: 140MHz ~ 280MHz
	 *			0xc0: 70MHz  ~ 140MHz
	 *			0xe0: 35MHz  ~ 70MHz
	 ***/

retry:
	if(pll_sel > 1)
	{
		//printf("ddr_phy_init fail!\n");
		do_reset(NULL, 0, 0, NULL);
	}
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
	if(rate > 625000000)
	{
		if(pll_sel)
			val |= 0x48;
		else
			val |= 0x28;
	}else{
		if(pll_sel)
			val |= 0x28;
		else
			val |= 0x48;
	}
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);
	udelay(500);
	serial_debug("DDRP_INNOPHY_PLL_CTRL: %x\n", ddr_readl(DDRP_INNOPHY_PLL_CTRL));
	/* pllprediv */
	val = ddr_readl(DDRP_INNOPHY_PLL_PDIV);
	val &= ~(0x1f);
	val |= 0x1;
	ddr_writel(val, DDRP_INNOPHY_PLL_PDIV);

	val = ddr_readl(DDRP_INNOPHY_PLL_CTRL);
	val &= ~(0xff);
	if(rate > 625000000)
	{
		if(pll_sel)
			val = 0x40;
		else
			val = 0x20;
	}else{
		if(pll_sel)
			val = 0x20;
		else
			val = 0x40;
	}
	ddr_writel(val, DDRP_INNOPHY_PLL_CTRL);
	udelay(500);
	serial_debug("DDRP_INNOPHY_PLL_CTRL: %x\n", ddr_readl(DDRP_INNOPHY_PLL_CTRL));
	while(!(ddr_readl(DDRP_INNOPHY_PLL_LOCK) & 1 << 2))
	{
		i++;
		if(i > 500)
		{
			pll_sel++;
			ddrc_reset_phy();
			goto retry;
		}
	}

	val = ddr_readl(DDRP_INNOPHY_MEM_CFG);
	val &= ~(0xff);
	val |= global_reg_value->DDRP_MEMCFG_VALUE;
	ddr_writel(val, DDRP_INNOPHY_MEM_CFG);

	val = DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L; /* 0x3:16bit */
	ddr_writel(val, DDRP_INNOPHY_DQ_WIDTH);

	val = ddr_readl(DDRP_INNOPHY_INNO_PHY_RST);
	val &= ~(0xff);
	val |= 0x0d;
	ddr_writel(val, DDRP_INNOPHY_INNO_PHY_RST);

    	/*config CWL*/
	val = ddr_readl(DDRP_INNOPHY_CWL);
	val &= ~(0xf);
	val |= global_reg_value->DDRP_CWL_VALUE;
	/*val |= 0x6;*/
	ddr_writel(val, DDRP_INNOPHY_CWL);

    	/*config CL*/
	val = ddr_readl(DDRP_INNOPHY_CL);
	val &= ~(0xf);
	val |= global_reg_value->DDRP_CL_VALUE;
	/*val |= 0x7;*/
	ddr_writel(val, DDRP_INNOPHY_CL);


	val = 0x0;
	ddr_writel(val, DDRP_INNOPHY_AL);
}

static void ddrc_dfi_init(enum ddr_type type, int bypass, unsigned int kgd_rtt_dic)
{
	FUNC_ENTER();
	ddr_writel(DDRC_DWCFG_DFI_INIT_START, DDRC_DWCFG); // dfi_init_start high
	ddr_writel(0, DDRC_DWCFG); // set buswidth 16bit
	while(!(ddr_readl(DDRC_DWSTATUS) & DDRC_DWSTATUS_DFI_INIT_COMP)); //polling dfi_init_complete

    ddr_writel(0, DDRC_CTRL); //set dfi_reset_n high
	udelay(5);
    ddr_writel(global_reg_value->DDRC_CFG_VALUE, DDRC_CFG);
    udelay(5);
    ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); // set CKE to high
    udelay(5);

#define DDRC_LMR_MR(n)												\
	(global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |		\
	((global_reg_value->DDR_MR##n##_VALUE & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |	\
		(((global_reg_value->DDR_MR##n##_VALUE >> 16) & 0x7) << DDRC_LMR_BA_BIT))

	ddr_writel(0, DDRC_LMR);udelay(5);
	ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR2
	udelay(5);
	ddr_writel(0, DDRC_LMR);udelay(5);
	ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR3
	udelay(5);
	ddr_writel(0, DDRC_LMR);udelay(5);
	ddr_writel((DDRC_LMR_MR(1) & (~0x266000)) | (kgd_rtt_dic << 12), DDRC_LMR); //MR1
	udelay(5);
	ddr_writel(0, DDRC_LMR);udelay(5);
	ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR0
	udelay(5);
	ddr_writel(global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_ZQCL_CS0, DDRC_LMR); //ZQCL
	mdelay(5);
#undef DDRC_LMR_MR

	FUNC_EXIT();
}

static void ddr_set_vref_skew(unsigned int *ddr_drv_config)
{
    int DQxOFFSET_rx[] = {0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x17, 0x19, 0x1b, 0x1d, 0x1f, 0x21, 0x23, 0x25}; // DQxxx RX寄存器偏移
    int DQxOFFSET_tx[] = {0x3, 0x5, 0x7, 0x9, 0xb, 0xd, 0xf, 0x11, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26}; // DQxxx TX寄存器偏移

    int i, wl;

    if (0 == ddr_drv_config[SKEW_TRX])
        return ;
    serial_debug("x2580 InnoPhy skew Settings...\n");

    // VREF A_L
    ddrp_reg_set_range(0x147, 0, 8, ddr_drv_config[VREF]);
    // VREF A_H
    ddrp_reg_set_range(0x157, 0, 8, ddr_drv_config[VREF]);

    /* RX */
    if (ddr_drv_config[SKEW_TRX] & 0x1)
    {
        writel(ddr_drv_config[SKEW_DQS0R], 0xb3011000 + (0x1c0 + 0x00) * 4); // RX DM0
        writel(ddr_drv_config[SKEW_DQS0R], 0xb3011000 + (0x1c0 + 0x12) * 4); // RX DQS0
        writel(ddr_drv_config[SKEW_DQS0R], 0xb3011000 + (0x1c0 + 0x2a) * 4); // RX DQSB0
        writel(ddr_drv_config[SKEW_DQS1R], 0xb3011000 + (0x1c0 + 0x15) * 4); // RX DM1
        writel(ddr_drv_config[SKEW_DQS1R], 0xb3011000 + (0x1c0 + 0x27) * 4); // RX DQS1
        writel(ddr_drv_config[SKEW_DQS1R], 0xb3011000 + (0x1c0 + 0x2b) * 4); // RX DQSB1

        for (i = 0; i < 16; i++)
        {
            writel(ddr_drv_config[SKEW_DQRX], 0xb3011000 + (0x1c0 + DQxOFFSET_rx[i]) * 4); // RX DQX
        }
    }

    /* TX */
    if (ddr_drv_config[SKEW_TRX] & 0x2)
    {
        /* TX BYPASS FUNCTION */
        wl = readl(0xb3011000 + (0x2) * 4);
        wl |= 0x8;
        writel(wl, 0xb3011000 + (0x2) * 4);

        writel(ddr_drv_config[SKEW_DQS0T], 0xb3011000 + (0x1c0 + 0x01) * 4); // TX DM0
        writel(ddr_drv_config[SKEW_DQS0T], 0xb3011000 + (0x1c0 + 0x13) * 4); // TX DQS0
        writel(ddr_drv_config[SKEW_DQS0T], 0xb3011000 + (0x1c0 + 0x14) * 4); // TX DQSB0
        writel(ddr_drv_config[SKEW_DQS1T], 0xb3011000 + (0x1c0 + 0x16) * 4); // TX DM1
        writel(ddr_drv_config[SKEW_DQS1T], 0xb3011000 + (0x1c0 + 0x28) * 4); // TX DQS1
        writel(ddr_drv_config[SKEW_DQS1T], 0xb3011000 + (0x1c0 + 0x29) * 4); // TX DASB1

        // cmd
        for (i = 0; i < 0x1d; i++)
        {
            writel(ddr_drv_config[SKEW_DQS0T], 0xb3011000 + (0x340 + i) * 4);
        }
        writel(ddr_drv_config[SKEW_DQS0T], 0xb3011000 + (0x340 + 0x1e) * 4);
        writel(ddr_drv_config[SKEW_DQS0T], 0xb3011000 + (0x340 + 0x1f) * 4);

        for (i = 0; i < 16; i++)
        {
            writel(ddr_drv_config[SKEW_DQTX], 0xb3011000 + (0x1c0 + DQxOFFSET_tx[i]) * 4);
        }
    }
}

/* DDR sdram init */
void sdram_init(int pll_sel)
{
	enum ddr_type type;
	unsigned int ddr_drv_config[INDEX_EN] = {0};
	unsigned int rate;
	int bypass = 0;

#ifndef CONFIG_UART_LOAD
	serial_debug("sdram init start\n");
#endif

	get_ddr_params();
	type = get_ddr_type();
#ifndef CONFIG_FPGA
	clk_set_rate(DDR, global_reg_value->h.freq);
	rate = clk_get_rate(DDR);
#else
	rate = gd->arch.gi->ddrfreq;
#endif

#ifndef CONFIG_UART_LOAD
	serial_debug("DDR clk rate %d\n", rate);
#endif
	ddrc_reset_phy();

	get_ddr_par(ddr_drv_config, sizeof(ddr_drv_config));

	ddr_phy_init(rate, pll_sel);

	ddrc_dfi_init(type, bypass, ddr_drv_config[KGD_RTT_DIC]);

	ddrp_set_drv_odt(ddr_drv_config);
	/* DDR Controller init*/
	ddrc_prev_init();

#ifdef CONFIG_DDR_HARDWARE_TRAINING
	ddrp_hardware_calibration();
#endif/*CONFIG_DDR_HARDWARE_TRAINING*/
#ifdef CONFIG_DDRP_SOFTWARE_TRAINING
	ddrp_software_calibration();
#endif/*CONFIG_DDRP_SOFTWARE_TRAINING*/

	dump_ddrp_register();

	ddrc_post_init();

	ddr_writel(global_reg_value->DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
	/*一些数据访问相关的配置，自动控制的配置，应该在training之后，防止training过程中出现干扰.*/
	if(global_reg_value->DDRC_AUTOSR_EN_VALUE) {
		/* ddr_writel(DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT); */
		ddr_writel(1, DDRC_AUTOSR_EN);
	} else {
		ddr_writel(0, DDRC_AUTOSR_EN);
	}

	ddr_writel(global_reg_value->DDRC_HREGPRO_VALUE, DDRC_HREGPRO);
	ddr_writel(global_reg_value->DDRC_PREGPRO_VALUE, DDRC_PREGPRO);

	dump_ddrc_register();

	debug("DDR size is : %d MByte\n", (global_reg_value->DDR_CHIP_0_SIZE + global_reg_value->DDR_CHIP_1_SIZE) / 1024 /1024);
	/* DDRC address remap configure*/

    /* Optimize DDR bandwidth */
	writel(0x0, 0xb301206c);
	writel(0xff000000, 0xb3012040);
	writel(0x2bd07460, 0xb3012048);
	writel(0x1, 0xb301206c);

	ddr_set_vref_skew(ddr_drv_config);
	debug("sdram init finished\n");
}

phys_size_t initdram(int board_type)
{
	/* SDRAM size was calculated when compiling. */
#ifndef EMC_LOW_SDRAM_SPACE_SIZE
#define EMC_LOW_SDRAM_SPACE_SIZE 0x20000000 /* 512M */
#endif /* EMC_LOW_SDRAM_SPACE_SIZE */

	unsigned int ram_size;

	/*init ddr params in uboot env. */
	get_ddr_params();
	ram_size = (unsigned int)(global_reg_value->DDR_CHIP_0_SIZE) + (unsigned int)(global_reg_value->DDR_CHIP_1_SIZE);
	if (ram_size > EMC_LOW_SDRAM_SPACE_SIZE)
		ram_size = EMC_LOW_SDRAM_SPACE_SIZE;
	serial_debug("ram_size=%x\n", ram_size);


	return ram_size;
}
