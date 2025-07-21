//#define DEBUG

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/arch/cpm.h>

#include "secall.h"
#include "pdma.h"
#include "aes.h"
#include "otp.h"

#include <cloner/cloner.h>
static int efuse_en_gpio = -1;

#ifdef CONFIG_PMU_RICOH6x
#include <regulator.h>
#define PMU_EFUSE_1V8	"RICOH619_LDO2"
static struct regulator *efuse_1v8 = NULL;
extern int ricoh61x_regulator_init(void);
#endif

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

static void efuse_1v8_output(int value)
{
	if(efuse_en_gpio != 0xffffffff || efuse_en_gpio != -1) {
		mdelay(2);		/* wait for EFUSE IO power for mdelay(1). */
		gpio_direction_output(efuse_en_gpio, value);
		LOG_INFO("EFUSE_EN_N gpio(%d) output %s!\n", efuse_en_gpio, value == 0 ? "low" : "high");
		mdelay(2);
	}
#ifdef CONFIG_PMU_RICOH6x
	else {
		mdelay(10);		/* delay 10ms for power down. prevent miss of WT_DONE. */
		if(value == 0) {
			regulator_set_voltage(efuse_1v8, 1800000, 1800000);
			regulator_enable(efuse_1v8);
		} else {
			regulator_disable(efuse_1v8);
		}
		mdelay(10);		/* wait for EFUSE IO power for mdelay(10). */
	}
#endif
}

static int efuse_config(void)
{
	/*efuse register*/
	volatile unsigned int *reg_ctrl = (volatile unsigned int *)EFUSE_REG_CTRL;
	volatile unsigned int *reg_cfg = (volatile unsigned int *)EFUSE_REG_CFG;
	volatile unsigned int *reg_stat = (volatile unsigned int *)EFUSE_REG_STAT;
	/*cpm register*/
	volatile unsigned int *reg_cpccr = (volatile unsigned int *)(CPM_BASE + CPM_CPCCR);
	volatile unsigned int *reg_cpapcr = (volatile unsigned int *)(CPM_BASE + CPM_CPAPCR);
	volatile unsigned int *reg_cpmpcr = (volatile unsigned int *)(CPM_BASE + CPM_CPMPCR);

	LOG_INFO("reg_cfg = %x\n",*reg_cfg);
	LOG_INFO("reg_cpccr = %x\n",*reg_cpccr);
	LOG_INFO("reg_cpmpcr = %x\n",*reg_cpmpcr);
	LOG_INFO("reg_stat: = %x\n", *reg_stat);

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
		int apll_m = ((*reg_cpapcr & 0x3ff<<20)>>20) + 1;
		int apll_n = ((*reg_cpapcr & 0x3f<<14)>>14) + 1;
		int apll_o = ((*reg_cpapcr & 0x7<<11)>>11) + 1;

		pll = 24 * 2 * apll_m / (apll_n * apll_o);
		LOG_INFO("AHB2 select APLL : NF=%d, NR=%d, NO=%d, FOUT=%d\n", apll_m, apll_n, apll_o, pll);
	} else if(sel_a == 2) {
		int mpll_m = ((*reg_cpmpcr & 0x3ff<<20)>>20) + 1;
		int mpll_n = ((*reg_cpmpcr & 0x3f<<14)>>14) + 1;
		int mpll_o = ((*reg_cpmpcr & 0x7<<11)>>11) + 1;

		pll = 24 * 2 * mpll_m / (mpll_n * mpll_o);
		LOG_INFO("AHB2 select MPLL : NF=%d, NR=%d, NO=%d, FOUT=%d\n", mpll_m, mpll_n, mpll_o, pll);
	}

	int ahb2 = pll/h2div;
	int ahb2_cycle= 1000/ahb2; //ns

	int wr_adj = 0;
	int rd_adj = 0;
	while(1) {
		if((wr_adj + 1) * ahb2_cycle > 4) {
			LOG_INFO("wr_adj = %x\n",wr_adj);
			break;
		}
		wr_adj ++;
		rd_adj ++;
	}

	int flag = 0;
	int wr_strobe = 0;
	while(1) {

		if((ahb2_cycle * (wr_adj+3000 + wr_strobe)) > 11000 &&
				(ahb2_cycle * (wr_adj+3000 + wr_strobe)) < 13000) {
			LOG_INFO("wr_strobe = %x\n",wr_strobe);
			break;
		}

		wr_strobe++;
		if((flag && wr_strobe == 0x7ff) || (!flag && wr_strobe == 0x3ff)) {
			LOG_ERROR("efuse can't run in bad AHB2 Frequency!!!!!!!\n");
			return -1;
		}

		if((ahb2_cycle * (wr_adj + 3000)) > 13000) {
			if((ahb2_cycle * (wr_adj+3000 - wr_strobe)) > 11000 &&
				(ahb2_cycle * (wr_adj+3000 - wr_strobe)) < 13000) {
				wr_strobe |= (1 << 10);
				LOG_INFO("wr_strobe = %x\n",wr_strobe);
				break;
			}
			flag = 1;
		}
	}

	int rd_strobe = 0;
	while(1) {
		if(((rd_adj + rd_strobe + 30) * ahb2_cycle) > 100) {
			LOG_INFO("rd_strobe = %x\n",rd_strobe);
			break;
		}

		rd_strobe++;
	}

	*reg_cfg = (rd_adj << 24) | (rd_strobe << 16) | (wr_adj<<12) | wr_strobe;
	LOG_INFO("reg_cfg = %x\n",*reg_cfg);
	LOG_INFO("mpll = %d\n",pll);
	LOG_INFO("ahb2 = %d\n",ahb2);
	return 0;
}

static int efuse_update_state(void)
{
	REG32(EFUSE_REG_STAT) = 0;
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	LOG_INFO("EFUSTATE: data1 = %x\n", REG32(EFUSE_REG_DAT1));
	LOG_INFO("EFUSTATE: state = %x\n", REG32(EFUSE_REG_STAT));
	REG32(EFUSE_REG_STAT) = 0;
}

static int redundancy_rd(void)
{
	REG32(EFUSE_REG_DAT1) = 0;
	LOG_INFO("EFUSE_REG_DAT1 = 0x%08x\n", REG32(EFUSE_REG_DAT1));
	REG32(EFUSE_REG_CTRL) = (0x1f << EFUSE_REGOFF_CRTL_ADDR) | (1 << EFUSE_REGOFF_CRTL_LENG) | EFUSE_REG_CTRL_RWL;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	LOG_INFO("EFUSE_REG_DAT1 = 0x%08x\n", REG32(EFUSE_REG_DAT1));
	LOG_INFO("EFUSE_REG_DAT2 = 0x%08x\n", REG32(EFUSE_REG_DAT2));
	REG32(EFUSE_REG_CTRL) = 0;
}

static int otp_r()
{
	efuse_1v8_output(!efuse_args->efuse_en_active);
	
        REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_STAT) = 0;
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR) | (0 << EFUSE_REGOFF_CRTL_LENG);
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	LOG_INFO("EFUSE_REG_DAT1 = %x\n",REG32(EFUSE_REG_DAT1));
        REG32(EFUSE_REG_STAT) = 0;

	return 0;
}

static int otp_w(unsigned int offset)
{
	if (offset >= 16) {
		LOG_ERROR("offset too big!\n");
		return -1;
	}
	unsigned int ret;
#define PRT_REDUNDANCY  0x00010001
	REG32(EFUSE_REG_DAT1) = PRT_REDUNDANCY << offset;
	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR) | (0 << EFUSE_REGOFF_CRTL_LENG);
	
        efuse_1v8_output(efuse_args->efuse_en_active);
	
        REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*pg en*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN; /*write en*/
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));
	
        efuse_1v8_output(!efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS);

	otp_r();

	return 0;
}



static int cpu_wtotp(int opera)
{
	unsigned int ret = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	redundancy_rd();
	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	REG32(EFUSE_REG_CTRL) = 0;
	
        efuse_1v8_output(efuse_args->efuse_en_active);
	
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS;
        REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;

	args->arg[0] = opera;
	ret = secall(args, SC_FUNC_WTOTP, 0, 1);

        efuse_1v8_output(!efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS);
	mdelay(2);		/* mdelay 2ms after clear CTRL_PGEN, waiting for AVDEFUSE down. */

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_WTOTP failed, ret = 0x%08x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -1;

	}

	efuse_update_state();
	redundancy_rd();

	return 0;
}

int otp_init(void)
{
	int ret;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT, 0, 1);

	LOG_INFO("\nEnter: %s\n",__func__);
	efuse_en_gpio = efuse_args->efuse_en_gpio;
	if(efuse_en_gpio != 0xffffffff || efuse_en_gpio != -1) {
		LOG_INFO("EFUSE_EN_N gpio(%d) output high!\n", efuse_en_gpio);
		gpio_direction_output(efuse_en_gpio, efuse_args->efuse_en_active);
	}
#ifdef CONFIG_PMU_RICOH6x
	else {
		ret = ricoh61x_regulator_init();
		if(ret < 0) {
			LOG_ERROR("regulator init error!\n");
			return -ESEC;
		}

		efuse_1v8 = regulator_get(PMU_EFUSE_1V8);
		if(efuse_1v8 == NULL){
			LOG_ERROR("regulator get efuse 1.8v error!\n");
			return -ESEC;
		}
	}
#endif

	efuse_config();
	efuse_update_state();
	*(volatile unsigned int *)(MCU_TCSM_RETVAL) = SC_ERR_SUCC;
	return 0;
}


int cpu_burn_rckey(void)
{
	unsigned int ret;
	volatile struct sc_args *args;
	volatile int *rir_ret = (volatile unsigned int *)MCU_TCSM_RETRIR;
	memset(rir_ret, 0, 16);

	LOG_INFO("\nEnter: %s\n",__func__);
	if(EFUSTATE_CK_PRT) {
		LOG_INFO("EFUSTATE: chipkey protection bit is set!\n");
		return 0;
	}

	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT, 0, 1);

	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;

	ret = secall(args, SC_FUNC_BURNCK, 0, 1);

	ret = *(volatile unsigned int *)(MCU_TCSM_RETVAL);
	if(ret == SC_ERR_CK_EXISTENCE) {
		LOG_INFO("chipkey check exists!\n");
	} else if (ret != SC_ERR_SUCC && ret != SC_ERR_RIR) {
		LOG_INFO("SC_FUNC_BURNCK failed, ret = %x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	if (cpu_wtotp(WT_OTP_CK) < 0) {
		LOG_ERROR("chipkey write failed\n");
		return -ESEC;
	}

	otp_w(EFUSE_PTCOFF_CKP);

	return 0;
}

int cpu_load_nku(unsigned int *idata, unsigned int length)
{
	unsigned int ret;
	unsigned int iLoop;
	unsigned int rsa_key_word = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *nku = (volatile unsigned int *)MCU_TCSM_NKU;
	secall(args, SC_FUNC_INIT, 0, 1);

	LOG_INFO("\nEnter: %s\n",__func__);

	set_rsakey(idata + 2, length - 8);

	nku[0] = rsakeylen * 8;
	nku[1] = rsakeylen * 8;
	rsa_key_word = rsakeylen / 4;

	debug("N %d BITS\n",nku[0]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2] = rsakey[iLoop];

		debug("%08x ", nku[iLoop + 2]);
		if((iLoop + 1) % 4 == 0)
			debug("\n");
	}

	debug("KU %d BITS\n",nku[1]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2 + rsa_key_word] = rsakey[iLoop + rsa_key_word];

		debug("%08x ", nku[iLoop + 2 + rsa_key_word]);
		if((iLoop + 1) % 4 == 0)
			debug("\n");
	}

	REG32(EFUSE_REG_CTRL) = 0;
	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_BURNNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_BURNNKU failed, ret = %x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	return 0;
}

static int check_nku(unsigned int *idata, unsigned int length)
{
	unsigned int ret;
	unsigned int iLoop;
	unsigned int rsa_key_word = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *nku = (volatile unsigned int *)MCU_TCSM_NKU;
	LOG_INFO("\nEnter: %s\n",__func__);

	set_rsakey(idata + 2, length - 8);

	nku[0] = rsakeylen * 8;
	nku[1] = rsakeylen * 8;
	rsa_key_word = rsakeylen / 4;

	debug("N %d BITS\n",nku[0]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2] = rsakey[iLoop];

		debug("%08x ", nku[iLoop + 2]);
		if((iLoop + 1) % 4 == 0)
			debug("\n");
	}

	debug("KU %d BITS\n",nku[1]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2 + rsa_key_word] = rsakey[iLoop + rsa_key_word];

		debug("%08x ", nku[iLoop + 2 + rsa_key_word]);
		if((iLoop + 1) % 4 == 0)
			debug("\n");
	}

	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	REG32(EFUSE_REG_CTRL) = 0;
	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_CHECKNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_CHECKNKU failed! ret = 0x%08x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -1;
	}
	LOG_INFO("SC_FUNC_CHECKNKU Success\n");
	return 0;
}


int cpu_burn_nku(void *idata,unsigned int length)
{
	unsigned int ret = 0;
	volatile int *rir_ret = (volatile unsigned int *)MCU_TCSM_RETRIR;
	memset(rir_ret, 0, 16);

	LOG_INFO("\nEnter: %s\n",__func__);
	if (EFUSTATE_NKU_PRT) {
		LOG_INFO("EFUSTATE: nku protection bit is set!\n");
		return 0;
	}

        LOG_INFO("NKU loaded into mcu sram\n");
	if (cpu_load_nku(idata, length) < 0) {
		LOG_ERROR("nku load failed\n");
		return -ESEC;
	}

        LOG_INFO("NKU write to efuse\n");
	if (cpu_wtotp(WT_OTP_NKU) < 0) {
		LOG_ERROR("nku write failed\n");
		return -ESEC;
	}

        LOG_INFO("set NKU protection bit\n");
	if (otp_w(EFUSE_PTCOFF_NKU) < 0) {
		LOG_ERROR("nku protection bit set failed\n");
		return -ESEC;
	}

	if (!EFUSTATE_NKU_PRT) {
		LOG_ERROR("nku protection bit is not set!\n");
		return -ESEC;
	}

        LOG_INFO("check NKU\n");
	if (check_nku(idata, length) < 0) {
		LOG_ERROR("nku check failed\n");
		return -ESEC;
	}

	return 0;

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
	volatile unsigned int *ukey = (volatile unsigned int *)MCU_TCSM_PUTUKEY;
	unsigned int *rsaukey = (unsigned int *)idata;
	volatile int *rir_ret = (volatile unsigned int *)MCU_TCSM_RETRIR;
	memset(rir_ret, 0, 16);

	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	LOG_INFO("\nEnter: %s\n",__func__);

	if(EFUSTATE_UK_PRT && EFUSTATE_UK1_PRT) {
		LOG_INFO("EFUSTATE: userkey0/1 protection bits is set!\n");
		return 0;
	}

//	do_rsa(rsaukey, rsakeylen, encukey, rsakey, rsakeylen);
//	for(iLoop = 0; iLoop < 4; iLoop++)
//		LOG_INFO("encukey[%d]: %x\n", iLoop, encukey[iLoop]);

#define UKEY_LEN_WORD    8
#define UKEY_F_OFFSET    0x02
#define UKEY1_F_OFFSET   0x03

        if (!EFUSTATE_UK_PRT) {
		secall(args, SC_FUNC_INIT, 0, 1);
                
                LOG_INFO("UK0 loaded into mcu sram\n");
                debug("UK0 %d WORD\n", UKEY_LEN_WORD);
                for (iLoop = 0; iLoop < UKEY_LEN_WORD; iLoop++) {
                        ukey[iLoop] = rsaukey[iLoop] /*encukey[iLoop]*/;

                        debug("%08x ",ukey[iLoop]);
                        if((iLoop + 1) % 4 == 0)
                                debug("\n");
                }

                args->arg[0] = (0x01 << UKEY_F_OFFSET);
                args->arg[1] = MCU_TCSM_PADDR(ukey);

                ret = secall(args, SC_FUNC_BURNUK, 0, 1);

                if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
                        LOG_INFO("SC_FUNC_BURNUK failed, ret = %x\n",
                                     *(volatile unsigned int *)(MCU_TCSM_RETVAL));
                        return -ESEC;
                }

                LOG_INFO("write UK0 to efuse\n");
                if (cpu_wtotp(WT_OTP_UK) < 0) {
                        LOG_ERROR("UK0 mcu write failed!\n");
                        return -ESEC;
                }

                LOG_INFO("set UK1 protection bit\n");
                otp_w(EFUSE_PTCOFF_UKP);

                if(!EFUSTATE_UK_PRT) {
                        LOG_ERROR("UK0 protection bit set failed!\n");
                        return -ESEC;
                }
        }


        if (!EFUSTATE_UK1_PRT) {
		secall(args, SC_FUNC_INIT, 0, 1);
		memset(ukey, 0, MCU_TCSM_KEYLEN);

		LOG_INFO("UK1 loaded into mcu sram\n");
		debug("UK1 %d WORD\n", UKEY_LEN_WORD);
                for (iLoop = 0; iLoop < UKEY_LEN_WORD; iLoop++) {
                        ukey[iLoop] = rsaukey[iLoop + UKEY_LEN_WORD] /*encukey[iLoop]*/;

                        debug("%08x ",ukey[iLoop]);
                        if((iLoop + 1) % 4 == 0)
                                debug("\n");
                }

                args->arg[0] = (0x01 << UKEY1_F_OFFSET);
                args->arg[2] = MCU_TCSM_PADDR(ukey);

                ret = secall(args, SC_FUNC_BURNUK, 0, 1);

                if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
                        LOG_INFO("SC_FUNC_BURNUK failed, ret = %x\n",
                                     *(volatile unsigned int *)(MCU_TCSM_RETVAL));
                        return -ESEC;
                }


                LOG_INFO("write UK1 to efuse\n");

                if (cpu_wtotp(WT_OTP_UK1) < 0) {
                        LOG_ERROR("UK1 mcu write failed!\n");
                        return -ESEC;
                }

                LOG_INFO("set UK1 protection bit\n");
                otp_w(EFUSE_PTCOFF_UKP1);

                if(!EFUSTATE_UK1_PRT) {
                        LOG_ERROR("UK1 protection bit set failed!\n");
                        return -ESEC;
                }
        }
	return 0;
}

int cpu_burn_secboot_enable(void)
{
	LOG_INFO("\nEnter: %s\n",__func__);
	
        efuse_update_state();

	if (!EFUSTATE_UK_PRT || !EFUSTATE_UK1_PRT) {
		LOG_ERROR("userkey protection bit is not set!\n");
		return -ESEC;
	}

	if (!EFUSTATE_NKU_PRT) {
		LOG_ERROR("nku protection bit is not set!\n");
		return -ESEC;
	}

        if (!EFUSTATE_SECBOOT_EN && EFUSTATE_SCB_PRT) {
		LOG_ERROR("security protection bit is set, but enable bit is not set!\n");
		return -ESEC;
	}

        LOG_INFO("set security enable bit\n");

	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	
        REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_STAT) = 0;

	/* set security boot enable, security boot enable protected, disable JTAG */
	REG32(EFUSE_REG_DAT1) = ((1 << EFUSE_PTCOFF_SEC) | (1 << EFUSE_PTCOFF_SCB) | (1 << EFUSE_PTCOFF_DJG));

	/*efuse config*/
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	
        efuse_1v8_output(efuse_args->efuse_en_active);

        REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS;
        REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));

        efuse_1v8_output(!efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN | EFUSE_REG_CTRL_PS);
	mdelay(2);		/* mdelay 2ms after clear CTRL_PGEN, waiting for AVDEFUSE down. */

	efuse_update_state();

	if (!EFUSTATE_SCB_PRT) {
		LOG_ERROR("security protection bit set failed!\n");
		return -ESEC;
        }
	if (!EFUSTATE_SECBOOT_EN) {
		LOG_ERROR("security enable bit set failed!\n");
		return -ESEC;
        }
	if (!EFUSTATE_DIS_JTAG) {
		LOG_ERROR("disable jtag bit set failed!\n");
		return -ESEC;
	}

	return 0;
}
