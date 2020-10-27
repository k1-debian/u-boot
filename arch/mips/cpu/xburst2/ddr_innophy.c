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
#include <generated/ddr_reg_values.h>
#endif

#include <asm/io.h>
#include <asm/arch/clk.h>

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

static enum ddr_type get_ddr_type(void)
{
	int type;
	ddrc_cfg_t ddrc_cfg;
	ddrc_cfg.d32 = global_reg_value->DDRC_CFG_VALUE;
	switch(ddrc_cfg.b.TYPE){
	case 3:
		type = LPDDR;
		debug("DDR type is : LPDDR\n");
		break;
	case 4:
		type = DDR2;
		debug("DDR type is : DDR2\n");
		break;
	case 5:
		type = LPDDR2;
		debug("DDR type is : LPDDR2\n");
		break;
	case 6:
		type = DDR3;
		debug("DDR type is : DDR3\n");
		break;
	case 7:
		type = LPDDR3;
		debug("DDR type is : LPDDR3\n");
		break;
	default:
		type = UNKOWN;
		debug("unsupport ddr type!\n");
		ddr_hang();
	}
	return type;
}

#if 0
static void ddrc_reset_phy(void)
{
	FUNC_ENTER();
	ddr_writel(0xf << 20, DDRC_CTRL);
	mdelay(1);
	ddr_writel(0x8 << 20, DDRC_CTRL);  //dfi_reset_n low for innophy
	mdelay(1);
	FUNC_EXIT();
}
#endif

static struct jzsoc_ddr_hook *ddr_hook = NULL;
void register_ddr_hook(struct jzsoc_ddr_hook * hook)
{
	ddr_hook = hook;
}

static void ddrp_pll_init(void)
{
	unsigned int val;

	val = ddr_readl(DDRP_INNOPHY_PLL_FBDIV);
	val &= ~(0xff);
	val |= 0x8;
	ddr_writel(val, DDRP_INNOPHY_PLL_FBDIV);

	val = ddr_readl(DDRP_INNOPHY_PLL_PDIV);
	val &= ~(0xff);
	val |= 4;
	ddr_writel(val, DDRP_INNOPHY_PLL_PDIV);

	ddr_writel(ddr_readl(DDRP_INNOPHY_PLL_CTRL) | DDRP_PLL_CTRL_PLLPDEN, DDRP_INNOPHY_PLL_CTRL);
	debug("DDRP_INNOPHY_PLL_LOCK: %x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK));
	ddr_writel(ddr_readl(DDRP_INNOPHY_PLL_CTRL) & ~DDRP_PLL_CTRL_PLLPDEN, DDRP_INNOPHY_PLL_CTRL);
	debug("DDRP_INNOPHY_PLL_LOCK: %x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK));

	while(!(ddr_readl(DDRP_INNOPHY_PLL_LOCK) & (1 << 3)));
	debug("DDRP_INNOPHY_PLL_LOCK: %x\n", ddr_readl(DDRP_INNOPHY_PLL_LOCK));

}

static void ddrp_cfg(void)
{
	unsigned int val;
#ifdef DEBUG_READ_WRITE
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
static void ddrp_auto_calibration(void)
{
	unsigned int reg_val = ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	unsigned int timeout = 0xffffff;
	unsigned int wait_cal_done = DDRP_CALIB_DONE_HDQCFA | DDRP_CALIB_DONE_LDQCFA;

	reg_val &= ~(DDRP_TRAINING_CTRL_DSCSE_BP);
	reg_val |= DDRP_TRAINING_CTRL_DSACE_START;
	ddr_writel(reg_val, DDRP_INNOPHY_TRAINING_CTRL);

	while(!((ddr_readl(DDRP_INNOPHY_CALIB_DONE) & 0x2) == 2) && --timeout) {

		udelay(1);
		printf("DDRP_INNOPHY_CALIB_DELAY_AL:%x\n", ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AL));
		printf("DDRP_INNOPHY_CALIB_DELAY_AH:%x\n", ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AH));
		printf("-----ddr_readl(DDRP_INNOPHY_CALIB_DONE): %x\n", ddr_readl(DDRP_INNOPHY_CALIB_DONE));
	}

	if(!timeout) {
		debug("ddrp_auto_calibration failed!\n");
	}
	ddr_writel(0, DDRP_INNOPHY_TRAINING_CTRL);
	debug("ddrp_auto_calibration success!\n");
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


#ifdef CONFIG_DDRP_SOFTWARE_TRAINING
static void ddrp_software_calibration(void)
{

	int x, y, z;
	int c, o, d =0, r = 0;
	unsigned int addr = 0xa0100000, val;
	unsigned int i, n, m = 0;
	struct ddrp_calib calib_val[8 * 2 * 8 * 5];

	unsigned int reg_val = ddr_readl(DDRP_INNOPHY_TRAINING_CTRL);
	unsigned int timeout = 0xffffff;
	unsigned int wait_cal_done = DDRP_CALIB_DONE_HDQCFA | DDRP_CALIB_DONE_LDQCFA;

	reg_val |= (DDRP_TRAINING_CTRL_DSCSE_BP);
	reg_val &= ~DDRP_TRAINING_CTRL_DSACE_START;
	ddr_writel(reg_val, DDRP_INNOPHY_TRAINING_CTRL);
	debug("-----DDRP_INNOPHY_TRAINING_CTRL: %x\n", ddr_readl(DDRP_INNOPHY_TRAINING_CTRL));
	debug("before trainning %x %x %x %x\n",ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL),
ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH),ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AL),ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AH));


	for(c = 0; c < 8; c ++) {
		for(o = 0; o < 2; o++) {
			for(d = 0; d < 8; d++)
			{
				x = c << 4 | o << 3 | d;
				y = c << 4 | o << 3 | d;
				unsigned int val1 = ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL) & (~0x7f);
				ddr_writel(x | val1, DDRP_INNOPHY_CALIB_BYPASS_AL);
				unsigned int val2 = ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH) & (~0x7f);
				ddr_writel(y | val2, DDRP_INNOPHY_CALIB_BYPASS_AH);

				for(r = 0; r < 4; r++) {
					unsigned int val1,val2;

#if 0
					val1 = ddr_readl(DDRP_INNOPHY_FPGA_RXDLL_DELAY) & (~(0x7 << 4)) & ((~(0x7 << 0))) ;
					ddr_writel(r << 0 | r << 4 | val1, DDRP_INNOPHY_FPGA_RXDLL_DELAY);
#else
					val1 = ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AL) & (~0x3);
					ddr_writel((r << 0) | val1, DDRP_INNOPHY_RXDLL_DELAY_AL);
					val1 = ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AH) & (~0x3);
					ddr_writel((r << 0)|val1, DDRP_INNOPHY_RXDLL_DELAY_AH);

#endif

					for(i = 0; i < 256; i++) {
						val = 0;
						for(n = 0; n < 4; n++ ) {
							val |= i<<(n * 8);
						}
						*(volatile unsigned int *)(addr + i * 4) = val;
						volatile unsigned int val1;
						val1 = *(volatile unsigned int *)(addr + i * 4);
						printf("c: %d, o: %d, d: %d, r: %d val1: %x, val: %x\n", c, o, d, r, val1, val);
#if 1
						if(val1 != val)
							break;
#endif
					}
					if(i == 256) {
						calib_val[m].bypass.b.cyclesel = c;
						calib_val[m].bypass.b.ophsel = o;
						calib_val[m].bypass.b.dllsel = d;
						calib_val[m].rx_dll.b.rx_dll = r;
						m++;
					}
				}
			}
		}
	}

	if(!m) {
		debug("calib bypass fail\n");
		return ;
	}

	debug("total trainning pass params: %d\n", m);
#ifdef DDR_CHOOSE_PARAMS
	m = choose_params(m);
#else
	m = m  / 2;
#endif


	c = calib_val[m].bypass.b.cyclesel;
	o = calib_val[m].bypass.b.ophsel;
	d = calib_val[m].bypass.b.dllsel;
	r = calib_val[m].rx_dll.b.rx_dll;

	x = c << 4 | o << 3 | d;
	y = c << 4 | o << 3 | d;
	z = r << 4 | r << 0;
	ddr_writel(x, DDRP_INNOPHY_CALIB_BYPASS_AL);
	ddr_writel(y, DDRP_INNOPHY_CALIB_BYPASS_AH);

#if 0
	unsigned int rxdll = ddr_readl(DDRP_INNOPHY_FPGA_RXDLL_DELAY) & (~(0x7 << 4)) & ((~(0x7 << 0))) ;
	ddr_writel(z | rxdll, DDRP_INNOPHY_FPGA_RXDLL_DELAY);
#else
	//unsigned int rxdll = ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AL) & 0x3;
	//rxdll = ddr_readl(DDRP_INNOPHY_RXDLL_DELAY_AH) & 0x3;
	ddr_writel(r << 0, DDRP_INNOPHY_RXDLL_DELAY_AL);
	ddr_writel(r << 0, DDRP_INNOPHY_RXDLL_DELAY_AH);
#endif

	{
		struct ddrp_calib b_al, b_ah, r_al,r_ah;
		b_al.bypass.u8 = ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AL);
		debug("bypass :CALIB_AL: dllsel %x, ophsel %x, cyclesel %x\n",b_al.bypass.b.dllsel, b_al.bypass.b.ophsel, b_al.bypass.b.cyclesel);
		b_ah.bypass.u8 = ddr_readl(DDRP_INNOPHY_CALIB_BYPASS_AH);
		debug("bypass:CAHIB_AH: dllsel %x, ophsel %x, cyclesel %x\n", b_ah.bypass.b.dllsel, b_ah.bypass.b.ophsel, b_ah.bypass.b.cyclesel);
		//debug("fpga rxdll delay %x\n", ddr_readl(DDRP_INNOPHY_FPGA_RXDLL_DELAY));

	}
}
#endif

void ddr_phy_init(enum ddr_type type)
{
	FUNC_ENTER();
	ddrp_pll_init();
	ddrp_cfg();
	FUNC_EXIT();
}

void ddrc_dfi_init(enum ddr_type type)
{
	unsigned int reg_val;
	FUNC_ENTER();

	reg_val = ddr_readl(DDRC_DWCFG);
	reg_val &= ~(1 << 3);
	ddr_writel(reg_val, DDRC_DWCFG); // set dfi_init_start low, and buswidth 16bit
	while(!(ddr_readl(DDRC_DWSTATUS) & DDRC_DWSTATUS_DFI_INIT_COMP)); //polling dfi_init_complete

	reg_val = ddr_readl(DDRC_CTRL);
	reg_val |= (1 << 23);
	ddr_writel(reg_val, DDRC_CTRL); //set dfi_reset_n high

	ddr_writel(global_reg_value->DDRC_CFG_VALUE, DDRC_CFG);
	ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); // set CKE to high

//	type = LPDDR3;

	switch(type) {
	case LPDDR2:
#if 0
#define DDRC_LMR_MR(n)										\
		DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |	\
			((DDR_MR##n##_VALUE & 0xff) << 24) |						\
			(((DDR_MR##n##_VALUE >> 8) & 0xff) << (16))
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
#endif
#if 0
	case DDR3:
#define DDRC_LMR_MR(n)								\
		DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |		\
		((DDR_MR##n##_VALUE & 0xffff) << DDRC_LMR_DDR_ADDR_BIT) |	\
		(((DDR_MR##n##_VALUE >> 16) & 0x7) << DDRC_LMR_BA_BIT)

		ddr_writel(DDRC_LMR_MR(0), DDRC_LMR); //MR0
		ddr_writel(DDRC_LMR_MR(1), DDRC_LMR); //MR1
		ddr_writel(DDRC_LMR_MR(2), DDRC_LMR); //MR2
		ddr_writel(DDRC_LMR_MR(3), DDRC_LMR); //MR3
		ddr_writel(DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_ZQCL_CS0, DDRC_LMR); //ZQCL
#undef DDRC_LMR_MR
		break;
#endif
	case LPDDR3:
#define DDRC_LMR_MR(n)                                                          \
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
		ddr_writel(DDRC_LMR_MR(11), DDRC_LMR); //set MR11
		mdelay(1);

#ifdef CONFIG_LPDDR2_W97BV6MKX

#define DDRC_LMR_MRW(v)                                                          \
                global_reg_value->DDRC_DLMR_VALUE | DDRC_LMR_START | DDRC_LMR_CMD_LMR |		 \
		((v & 0xff) << 24)  |                           		 \
		(((v >> 8) & 0xff) << (16))

		if(global_reg_value->DDRC_AUTOSR_EN_VALUE) {
			ddr_writel(DDRC_LMR_MRW(0x092a), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0915), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0900), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x093c), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0900), DDRC_LMR);
			mdelay(1);

			ddr_writel(DDRC_LMR_MRW(0x092a), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0915), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0951), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x093c), DDRC_LMR);
			mdelay(1);

			ddr_writel(DDRC_LMR_MRW(0x092a), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0915), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0917), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x093c), DDRC_LMR);
			mdelay(1);

			ddr_writel(DDRC_LMR_MRW(0x092a), DDRC_LMR);
			mdelay(1);
			ddr_writel(DDRC_LMR_MRW(0x0915), DDRC_LMR);
			mdelay(1);

			// test mode pattern1: ok? on board1, on board x2
			ddr_writel(DDRC_LMR_MRW(0x0907), DDRC_LMR);
			mdelay(1);
			//printf("test pattern1 0x0907\n");

			// test mode pattern2
			//ddr_writel(DDRC_LMR_MRW(0x0945), DDRC_LMR);
			//mdelay(1);
			//printf("test pattern2 0x0945\n");
			// test mode pattern3
			//ddr_writel(DDRC_LMR_MRW(0x09F7), DDRC_LMR);
			//mdelay(1);
			//printf("test pattern3 0x09F7\n");

			ddr_writel(DDRC_LMR_MRW(0x093c), DDRC_LMR);
			mdelay(1);
		}
#endif


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

	/* ddr_writel(DDRC_CTRL_CKE, DDRC_CTRL); */
	ddr_writel(global_reg_value->DDRC_CTRL_VALUE & ~(7 << 12), DDRC_CTRL);

	FUNC_EXIT();
}

static void ddrc_post_init(void)
{
	FUNC_ENTER();

	ddr_writel(global_reg_value->DDRC_REFCNT_VALUE, DDRC_REFCNT);
	mem_remap();
	debug("DDRC_STATUS: %x\n",ddr_readl(DDRC_STATUS));
	ddr_writel(global_reg_value->DDRC_CTRL_VALUE, DDRC_CTRL);

	ddr_writel(global_reg_value->DDRC_CGUC0_VALUE, DDRC_CGUC0);
	ddr_writel(global_reg_value->DDRC_CGUC1_VALUE, DDRC_CGUC1);

	FUNC_EXIT();
}

void dump_generated_reg(struct ddr_reg_value *reg)
{
	int i;
	printf("id		      = %x\n", reg->h.id);
	printf("type		      = %x\n", reg->h.type);
	printf("freq		      = %x\n", reg->h.freq);
	printf("DDRC_CFG_VALUE        = %x\n", reg->DDRC_CFG_VALUE);
	printf("DDRC_CTRL_VALUE       = %x\n", reg->DDRC_CTRL_VALUE);
	printf("DDRC_DLMR_VALUE       = %x\n", reg->DDRC_DLMR_VALUE);
	printf("DDRC_DDLP_VALUE       = %x\n", reg->DDRC_DDLP_VALUE);
	printf("DDRC_MMAP0_VALUE      = %x\n", reg->DDRC_MMAP0_VALUE);
	printf("DDRC_MMAP1_VALUE      = %x\n", reg->DDRC_MMAP1_VALUE);
	printf("DDRC_REFCNT_VALUE     = %x\n", reg->DDRC_REFCNT_VALUE);
	printf("DDRC_TIMING1_VALUE    = %x\n", reg->DDRC_TIMING1_VALUE);
	printf("DDRC_TIMING2_VALUE    = %x\n", reg->DDRC_TIMING2_VALUE);
	printf("DDRC_TIMING3_VALUE    = %x\n", reg->DDRC_TIMING3_VALUE);
	printf("DDRC_TIMING4_VALUE    = %x\n", reg->DDRC_TIMING4_VALUE);
	printf("DDRC_TIMING5_VALUE    = %x\n", reg->DDRC_TIMING5_VALUE);
	printf("DDRC_AUTOSR_CNT_VALUE = %x\n", reg->DDRC_AUTOSR_CNT_VALUE);
	printf("DDRC_AUTOSR_EN_VALUE  = %x\n", reg->DDRC_AUTOSR_EN_VALUE);
	printf("DDRC_HREGPRO_VALUE    = %x\n", reg->DDRC_HREGPRO_VALUE);
	printf("DDRC_PREGPRO_VALUE    = %x\n", reg->DDRC_PREGPRO_VALUE);
	printf("DDRC_CGUC0_VALUE      = %x\n", reg->DDRC_CGUC0_VALUE);
	printf("DDRC_CGUC1_VALUE      = %x\n", reg->DDRC_CGUC1_VALUE);
	printf("DDRP_MEMCFG_VALUE     = %x\n", reg->DDRP_MEMCFG_VALUE);
	printf("DDRP_CL_VALUE         = %x\n", reg->DDRP_CL_VALUE);
	printf("DDRP_CWL_VALUE        = %x\n", reg->DDRP_CWL_VALUE);
	printf("DDR_MR0_VALUE         = %x\n", reg->DDR_MR0_VALUE);
	printf("DDR_MR1_VALUE         = %x\n", reg->DDR_MR1_VALUE);
	printf("DDR_MR2_VALUE         = %x\n", reg->DDR_MR2_VALUE);
	printf("DDR_MR3_VALUE         = %x\n", reg->DDR_MR3_VALUE);
	printf("DDR_MR10_VALUE        = %x\n", reg->DDR_MR10_VALUE);
	printf("DDR_MR11_VALUE        = %x\n", reg->DDR_MR11_VALUE);
	printf("DDR_MR63_VALUE        = %x\n", reg->DDR_MR63_VALUE);
	printf("DDR_CHIP_0_SIZE       = %x\n", reg->DDR_CHIP_0_SIZE);
	printf("DDR_CHIP_1_SIZE       = %x\n", reg->DDR_CHIP_1_SIZE);
	for(i = 0; i < 5; i++) {
		printf("REMMAP_ARRAY[%d] = %x\n", i, reg->REMMAP_ARRAY[i]);
	}

}

#ifndef CONFIG_BURNER
void get_ddr_params_normal(void)
{
	int found = 0;
	int size = 0;
	int i;
	unsigned int burned_ddr_id = *(volatile unsigned int *)(0xb2401000 + 128);

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

	if(found) {
		printf("found ddr params with id: %x\n", burned_ddr_id);
	} else {
		printf("cound not found ddr params\n");
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
	get_ddr_params_normal();
#else
	get_ddr_params_burner();
#endif
//	dump_generated_reg(global_reg_value);

}

unsigned int get_ddr_size(void)
{
	return (global_reg_value->DDR_CHIP_0_SIZE) >> 20;
}

void sdram_init(void)
{
	enum ddr_type type;
	unsigned int rate;
	unsigned int reg_val;

	debug("sdram init start\n");
	soc_ddr_init();
	get_ddr_params();
	type = get_ddr_type();
	clk_set_rate(DDR, global_reg_value->h.freq);

	if(ddr_hook && ddr_hook->prev_ddr_init)
		ddr_hook->prev_ddr_init(type);
	rate = clk_get_rate(DDR);
	debug("DDR clk rate %d\n", rate);

//	ddrc_reset_phy();


	ddr_writel(1 << 20, DDRC_CTRL);  /* ddrc_reset_phy */

	/* DDR PHY init*/
//	ddr_phy_init(type);
	ddrp_cfg();

	reg_val = ddr_readl(DDRC_CTRL);
	reg_val &= ~ (1 << 20);
	ddr_writel(reg_val, DDRC_CTRL); /*ddrc_reset_phy clear*/

	ddrp_pll_init();
	ddrc_dfi_init(type);

	dump_ddrp_register();
	/* DDR Controller init*/
	ddrc_prev_init();

	ddr_writel(global_reg_value->DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT);
	ddrc_post_init();

#ifdef CONFIG_DDRP_SOFTWARE_TRAINING
	ddrp_software_calibration();
#else
	ddrp_auto_calibration();
#endif
	dump_ddrp_register();
	if(ddr_hook && ddr_hook->post_ddr_init)
		ddr_hook->post_ddr_init(type);

//	get_dynamic_calib_value(rate);/*reserved*/

	if(global_reg_value->DDRC_AUTOSR_EN_VALUE) {
		/* ddr_writel(DDRC_AUTOSR_CNT_VALUE, DDRC_AUTOSR_CNT); */
		ddr_writel(1, DDRC_AUTOSR_EN);
	} else {
		ddr_writel(0, DDRC_AUTOSR_EN);
	}
	dump_ddrc_register();

	debug("DDR size is : %d MByte\n", (global_reg_value->DDR_CHIP_0_SIZE + global_reg_value->DDR_CHIP_1_SIZE) / 1024 /1024);
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

	/*init ddr params in uboot env. */
	get_ddr_params();
	ram_size = (unsigned int)(global_reg_value->DDR_CHIP_0_SIZE) + (unsigned int)(global_reg_value->DDR_CHIP_1_SIZE);
	debug("ram_size=%x\n", ram_size);

	if (ram_size > EMC_LOW_SDRAM_SPACE_SIZE)
		ram_size = EMC_LOW_SDRAM_SPACE_SIZE;

	return ram_size;
}
