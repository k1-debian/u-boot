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

#define DEBUG
#include <config.h>
#include <common.h>
#include <ddr/ddr_common.h>
#include <generated/ddr_reg_values.h>

#include <asm/io.h>
#include <asm/arch/clk.h>
#define CONFIG_DWC_DEBUG 1
#include "ddr_debug.h"
#define ddr_hang() do{						\
		printf("%s %d\n",__FUNCTION__,__LINE__);	\
		hang();						\
	}while(0)

DECLARE_GLOBAL_DATA_PTR;

struct ddr_reg_value *global_reg_value __attribute__ ((section(".data")));

extern void ddrp_auto_calibration(void);

#ifdef  CONFIG_DWC_DEBUG
#define FUNC_ENTER() printf("%s enter.\n",__FUNCTION__);
#define FUNC_EXIT() printf("%s exit.\n",__FUNCTION__);

static void dump_ddrc_register(void)
{
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
	printf("DDRC_REFCNT         0x%x\n", ddr_readl(DDRC_REFCNT));
	printf("DDRC_AUTOSR_CNT     0x%x\n", ddr_readl(DDRC_AUTOSR_CNT));
	printf("DDRC_AUTOSR_EN      0x%x\n", ddr_readl(DDRC_AUTOSR_EN));
	printf("DDRC_MMAP0          0x%x\n", ddr_readl(DDRC_MMAP0));
	printf("DDRC_MMAP1          0x%x\n", ddr_readl(DDRC_MMAP1));
	printf("DDRC_REMAP1         0x%x\n", ddr_readl(DDRC_REMAP(1)));
	printf("DDRC_REMAP2         0x%x\n", ddr_readl(DDRC_REMAP(2)));
	printf("DDRC_REMAP3         0x%x\n", ddr_readl(DDRC_REMAP(3)));
	printf("DDRC_REMAP4         0x%x\n", ddr_readl(DDRC_REMAP(4)));
	printf("DDRC_REMAP5         0x%x\n", ddr_readl(DDRC_REMAP(5)));
	printf("DDRC_REMAP6         0x%x\n", ddr_readl(DDRC_REMAP(6)));
	printf("DDRC_DWCFG          0x%x\n", ddr_readl(DDRC_DWCFG));
	printf("DDRC_HREGPRO        0x%x\n", ddr_readl(DDRC_HREGPRO));
	printf("DDRC_PREGPRO        0x%x\n", ddr_readl(DDRC_PREGPRO));
/*
	printf("#define timing1_tWL         %d\n", timing1_tWL);
	printf("#define timing1_tWR         %d\n", timing1_tWR);
	printf("#define timing1_tWTR        %d\n", timing1_tWTR);
	printf("#define timing1_tWDLAT      %d\n", timing1_tWDLAT);

	printf("#define timing2_tRL         %d\n", timing2_tRL);
	printf("#define timing2_tRTP        %d\n", timing2_tRTP);
	printf("#define timing2_tRTW        %d\n", timing2_tRTW);
	printf("#define timing2_tRDLAT      %d\n", timing2_tRDLAT);

	printf("#define timing3_tRP         %d\n", timing3_tRP);
	printf("#define timing3_tCCD        %d\n", timing3_tCCD);
	printf("#define timing3_tRCD        %d\n", timing3_tRCD);
	printf("#define timing3_ttEXTRW     %d\n", timing3_ttEXTRW);

	printf("#define timing4_tRRD        %d\n", timing4_tRRD);
	printf("#define timing4_tRAS        %d\n", timing4_tRAS);
	printf("#define timing4_tRC         %d\n", timing4_tRC);
	printf("#define timing4_tFAW        %d\n", timing4_tFAW);

	printf("#define timing5_tCKE        %d\n", timing5_tCKE);
	printf("#define timing5_tXP         %d\n", timing5_tXP);
	printf("#define timing5_tCKSRE      %d\n", timing5_tCKSRE);
	printf("#define timing5_tCKESR      %d\n", timing5_tCKESR);
	printf("#define timing5_tXS         %d\n", timing5_tXS);
*/
}

static void dump_ddrp_register(void)
{
	printf("DDRP_INNOPHY_PHY_RST		0x%x\n", ddr_readl(DDRP_INNOPHY_PHY_RST));
	printf("DDRP_INNOPHY_MEM_CFG		0x%x\n", ddr_readl(DDRP_INNOPHY_MEM_CFG));
	printf("DDRP_INNOPHY_DQ_WIDTH		0x%x\n", ddr_readl(DDRP_INNOPHY_DQ_WIDTH));
	printf("DDRP_INNOPHY_CL			0x%x\n", ddr_readl(DDRP_INNOPHY_CL));
	printf("DDRP_INNOPHY_CWL		0x%x\n", ddr_readl(DDRP_INNOPHY_CWL));
	printf("DDRP_INNOPHY_PLL_FBDIV		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_FBDIV));
	printf("DDRP_INNOPHY_PLL_CTRL		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_CTRL));
	printf("DDRP_INNOPHY_PLL_PDIV		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_PDIV));
	printf("DDRP_INNOPHY_PLL_LOCK		0x%x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK));
	printf("DDRP_INNOPHY_TRAINING_CTRL	0x%x\n", ddr_readl(DDRP_INNOPHY_TRAINING_CTRL));
	printf("DDRP_INNOPHY_CALIB_DONE		0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
	printf("DDRP_INNOPHY_CALIB_DELAY_AL	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AL));
	printf("DDRP_INNOPHY_CALIB_DELAY_AH	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_DELAY_AH));
	printf("DDRP_INNOPHY_CALIB_BYPASS_AL	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL));
	printf("DDRP_INNOPHY_CALIB_BYPASS_AH	0x%x\n", ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH));
	printf("DDRP_INNOPHY_INIT_COMP		0x%x\n", ddr_readl(DDRP_INNOPHY_INIT_COMP));
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
	unsigned int *remap;
	remap = global_reg_value->REMMAP_ARRAY;

	for(i = 0;i < ARRAY_SIZE(global_reg_value->REMMAP_ARRAY);i++)
	{
		ddr_writel(remap[i], DDRC_REMAP(i+1));
	}
}
#if 0
void ddr_controller_init(enum ddr_type type)
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

	/* DDRC memory map configure*/
	ddr_writel(DDRC_MMAP0_VALUE, DDRC_MMAP0);
	ddr_writel(DDRC_MMAP1_VALUE, DDRC_MMAP1);
	ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL);
	ddr_writel(DDRC_REFCNT_VALUE, DDRC_REFCNT);
	ddr_writel(DDRC_CTRL_VALUE, DDRC_CTRL);
	//mem_remap();
	debug("DDRC_STATUS: %x\n",ddr_readl(DDRC_STATUS));

	if(DDRC_AUTOSR_CNT_VALUE) {
		ddr_writel(DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
		ddr_writel(1, DDRC_AUTOSR_EN);
	} else {
		ddr_writel(0, DDRC_AUTOSR_EN);
	}
	FUNC_EXIT();
}
#endif
static enum ddr_type get_ddr_type(void)
{
	int type;

	type = global_reg_value->h.type;
	switch(global_reg_value->h.type){

		case DDR3:
			printf("DDR: %s type is : DDR3\n", global_reg_value->h.name);
			break;
		case LPDDR:
			printf("DDR: %s type is : LPDDR\n", global_reg_value->h.name);
			break;
		case LPDDR2:
			printf("DDR: %s type is : LPDDR2\n", global_reg_value->h.name);
			break;
		case LPDDR3:
			printf("DDR: %s type is : LPDDR3\n", global_reg_value->h.name);
			break;
		case DDR2:
			printf("DDR: %s type is : DDR2\n", global_reg_value->h.name);
			break;
		default:
			type = UNKOWN;
			printf("unsupport ddr type!\n");
			ddr_hang();
	}

	return type;
}

static void controller_reset_phy(void)
{
	FUNC_ENTER();
	ddr_writel(0xf << 20, DDRC_CTRL);
	mdelay(1);
	ddr_writel(0x8 << 20, DDRC_CTRL);  //dfi_reset_n low for innophy
	mdelay(1);
	FUNC_EXIT();
}


static struct jzsoc_ddr_hook *ddr_hook = NULL;
void register_ddr_hook(struct jzsoc_ddr_hook * hook)
{
	ddr_hook = hook;
}

static void ddrp_pll_init(void)
{
	ddr_writel(0x0, DDRP_INNOPHY_PLL_FBDIV);
	ddr_writel(0x6, DDRP_INNOPHY_PLL_FBDIV_H);
	ddr_writel(0x41, DDRP_INNOPHY_PLL_PDIV);
	ddr_writel(0x0, DDRP_INNOPHY_PLL_CTRL);
	udelay(500);
}

static void ddrp_cfg(struct ddr_reg_value *global_reg_value)
{
	unsigned int val;

	ddr_writel(0, DDRP_INNOPHY_DQ_WIDTH_H);
	ddr_writel(DDRP_DQ_WIDTH_DQ_H | DDRP_DQ_WIDTH_DQ_L, DDRP_INNOPHY_DQ_WIDTH);
	ddr_writel(global_reg_value->DDRP_MEMCFG_VALUE, DDRP_INNOPHY_MEM_CFG);

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

}

/*
 * Name     : ddrp_calibration()
 * Function : control the RX DQS window delay to the DQS
 *
 * a_low_8bit_delay	= al8_2x * clk_2x + al8_1x * clk_1x;
 * a_high_8bit_delay	= ah8_2x * clk_2x + ah8_1x * clk_1x;
 *
 * */
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
	unsigned int addr = 0xa0000000, val;
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

void ddr_phy_init(enum ddr_type type)
{
	FUNC_ENTER();
	ddrp_pll_init();
	ddrp_cfg(global_reg_value);
	FUNC_EXIT();
}

void ddrc_dfi_init(enum ddr_type type)
{
	unsigned int tmp;
	FUNC_ENTER();

	tmp = ddr_readl(DDRC_DWCFG);
	tmp &= ~(1 << 3);
	tmp |= (1 << 4);
	ddr_writel(tmp, DDRC_DWCFG); // set dfi_init_start low, and buswidth 16bit
	while(!(ddr_readl(DDRC_DWSTATUS) & DDRC_DWSTATUS_DFI_INIT_COMP)); //polling dfi_init_complete

	tmp = ddr_readl(DDRC_CTRL);
	tmp |= (1 << 23);
	ddr_writel(tmp, DDRC_CTRL); //set dfi_reset_n high

	udelay(500);

	ddr_writel(global_reg_value->DDRC_CFG_VALUE, DDRC_CFG);

	ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); // set CKE to high

	switch(type) {
	case LPDDR2:
#define DDRC_LMR_MR(n)										\
		global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |	\
			((global_reg_value->DDR_MR##n##_VALUE & 0xff) << 24) |						\
			(((global_reg_value->DDR_MR##n##_VALUE >> 8) & 0xff) << (16))
		ddr_writel(DDRC_LMR_MR(63), DDRC_LMR); //set MRS reset
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(10), DDRC_LMR); //set IO calibration
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //set MR1
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //set MR2
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //set MR3
		mdelay(1);
#undef DDRC_LMR_MR
		break;
	case DDR3:
		udelay(200);
#define DDRC_LMR_MR(n)								\
		global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR | 2 |	\
		((global_reg_value->DDR_MR##n##_VALUE & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |	\
		(((global_reg_value->DDR_MR##n##_VALUE >> 16) & 0x7) << DDRC_LMR_BA_BIT)	|	\
		(((global_reg_value->DDR_MR##n##_VALUE >> 16) & 0x7) << 28)

		ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR0
		mdelay(5);
		ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR1
		mdelay(5);
		ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //MR2
		mdelay(5);
		ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR3
		mdelay(5);
		ddr_writel(global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_ZQCL_CS0, DDRC_LMR); //ZQCL
		mdelay(10);
#undef DDRC_LMR_MR
		break;
	case LPDDR3:
#define DDRC_LMR_MR(n)                                                         \
        global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |		\
		((global_reg_value->DDR_MR##n##_VALUE & 0xff) << 24)  |                           \
		(((global_reg_value->DDR_MR##n##_VALUE >> 8) & 0xff) << (16))
		ddr_writel(DDRC_LMR_MR(63), DDRC_LMR); //set MRS reset
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(10), DDRC_LMR); //set IO calibration
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //set MR1
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //set MR2
		mdelay(1);
		ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //set MR3
		mdelay(1);
		/* ddr_writel(DDRC_LMR_MR(11), DDRC_LMR); //set MR11 */
		/* mdelay(1); */
#undef DDRC_LMR_MR
		break;
case DDR2:
#define DDRC_LMR_MR(n)											\
		global_reg_value->DDRC_DLMR_VALUE | 1 << 1 | DDRC_LMR_START | DDRC_LMR_CMD_LMR |	\
			((global_reg_value->DDR_MR##n##_VALUE & 0x1fff) << DDRC_LMR_DDR_ADDR_BIT) |		\
			(((global_reg_value->DDR_MR##n##_VALUE >> 13) & 0x3) << DDRC_LMR_BA_BIT)

		while (ddr_readl(DDRC_LMR) & (1 << 0));
		udelay(100);
		ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR2
		udelay(5);
		ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR3
		udelay(5);
		ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //MR1
		udelay(5);
		ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR0
		udelay(5 * 1000);
#undef DDRC_LMR_MR
		break;
	default:
		ddr_hang();
	}
	FUNC_EXIT();
}

static void ddrc_prev_init(void)
{
	FUNC_ENTER();
	/* DDRC CFG init*/
	/* /\* DDRC CFG init*\/ */
	/* ddr_writel(DDRC_CFG_VALUE, DDRC_CFG); */
	/* DDRC timing init*/
	ddr_writel(global_reg_value->DDRC_TIMING1_VALUE, DDRC_TIMING(1));
	ddr_writel(global_reg_value->DDRC_TIMING2_VALUE, DDRC_TIMING(2));
	ddr_writel(global_reg_value->DDRC_TIMING3_VALUE, DDRC_TIMING(3));
	ddr_writel(global_reg_value->DDRC_TIMING4_VALUE, DDRC_TIMING(4));
	ddr_writel(global_reg_value->DDRC_TIMING5_VALUE, DDRC_TIMING(5));

	/* DDRC memory map configure*/
	ddr_writel(global_reg_value->DDRC_MMAP0_VALUE, DDRC_MMAP0);
	ddr_writel(global_reg_value->DDRC_MMAP1_VALUE, DDRC_MMAP1);

	ddr_writel(global_reg_value->DDRC_CTRL_VALUE & ~(7 << 12), DDRC_CTRL);
	ddr_writel(global_reg_value->DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
	ddr_writel(global_reg_value->DDRC_REFCNT_VALUE, DDRC_REFCNT);

	FUNC_EXIT();
}

static void ddrc_post_init(void)
{
	FUNC_ENTER();

	mem_remap();
	debug("DDRC_STATUS: %x\n",ddr_readl(DDRC_STATUS));
	ddr_writel(global_reg_value->DDRC_CTRL_VALUE, DDRC_CTRL);

	ddr_writel(global_reg_value->DDRC_CGUC0_VALUE, DDRC_CGUC0);
	ddr_writel(global_reg_value->DDRC_CGUC1_VALUE, DDRC_CGUC1);

	if(global_reg_value->DDRC_AUTOSR_CNT_VALUE) {
		ddr_writel(1, DDRC_AUTOSR_EN);
	} else {
		ddr_writel(0, DDRC_AUTOSR_EN);
	}

	FUNC_EXIT();
}

void get_ddr_params_normal(void)
{
	int found = 0;
	int size = 0;
	int i;
	unsigned int burned_ddr_id = *(volatile unsigned int *)(CONFIG_SPL_TEXT_BASE + 128);

	if((burned_ddr_id & 0xffff) != (burned_ddr_id >> 16)) {
		printf("invalid burned ddr id\n");
	}

	burned_ddr_id &= 0xffff;

	size = ARRAY_SIZE(supported_ddr_reg_values);

	if(size == 1) {
		found = 1;
		global_reg_value = &supported_ddr_reg_values[0];
	} else {
		for(i = 0; i < ARRAY_SIZE(supported_ddr_reg_values); i++) {
			global_reg_value = &supported_ddr_reg_values[i];
			if(burned_ddr_id == global_reg_value->h.id) {
				found = 1;
				break;
			}
		}
	}
}


void get_ddr_params(void)
{
	get_ddr_params_normal();
	//dump_generated_reg(global_reg_value);
}

void sdram_init(void)
{
	enum ddr_type type;
	unsigned int rate;
	unsigned int tmp;

	debug("sdram init start\n");
	soc_ddr_init();
	get_ddr_params();
	type = get_ddr_type();
	clk_set_rate(DDR, global_reg_value->h.freq);
	if(ddr_hook && ddr_hook->prev_ddr_init)
		ddr_hook->prev_ddr_init(type);
	rate = clk_get_rate(DDR);
	debug("DDR clk rate %x\n", rate);

//	controller_reset_phy();

	ddr_writel(1 << 20, DDRC_CTRL);

	/* DDR PHY init*/
//	ddr_phy_init(type);

	ddrp_cfg(global_reg_value);

	tmp = ddr_readl(DDRC_CTRL);
	tmp &= ~ (1 << 20);
	ddr_writel(tmp, DDRC_CTRL);

	ddrp_pll_init();

	ddrc_prev_init();

	ddrc_dfi_init(type);

	ddrp_software_calibration();

	if(ddr_hook && ddr_hook->post_ddr_init)
		ddr_hook->post_ddr_init(type);

	ddrc_post_init();

	dump_ddrc_register();


	printf("DDR size is : %d MByte\n", (global_reg_value->DDR_CHIP_0_SIZE + global_reg_value->DDR_CHIP_1_SIZE) / 1024 /1024);
	debug("sdram init finished\n");
}

phys_size_t initdram(int board_type)
{
	/* SDRAM size was calculated when compiling. */
#ifndef EMC_LOW_SDRAM_SPACE_SIZE
#define EMC_LOW_SDRAM_SPACE_SIZE 0x10000000 /* 256M */
#endif /* EMC_LOW_SDRAM_SPACE_SIZE */

	unsigned int ram_size;
	get_ddr_params();
	ram_size = (unsigned int)(global_reg_value->DDR_CHIP_0_SIZE) + (unsigned int)(global_reg_value->DDR_CHIP_1_SIZE);
	debug("ram_size=%x\n", ram_size);

	if (ram_size > EMC_LOW_SDRAM_SPACE_SIZE)
		ram_size = EMC_LOW_SDRAM_SPACE_SIZE;

	return ram_size;
}
