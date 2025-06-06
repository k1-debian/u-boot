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

/* #define DEBUG */
#include <config.h>
#include <common.h>
#include <ddr/ddr_common.h>
#include <asm/io.h>
#include <asm/arch/clk.h>

#ifndef CONFIG_BURNER
#include <generated/ddr_reg_values.h>
#undef	DDRC_CFG_VALUE
#undef  DDRC_MMAP0_VALUE
#undef	DDRC_MMAP1_VALUE
#undef	DDRC_TIMING4_VALUE
#undef  DDRC_AUTOSR_EN_VALUE
#undef	DDR_CHIP_0_SIZE
#define DDRC_CFG_VALUE	(gd->arch.gi->ddr_change_param.ddr_cfg)
#define	DDRC_MMAP0_VALUE (gd->arch.gi->ddr_change_param.ddr_mmap0)
#define DDRC_MMAP1_VALUE (gd->arch.gi->ddr_change_param.ddr_mmap1)
#define DDRC_TIMING4_VALUE (gd->arch.gi->ddr_change_param.ddr_timing4)
#define DDRC_AUTOSR_EN_VALUE (gd->arch.gi->ddr_change_param.ddr_autosr)
#define remap_array (gd->arch.gi->ddr_change_param.ddr_remap_array)
static unsigned int out_imp_table[] = DDRP_IMPANDCE_ARRAY;
static unsigned int odt_imp_table[] = DDRP_ODT_IMPANDCE_ARRAY;
static unsigned char rzq_table[] = DDRP_RZQ_TABLE;
#else
#include "ddr_reg_data.h"
#define out_imp_table DDRP_IMPANDCE_ARRAY
#define odt_imp_table DDRP_ODT_IMPANDCE_ARRAY
#define rzq_table DDRP_RZQ_TABLE
#define remap_array REMMAP_ARRAY
#endif

/* #define CONFIG_DWC_DEBUG 1 */
#include "ddr_debug.h"
#define ddr_hang() do{								\
		serial_debug("%s %d\n",__FUNCTION__,__LINE__);	\
		hang();										\
	}while(0)

DECLARE_GLOBAL_DATA_PTR;

#ifdef  CONFIG_DWC_DEBUG
#define FUNC_ENTER() serial_debug("%s enter.\n",__FUNCTION__);
#define FUNC_EXIT() serial_debug("%s exit.\n",__FUNCTION__);

static void dump_ddrc_register(void)
{
	serial_debug("DDRC_STATUS		0x%x\n", ddr_readl(DDRC_STATUS));
	serial_debug("DDRC_CFG		0x%x\n", ddr_readl(DDRC_CFG));
	serial_debug("DDRC_CTRL		0x%x\n", ddr_readl(DDRC_CTRL));
	serial_debug("DDRC_LMR		0x%x\n", ddr_readl(DDRC_LMR));
	serial_debug("DDRC_TIMING1		0x%x\n", ddr_readl(DDRC_TIMING(1)));
	serial_debug("DDRC_TIMING2		0x%x\n", ddr_readl(DDRC_TIMING(2)));
	serial_debug("DDRC_TIMING3		0x%x\n", ddr_readl(DDRC_TIMING(3)));
	serial_debug("DDRC_TIMING4		0x%x\n", ddr_readl(DDRC_TIMING(4)));
	serial_debug("DDRC_TIMING5		0x%x\n", ddr_readl(DDRC_TIMING(5)));
	serial_debug("DDRC_TIMING6		0x%x\n", ddr_readl(DDRC_TIMING(6)));
	serial_debug("DDRC_REFCNT		0x%x\n", ddr_readl(DDRC_REFCNT));
	serial_debug("DDRC_MMAP0		0x%x\n", ddr_readl(DDRC_MMAP0));
	serial_debug("DDRC_MMAP1		0x%x\n", ddr_readl(DDRC_MMAP1));
	serial_debug("DDRC_REMAP1		0x%x\n", ddr_readl(DDRC_REMAP(1)));
	serial_debug("DDRC_REMAP2		0x%x\n", ddr_readl(DDRC_REMAP(2)));
	serial_debug("DDRC_REMAP3		0x%x\n", ddr_readl(DDRC_REMAP(3)));
	serial_debug("DDRC_REMAP4		0x%x\n", ddr_readl(DDRC_REMAP(4)));
	serial_debug("DDRC_REMAP5		0x%x\n", ddr_readl(DDRC_REMAP(5)));
	serial_debug("DDRC_CLKSTP_CFG		0x%x\n", ddr_readl(DDRC_CLKSTP_CFG));
	serial_debug("DDRC_AUTOSR_EN		0x%x\n", ddr_readl(DDRC_AUTOSR_EN));
}

static void dump_ddrp_register(void)
{
	serial_debug("DDRP_PIR		0x%x\n", ddr_readl(DDRP_PIR));
	serial_debug("DDRP_PGCR		0x%x\n", ddr_readl(DDRP_PGCR));
	serial_debug("DDRP_PGSR		0x%x\n", ddr_readl(DDRP_PGSR));
	serial_debug("DDRP_PTR0		0x%x\n", ddr_readl(DDRP_PTR0));
	serial_debug("DDRP_PTR1		0x%x\n", ddr_readl(DDRP_PTR1));
	serial_debug("DDRP_PTR2		0x%x\n", ddr_readl(DDRP_PTR2));
	serial_debug("DDRP_DCR		0x%x\n", ddr_readl(DDRP_DCR));
	serial_debug("DDRP_DTPR0		0x%x\n", ddr_readl(DDRP_DTPR0));
	serial_debug("DDRP_DTPR1		0x%x\n", ddr_readl(DDRP_DTPR1));
	serial_debug("DDRP_DTPR2		0x%x\n", ddr_readl(DDRP_DTPR2));
	serial_debug("DDRP_MR0		0x%x\n", ddr_readl(DDRP_MR0));
	serial_debug("DDRP_MR1		0x%x\n", ddr_readl(DDRP_MR1));
	serial_debug("DDRP_MR2		0x%x\n", ddr_readl(DDRP_MR2));
	serial_debug("DDRP_MR3		0x%x\n", ddr_readl(DDRP_MR3));
	serial_debug("DDRP_ODTCR		0x%x\n", ddr_readl(DDRP_ODTCR));
	serial_debug("DDRP_ZQXSR0		0x%x\n", ddr_readl(DDRP_ZQXSR0(0)));

	int i=0;
	for(i=0;i<4;i++) {
		serial_debug("DX%dGSR0: %x\n", i, ddr_readl(DDRP_DXGSR0(i)));
		serial_debug("@pas:DXDQSTR(%d)= 0x%x\n", i,ddr_readl(DDRP_DXDQSTR(i)));
	}
}
#else
#define FUNC_ENTER()
#define FUNC_EXIT()

#define dump_ddrc_register()
#define dump_ddrp_register()
#endif

static void mem_remap(void)
{
	int i;
	unsigned int *remap = remap_array;
	for(i = 0;i < ARRAY_SIZE(remap_array);i++)
	{
		ddr_writel(remap[i],DDRC_REMAP(i+1));
	}
}
void ddr_controller_init(int bypass,enum ddr_type type)
{
	FUNC_ENTER();
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
	ddr_writel(DDRC_CTRL_VALUE, DDRC_CTRL);
	mem_remap();
	debug("DDRC_STATUS: %x\n",ddr_readl(DDRC_STATUS));
	ddr_writel(ddr_readl(DDRC_STATUS) & ~DDRC_DSTATUS_MISS, DDRC_STATUS);

	if(DDRC_AUTOSR_EN_VALUE){
		if(!bypass) {
			ddr_writel(0, DDRC_DLP);
		} else {
			/* ddr_writel(0, DDRC_DLP); */
			ddr_writel((9 << 28) | 0xf,DDRC_CLKSTP_CFG);
		}
	}
	ddr_writel(DDRC_AUTOSR_EN_VALUE, DDRC_AUTOSR_EN);
	FUNC_EXIT();
}
static void wait_ddrp_pgsr(unsigned int wait_val,int timeout)
{
	while(((ddr_readl(DDRP_PGSR) & wait_val) != wait_val) && --timeout)
		debug("DDRP_PGSR:%x\n",ddr_readl(DDRP_PGSR));
	if (timeout == 0) {
		debug("DDR init timeout: PGSR=%X %x\n", ddr_readl(DDRP_PGSR),wait_val);
		ddr_hang();
	}
}
static int ddr_training_hardware(int bypass)
{
	int result = 0;
	int timeout = 500000;
	unsigned int wait_val = 0;
	unsigned int pir_val = DDRP_PIR_INIT | DDRP_PIR_QSTRN;
	FUNC_ENTER();
	debug("DTDR0-1: %x %x\n",ddr_readl(DDRP_DTDR0),ddr_readl(DDRP_DTDR1));
	wait_val = DDRP_PGSR_IDONE | DDRP_PGSR_DTDONE;
	if(bypass) {
		pir_val |= DDRP_PIR_DLLBYP | DDRP_PIR_LOCKBYP;
	}
	ddr_writel(pir_val, DDRP_PIR);
	wait_ddrp_pgsr(wait_val,timeout);
	result = ddr_readl(DDRP_PGSR);
	if (result & (DDRP_PGSR_DTERR | DDRP_PGSR_DTIERR)) {
		serial_debug("DDR hardware training error result= %x\n",result);
		dump_ddrp_register();
	} else
		result = 0;
	FUNC_EXIT();
	return result;
}
static enum ddr_type get_ddr_type(void)
{
	int type;
	ddrc_cfg_t ddrc_cfg;
	ddrc_cfg.d32 = DDRC_CFG_VALUE;

	switch(ddrc_cfg.b.TYPE){
	case 3:
		type = LPDDR;
		break;
	case 4:
		type = DDR2;
		break;
	case 5:
		type = LPDDR2;
		break;
	case 6:
		type = DDR3;
		break;
	case 7:
		type = LPDDR3;
		break;
	default:
		type = UNKOWN;
		debug("unsupport ddr type!\n");
		ddr_hang();
	}
	return type;
}
static void ddr_phy_param_config(int bypass,enum ddr_type type)
{
	FUNC_ENTER();
	ddr_writel(DDRP_DCR_VALUE, DDRP_DCR);
	//ddr_writel(DDRP_ODTCR_VALUE, DDRP_ODTCR);
	ddr_writel(DDRP_PTR0_VALUE, DDRP_PTR0);
	ddr_writel(DDRP_PTR1_VALUE, DDRP_PTR1);
	ddr_writel(DDRP_PTR2_VALUE, DDRP_PTR2);
	ddr_writel(DDRP_DTPR0_VALUE, DDRP_DTPR0);
	ddr_writel(DDRP_DTPR1_VALUE, DDRP_DTPR1);
	ddr_writel(DDRP_DTPR2_VALUE, DDRP_DTPR2);

	ddr_writel(DDRP_DX0GCR_VALUE,DDRP_DXGCR(0));
	ddr_writel(DDRP_DX1GCR_VALUE,DDRP_DXGCR(1));
	ddr_writel(DDRP_DX2GCR_VALUE,DDRP_DXGCR(2));
	ddr_writel(DDRP_DX3GCR_VALUE,DDRP_DXGCR(3));

	ddr_writel(DDRP_PGCR_VALUE, DDRP_PGCR);
	/***************************************************************
	 *  DXCCR:
	 *       DQSRES:  4...7bit  is DQSRES[].
	 *       DQSNRES: 8...11bit is DQSRES[] too.
	 *
	 *      Selects the on-die pull-down/pull-up resistor for DQS pins.
	 *      DQSRES[3]: selects pull-down (when set to 0) or pull-up (when set to 1).
	 *      DQSRES[2:0] selects the resistor value as follows:
	 *      000 = Open: On-die resistor disconnected
	 *      001 = 688 ohms
	 *      010 = 611 ohms
	 *      011 = 550 ohms
	 *      100 = 500 ohms
	 *      101 = 458 ohms
	 *      110 = 393 ohms
	 *      111 = 344 ohms
	 *****************************************************************
	 *      Note: DQS resistor must be connected for LPDDR/LPDDR2    *
	 *****************************************************************
	 *     the config will affect power and stablity
	 *
	 *     DXIOM: 1bit
	 *           Data I/O Mode: Selects SSTL mode (when set to 0)
	 *                           or CMOS mode (when set to 1)
	 *     I/O Mode: I/O Mode select
	 *           0 = DDR2/DDR3/DDR3L/LPDDR2 mode
	 *           1 = Mobile DDR mode
	 *
	 */
	switch(type){
	case LPDDR:
		ddr_writel(0x30c00813, DDRP_ACIOCR);
		ddr_writel(0x4912, DDRP_DXCCR);
		ddr_writel(DDRP_MR0_VALUE, DDRP_MR0);
		ddr_writel(DDRP_MR2_VALUE, DDRP_MR2);
		break;
	case DDR3:
		/* ddr_writel(0x800, DDRP_DXCCR); ??? */
		/* ddr_writel(0x910, DDRP_DXCCR); */
		/* ddr_writel(0xc40, DDRP_DXCCR); */
		ddr_writel(DDRP_MR0_VALUE, DDRP_MR0);
		ddr_writel(DDRP_MR1_VALUE, DDRP_MR1);
		ddr_writel(DDRP_MR2_VALUE, DDRP_MR2);
		break;
	case LPDDR2:
		ddr_writel(0x910, DDRP_DXCCR);
		ddr_writel(DDRP_MR3_VALUE, DDRP_MR3);
		ddr_writel(DDRP_MR1_VALUE, DDRP_MR1);
		ddr_writel(DDRP_MR2_VALUE, DDRP_MR2);
		break;
	case LPDDR3:
		ddr_writel(0x910, DDRP_DXCCR);
		ddr_writel(DDRP_MR3_VALUE, DDRP_MR3);
		ddr_writel(DDRP_MR1_VALUE, DDRP_MR1);
		ddr_writel(DDRP_MR2_VALUE, DDRP_MR2);
		break;
	case DDR2:
		ddr_writel(0x910, DDRP_DXCCR);
		ddr_writel(DDRP_MR0_VALUE, DDRP_MR0);
		ddr_writel(DDRP_MR1_VALUE, DDRP_MR1);
		break;
	default:
		ddr_hang();
	}
	FUNC_EXIT();
}
static void ddr_phy_impedance_calibration(unsigned int cal_value)
{
	unsigned int pull[4];
	unsigned int *out_imp = out_imp_table;
	unsigned int *odt_imp = odt_imp_table;
	unsigned char *rzq = rzq_table;

	unsigned int val = cal_value;
	int i,j;
	for(i = 0;i < 4;i++){
		pull[i] = (val >> (5 * i)) & 0x1f;
		for(j = 0;j < ARRAY_SIZE(rzq_table);j++)
			if(pull[i] == rzq[j])
				break;
		debug("cal pull %d index = %d\n",pull[i],j);
		pull[i] = j;

	}
	debug("out_imp %d %d,odt_imp %d %d\n",out_imp[0],out_imp[1],odt_imp[0],odt_imp[1]);
	pull[0] = (out_imp[0] * pull[0] + out_imp[1] / 2) / out_imp[1];
	pull[1] = (out_imp[0] * pull[1] + out_imp[1] / 2) / out_imp[1];

	pull[2] = (odt_imp[0] * pull[2] + odt_imp[1] / 2) / odt_imp[1];
	pull[3] = (odt_imp[0] * pull[3] + odt_imp[1] / 2) / odt_imp[1];

	val = ddr_readl(DDRP_ZQXCR0(0));
	val &= 0x10000000;
	for(i = 0;i < 4;i++){
		debug("set pull %d index = %d\n",rzq[pull[i]],pull[i]);
		val |= (rzq[pull[i]] << (5 * i));
	}
	val |= DDRP_ZQXCR_ZDEN;
	ddr_writel(val, DDRP_ZQXCR0(0));
}
static void ddr_phy_init_dram(int bypass,enum ddr_type type)
{
	int timeout = 10000;
	unsigned int pir_val;
	unsigned int val;
	unsigned int wait_val = 0;
	FUNC_ENTER();
	pir_val = DDRP_PIR_INIT | DDRP_PIR_DLLSRST | DDRP_PIR_ITMSRST | DDRP_PIR_DRAMINT | DDRP_PIR_DLLLOCK | DDRP_PIR_ZCAL;
	wait_val = DDRP_PGSR_IDONE | DDRP_PGSR_ZCDONE | DDRP_PGSR_DIDONE | DDRP_PGSR_DLDONE;

	if(type == LPDDR) {
		pir_val &= ~(DDRP_PIR_ZCAL);
		wait_val &= ~(DDRP_PGSR_ZCDONE);
	}
	if(type == DDR3)
		pir_val |= DDRP_PIR_DRAMRST;

	if(pir_val &  DDRP_PIR_ZCAL)
	{
		/* for ddr impedance calibration. */
		val = ddr_readl(DDRP_ZQXCR0(0));
		val &= ~((1 << 31) | (1 << 29) | (1 <<28) | 0xfffff);
		val |= (1 << 30);
		ddr_writel(val,DDRP_ZQXCR0(0));
		ddr_writel(DDRP_ZQNCR1_VALUE ,DDRP_ZQXCR1(0));
		debug("require impedamce is %x\n",DDRP_ZQNCR1_VALUE);

	}
	if(bypass) {
		pir_val &= ~DDRP_PIR_DLLLOCK;
		pir_val &= ~DDRP_PIR_DLLSRST;
		pir_val |= DDRP_PIR_DLLBYP | DDRP_PIR_LOCKBYP;
		// DLL Disable: only bypassmode
		ddr_writel(0x1 << 31, DDRP_ACDLLCR);
		// 200M bypass.
		val = ddr_readl(DDRP_DLLGCR);
		val |= 1 << 23;
		ddr_writel(val,DDRP_DLLGCR);

		/*  LPDLLPD:  only for ddr bypass mode
		 * Low Power DLL Power Down: Specifies if set that the PHY should respond to the *
		 * DFI low power opportunity request and power down the DLL of the PHY if the *
		 * wakeup time request satisfies the DLL lock time */
	 	val = ddr_readl(DDRP_DSGCR);
	 	val &= ~(1 << 4);
	 	ddr_writel(val,DDRP_DSGCR);
		wait_val &= ~DDRP_PGSR_DLDONE;
	}
	wait_ddrp_pgsr((DDRP_PGSR_IDONE | DDRP_PGSR_DLDONE | DDRP_PGSR_ZCDONE),0x10000);
	ddr_writel(pir_val, DDRP_PIR);
	wait_ddrp_pgsr(wait_val,timeout);
	if(pir_val & DDRP_PIR_ZCAL) {
		val = ddr_readl(DDRP_ZQXSR0(0));
		debug("calibrate: %x\n",val);
		if(val & 0x40000000)
		{
			debug("ddr_impedance_calibration error!");
			ddr_hang();
		}else{
			ddr_phy_impedance_calibration(val);
		}
	}
	FUNC_EXIT();
}
static struct jzsoc_ddr_hook *ddr_hook = NULL;
void register_ddr_hook(struct jzsoc_ddr_hook * hook)
{
	ddr_hook = hook;
}
static void force_no_slefresh(void)
{
	/* force CKE1 CS1 HIGH */
	ddr_writel(DDRC_CFG_VALUE | DDRC_CFG_CS1EN |  DDRC_CFG_CS0EN, DDRC_CFG);
	ddr_writel((1 << 1), DDRC_CTRL);
}
void ddr_phy_init(int bypass,enum ddr_type type)
{
	int ret;
	FUNC_ENTER();
	ddr_writel(0x150000, DDRP_DTAR);
	/* DDR training address set*/
	ddr_phy_param_config(bypass,type);
	ddr_phy_init_dram(bypass,type);

	if(ddr_hook && ddr_hook->reset_controller)
		ddr_hook->reset_controller(bypass,type);
	force_no_slefresh();
	ret = ddr_training_hardware(bypass);
	if(ret)
		ddr_hang();
	FUNC_EXIT();
}

extern void soc_ddr_init(void);
void sdram_init(void)
{
	enum ddr_type type;
	unsigned int bypass = 0;
	unsigned int rate;

	debug("sdram init start\n");
	soc_ddr_init();
	//dump_ddrc_register();
	type = get_ddr_type();
	clk_set_rate(DDR, gd->arch.gi->ddrfreq);
	rate = clk_get_rate(DDR);
	if(rate < 300000000)
		bypass = 1;
	if(ddr_hook && ddr_hook->prev_ddr_init)
		ddr_hook->prev_ddr_init(bypass,type);

	/* DDR PHY init*/
	ddr_phy_init(bypass,type);
	dump_ddrp_register();
	/* DDR Controller init*/
	ddr_controller_init(bypass,type);
	if(ddr_hook && ddr_hook->post_ddr_init)
		ddr_hook->post_ddr_init(bypass,type);
	debug("DDRC_DLP:%x\n",ddr_readl(DDRC_DLP));
	dump_ddrc_register();
	/* DDRC address remap configure*/
	debug("sdram init finished\n");

}
phys_size_t initdram(int board_type)
{
	/* SDRAM size was calculated when compiling. */
#ifndef EMC_LOW_SDRAM_SPACE_SIZE
#define EMC_LOW_SDRAM_SPACE_SIZE 0x10000000 /* 256M */
#endif /* EMC_LOW_SDRAM_SPACE_SIZE */

	unsigned int ram_size;
	ddrc_cfg_t ddrc_cfg = {.d32 = ddr_readl(DDRC_CFG)};
	uint16_t col0 = ddrc_cfg.b.COL0 + 8, row0 = ddrc_cfg.b.ROW0 + 12;
	uint8_t dw = ddrc_cfg.b.DW ? 4 : 2, ba0 = ddrc_cfg.b.BA0 ? 8 : 4;
	uint32_t chip_0_szie = (uint32_t)(1 << (col0 + row0)) * dw * ba0;
	ram_size = chip_0_szie + (unsigned int)(DDR_CHIP_1_SIZE);
	if (ram_size > EMC_LOW_SDRAM_SPACE_SIZE)
		ram_size = EMC_LOW_SDRAM_SPACE_SIZE;

	return ram_size;
}
