#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include "secall.h"
#include "pdma.h"
#include "aes.h"
#include "otp.h"
#include "test_nku.h"
#include <cloner/cloner.h>

#include <regulator.h>
#define PMU_EFUSE_1V8	"RICOH619_LDO2"
static struct regulator *efuse_1v8 = NULL;

unsigned int rsakey[256];

unsigned int rsakeylen;

static void set_rsakey(unsigned int *idata, unsigned int length)
{
	unsigned int iLoop;

	memset(rsakey, 0, sizeof(rsakey));
	for(iLoop = 0; iLoop < length / 4; iLoop++)
		rsakey[iLoop] = idata[iLoop];

	rsakeylen = length / 2;
}

int get_rsakeylen(void)
{
	return rsakeylen;
}

static void efuse_1v8_output(int enable)
{
	mdelay(1);		/* delay 1ms for power down. prevent miss of WT_DONE. */
	if(enable) {
		regulator_enable(efuse_1v8);
	} else {
		regulator_disable(efuse_1v8);
	}
	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
}

static int efuse_config(void)
{
	/*efuse register*/
	volatile unsigned int *reg_ctrl = (volatile unsigned int *)EFUSE_REG_CTRL;
	volatile unsigned int *reg_cfg = (volatile unsigned int *)EFUSE_REG_CFG;
	volatile unsigned int *reg_stat = (volatile unsigned int *)EFUSE_REG_STAT;
	/*cpm register*/
	volatile unsigned int *reg_cpccr = (volatile unsigned int *)0xb0000000;
	volatile unsigned int *reg_cpapcr = (volatile unsigned int *)0xb0000010;
	volatile unsigned int *reg_cpmpcr = (volatile unsigned int *)0xb0000014;

//	printf("xxxxxxx reg_cfg = %x\n",*reg_cfg);
//	printf("xxxxxxx reg_cpccr = %x\n",*reg_cpccr);
//	printf("xxxxxxx reg_cpmpcr = %x\n",*reg_cpmpcr);
//	printf("xxxxxxx reg_stat: = %x\n", *reg_stat);

	int cfg = 0;
	int h2div = ((*reg_cpccr & 0xf<<12)>>12) + 1;
	int sel_a = 0;
	if(((*reg_cpccr >> 24) & 0x3) == 1) {
		sel_a = 1;
	} else if (((*reg_cpccr >> 24) & 0x3) == 2) {
		sel_a = 2;
	}

	int pll = 0;
	if(sel_a == 1) {
		int apll_m = ((*reg_cpapcr & 0x7f<<24)>>24) + 1;
		int apll_n = ((*reg_cpapcr & 0x1f<<18)>>18) + 1;
		int apll_o = ((*reg_cpapcr & 0x3<<16)>>16) + 1;

		pll = 24*apll_m/(apll_n * apll_o);
//		printf(" xxxx AHB2 select APLL : %d\n", pll);
	} else if(sel_a == 2) {
		int mpll_m = ((*reg_cpmpcr & 0x7f<<24)>>24) + 1;
		int mpll_n = ((*reg_cpmpcr & 0x1f<<18)>>18) + 1;
		int mpll_o = ((*reg_cpmpcr & 0x3<<16)>>16) + 1;

		pll = 24*mpll_m/(mpll_n*mpll_o);
//		printf(" xxxx AHB2 select MPLL : %d\n", pll);
	}

	int ahb2 = pll/h2div;

	int ahb2_cycle= 1000/ahb2; //ns

	int wr_adj = 0;
	int rd_adj = 0;
	while(1) {
		if((wr_adj + 1) * ahb2_cycle > 2) {
//			printf("-----wr_adj = %x --\n",wr_adj);
			break;
		}
		wr_adj ++;
		rd_adj ++;
	}

	int wr_strobe = 0;
	while(1) {
		if(((ahb2_cycle * (wr_adj+916 + wr_strobe)) > 4000) && ((ahb2_cycle * (wr_adj+916 + wr_strobe))< 6000)) {
//			printf("-----wr_strobe = %x --\n",wr_strobe);
			break;
		}
		if(ahb2_cycle * (wr_adj+916 + wr_strobe) > 6000) {
			printf("!!!!!!!!!!!! efuse can't run in bad AHB2 Frequency!!!!!!!\n");
			break;
		}
		wr_strobe++;
	}


	int rd_strobe = 0;
	while(1) {
		if(((rd_adj + 3 + rd_strobe) * ahb2_cycle) > 15) {
//			printf("-----rd_strobe = %x --\n",rd_strobe);
			break;
		}

		rd_strobe++;
	}

	//rd_adj = 100;
	//rd_strobe = 100;
	*reg_cfg = (rd_adj << 19) | (rd_strobe << 16) | (wr_adj<<12) | wr_strobe;
//	printf("xxxxxxx reg_cfg = %x\n",*reg_cfg);
//	printf("xxxxxxx mpll = %d\n",pll);
//	printf("xxxxxxx ahb2 = %d\n",ahb2);

}

static int efuse_update_state(void)
{
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	printf("xxxxxxx data updated: %x\n", *(unsigned int *)EFUSE_REG_DAT1);
	printf("xxxxxxx state updated: %x\n", REG32(EFUSE_REG_STAT));
}

int cpu_wtotp(int opera)
{
	unsigned int ret = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS; /*pg en*/
	efuse_1v8_output(1);

	args->arg[0] = opera;
	ret = secall(args, SC_FUNC_WTOTP, 0, 1);

	efuse_1v8_output(0);
	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS);

	printf("**************************MCU_TCSM_RETVAL = 0x%08x\n", MCU_TCSM_RETVAL);
	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		printf("secall SC_FUNC_WTOTP fail 0x%08x\n", *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -1;

	}

	efuse_update_state();

	return 0;
}

void otp_init(void)
{
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT, 0, 1);

	efuse_1v8 = regulator_get(PMU_EFUSE_1V8);
	if(efuse_1v8 == NULL){
		printf("get efuse 1.8v regulator error!\n");
		return;
	}
	regulator_set_voltage(efuse_1v8, 1800000, 1800000);

	efuse_config();
	efuse_update_state();
	*(volatile unsigned int *)(MCU_TCSM_RETVAL) = SC_ERR_SUCC;
}

int otp_r()
{
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR | 0x01 << EFUSE_REGOFF_CRTL_LENG);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;

	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));

	printf("REG32(EFUSE_REG_DAT1) = %x\n",REG32(EFUSE_REG_DAT1));
	return 0;
}

static int otp_w(unsigned int offset)
{
	if (offset >= 16) {
		fprintf(stderr, "offset too big!\n");
		return -1;
	}
	unsigned int ret;
#define PRT_REDUNDANCY  0x00010001
	REG32(EFUSE_REG_DAT1) = PRT_REDUNDANCY << offset;
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR) | (0 << EFUSE_REGOFF_CRTL_LENG);
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*pg en*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/
	efuse_1v8_output(1);
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN; /*write en*/

	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));

	efuse_1v8_output(0);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS);

	otp_r();

	return 0;
}

int cpu_burn_rckey(void)
{
	unsigned int ret;
	volatile struct sc_args *args;

	if(EFUSTATE_CK_PRT)
		return 0;


	args = (volatile struct sc_args *)GET_SC_ARGS();
	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS; /*pg en*/
	efuse_1v8_output(1);

	ret = secall(args, SC_FUNC_BURNCK, 0, 1);

	ret = *(volatile unsigned int *)(MCU_TCSM_RETVAL);

	if (ret != SC_ERR_SUCC && ret != SC_ERR_CK_EXISTENCE) {
		printf("#################### SC_FUNC_BURNRKCK fail 0x%08x\n", ret);
		efuse_1v8_output(0);
		return -ESEC;
	}

	efuse_1v8_output(0);
	REG32(EFUSE_REG_CTRL) = 0;

	ret = otp_w(EFUSE_PTCOFF_CKP);

	return ret;
}

int cpu_load_nku(unsigned int *idata, unsigned int length)
{
	unsigned int ret;
	unsigned int iLoop;
	unsigned int rsa_key_word = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *nku = (volatile unsigned int *)MCU_TCSM_NKU;

	printf("xxxxxxxxxxx func : %s\n",__func__);

	set_rsakey(idata + 2, length - 8);

	nku[0] = rsakeylen * 8;
	nku[1] = rsakeylen * 8;
	rsa_key_word = rsakeylen / 4;

	printf("%s %s %d %d %d rsakeylen = %d\n", __FILE__, __func__, __LINE__, nku[0], nku[1], rsakeylen);


	for (iLoop = 0; iLoop < rsa_key_word; iLoop++)
		nku[iLoop + 2] = rsakey[iLoop];
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++)
		nku[iLoop + 2 + rsa_key_word] = rsakey[iLoop + rsakeylen / 4];

	for (iLoop = 2; iLoop < rsakeylen / 2 + 2; iLoop++) {

		if (iLoop % 6 == 0)
			printf("\n");
		if (iLoop != 0 && iLoop == 66)
			printf("\n");

		printf("%x ", nku[iLoop]);
	}

	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_BURNNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		printf("burn nku err, ret val %x\n",*(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	return 0;
}

int cpu_burn_nku(void *idata,unsigned int length)
{
	unsigned int ret = 0;

	if (EFUSTATE_NKU_PRT)
		return 0;

	if (cpu_load_nku(idata, length) < 0)
		return -ESEC;

	if (cpu_wtotp(WT_OTP_NKU) < 0)
		return -ESEC;

	ret = otp_w(EFUSE_PTCOFF_NKU);

	return ret;

}

int cpu_get_enckey(unsigned int *odata)
{
	return 0;
}

int cpu_burn_ukey(void *idata)
{
	unsigned int ret;
	unsigned int iLoop;
	unsigned int encukey[4] = {0};
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *ukey = (volatile unsigned int *)MCU_TCSM_PUTUKEY;
	unsigned int *rsaukey = (unsigned int *)idata;

	printf("xxxxxxxxxxx func : %s\n",__func__);

	if(EFUSTATE_UK_PRT && EFUSTATE_UK1_PRT)
		return 0;
	printf("xxxxxxxxxxx func : %s %d %d\n",__func__, EFUSTATE_UK_PRT, EFUSTATE_UK1_PRT);

//	do_rsa(rsaukey, rsakeylen, encukey, rsakey, rsakeylen);
//	for(iLoop = 0; iLoop < 4; iLoop++)
//		printf("encukey[%d]: %x\n", iLoop, encukey[iLoop]);

#define UKEY_LEN_WORD    8
#define UKEY_F_OFFSET    0x02
#define UKEY1_F_OFFSET   0x03

	for (iLoop = 0; iLoop < UKEY_LEN_WORD * 2; iLoop++)
		ukey[iLoop] = rsaukey[iLoop] /*encukey[iLoop]*/;

	for(iLoop = 0; iLoop < UKEY_LEN_WORD * 2; iLoop++) {
		if (iLoop % UKEY_LEN_WORD == 0)
			printf("\n");

		printf("%x ", ukey[iLoop]);
	}

	args->arg[0] = (0x01 << UKEY_F_OFFSET) | (0x01 << UKEY1_F_OFFSET);
	args->arg[1] = MCU_TCSM_PADDR(ukey);
	args->arg[2] = MCU_TCSM_PADDR(&ukey[UKEY_LEN_WORD]);

	ret = secall(args, SC_FUNC_BURNUK, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		printf("burn ukey err, ret val %x\n",*(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	if (EFUSTATE_UK_PRT == 0) {
		if (cpu_wtotp(WT_OTP_UK) < 0) {
			return -ESEC;
		}

		otp_w(EFUSE_PTCOFF_UKP);
	}

	if (EFUSTATE_UK1_PRT == 0) {
		if (cpu_wtotp(WT_OTP_UK1) < 0) {
			printf("%s %d\n", __func__, __LINE__);
			return -ESEC;
		}

		otp_w(EFUSE_PTCOFF_UKP1);
	}
	printf("xxxxxxxxxxx func : %s %d %d\n",__func__, EFUSTATE_UK_PRT, EFUSTATE_UK1_PRT);
	printf("%s %d\n", __func__, __LINE__);

	return 0;
}

int cpu_burn_secboot_enable(void)
{
	printf("xxxx otp efuse state:%x\n", REG32(EFUSE_REG_STAT));

	/* set write data :security boot enable, security boot enable protected, disable JTAG*/
	REG32(EFUSE_REG_DAT1) = ((1 << EFUSE_PTCOFF_SEC) | (1 << EFUSE_PTCOFF_SCB)
							 | (1 << EFUSE_PTCOFF_DJG));

	/*efuse config*/
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS; /*pg en*/

	efuse_1v8_output(1);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN; /*write en*/

	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));

	efuse_1v8_output(0);
	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS);

	efuse_update_state();

	return 0;
}
