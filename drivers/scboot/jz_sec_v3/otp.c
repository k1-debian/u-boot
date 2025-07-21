#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#include <asm/arch/efuse.h>
#include <cloner/cloner.h>

#include "secall.h"
#include "pdma.h"
#include "aes.h"
#include "otp.h"


static int efuse_en_gpio = -1;
static int efuse_en_active = 0;

static void set_efuse_vddq(int gpio, int level)
{
	mdelay(2);		/*  mdelay(10) wait for EFUSE VDDQ setup. */
	gpio_direction_output(gpio, level);
	LOG_INFO("gpio[%d] output %s\n",gpio,(level ? "high":"low"));
	mdelay(2);		/*  mdelay(10) wait for EFUSE VDDQ setup. */
}

static int efuse_update_state(void)
{
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	LOG_INFO("efuse state = 0x%08x\n",REG32(EFUSE_REG_STAT));
}

static int cpu_wtotp(int opera)
{
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	REG32(EFUSE_REG_CTRL) = 0;

	set_efuse_vddq(efuse_en_gpio, efuse_en_active);
	
        REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;
	args->arg[0] = opera;
	secall(args, SC_FUNC_WTOTP, 0, 1);
	
        set_efuse_vddq(efuse_en_gpio, !efuse_en_active);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_ERROR("SC_FUNC_WTOTP ret = 0x%08x\n",
				*(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	efuse_update_state();

	return 0;
}

static int otp_r(void)
{
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR);
	REG32(EFUSE_REG_CTRL) |= (1 << EFUSE_REGOFF_CRTL_LENG);
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	return 0;
}


static int otp_w(unsigned int offset)
{
	REG32(EFUSE_REG_DAT1) = (1 << offset);
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR);
	REG32(EFUSE_REG_CTRL) |= (1 << EFUSE_REGOFF_CRTL_LENG);

	set_efuse_vddq(efuse_en_gpio, efuse_en_active);
	
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));
	
	set_efuse_vddq(efuse_en_gpio, !efuse_en_active);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN);

	otp_r();
	return 0;
}

int get_rsakeylen(void)
{
	return 0;
}


int cpu_get_enckey(unsigned int *odata)
{
	return 0;
}


int cpu_burn_secboot_enable(void)
{
        LOG_INFO("Enter: %s\n",__func__);
	
        REG32(EFUSE_REG_DAT1) = (1 << EFUSE_PTCOFF_SEC) | (1 << EFUSE_PTCOFF_SCB);

	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= (1 << EFUSE_REGOFF_CRTL_LENG);

	set_efuse_vddq(efuse_en_gpio, efuse_en_active);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;
        REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));
	
        set_efuse_vddq(efuse_en_gpio, !efuse_en_active);

	REG32(EFUSE_REG_CTRL) &= ~(EFUSE_REG_CTRL_PGEN);

	efuse_update_state();

	if (!EFUSTATE_SECBOOT_EN) {
		LOG_ERROR("secure enable bit write failed!\n");
	}

        if (!EFUSTATE_SCB_PRT) {
		LOG_ERROR("secure enable protect bit write failed!\n");
	}

	return 0;
}



int cpu_burn_rckey(void)
{
	unsigned int ret;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);

        LOG_INFO("Enter: %s\n",__func__);
	
        if(EFUSTATE_CK_PRT) {
		LOG_ERROR("chipkey protect bit have been written\n");
		return 0;
	}

	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN;

	ret = secall(args, SC_FUNC_BURNCK, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_ERROR("SC_FUNC_BURNCK ret = 0x%08x\n",
				*(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}


	if (cpu_wtotp(WT_OTP_CK) < 0) {
		LOG_ERROR("wtotp error!\n");
		return -ESEC;
	}

	otp_w(EFUSE_PTCOFF_CKP);

	return 0;
}

static int cpu_load_nku(unsigned int *data, unsigned int length)
{
	int ret = -ESEC;
	int i = 0;
	int rsakey_byte_num = (length - 8) / 2;
	int rsakey_word_num = rsakey_byte_num / 4;
	int rsakey_bit_num = rsakey_byte_num * 8;

	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *nku = (volatile unsigned int *)MCU_TCSM_NKU;
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);

	nku[0] = rsakey_bit_num;
	nku[1] = rsakey_bit_num;

	LOG_DEBUG("rsa kn %d bits\n",nku[0]);
	for (i = 0; i < rsakey_word_num; i++) {
		nku[i + 2] = data[i + 2];
		LOG_DEBUG("%08x ", nku[i + 2]);
		if((i + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	LOG_DEBUG("rsa ku %d bits\n",nku[1]);
	for (i = 0; i < rsakey_word_num; i++) {
		nku[i + 2 + rsakey_word_num] = data[i + 2 + rsakey_word_num];

		LOG_DEBUG("%08x ",nku[i + 2 + rsakey_word_num]);
		if((i + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	REG32(EFUSE_REG_CTRL) = 0;
	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_BURNNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_ERROR("SC_FUNC_BURNNKU ret = 0x%08x\n",
				*(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	return 0;
}

static int check_nku(unsigned int *data, unsigned int length)
{
	int ret = -ESEC;
	int i = 0;
	int rsakey_byte_num = (length - 8) / 2;
	int rsakey_word_num = rsakey_byte_num / 4;
	int rsakey_bit_num = rsakey_byte_num * 8;

	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *nku = (volatile unsigned int *)MCU_TCSM_NKU;
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);

	nku[0] = rsakey_bit_num;
	nku[1] = rsakey_bit_num;

	LOG_DEBUG("rsa kn %d bits\n",nku[0]);
	for (i = 0; i < rsakey_word_num; i++) {
		nku[i + 2] = data[i + 2];
		LOG_DEBUG("%08x ", nku[i + 2]);
		if((i + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	LOG_DEBUG("rsa ku %d bits\n",nku[1]);
	for (i = 0; i < rsakey_word_num; i++) {
		nku[i + 2 + rsakey_word_num] = data[i + 2 + rsakey_word_num];

		LOG_DEBUG("%08x ",nku[i + 2 + rsakey_word_num]);
		if((i + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	REG32(EFUSE_REG_CTRL) = 0;
	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_CHECKNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_ERROR("SC_FUNC_CHECKNKU ret = 0x%08x\n",
				*(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	return 0;
}


int cpu_burn_nku(void *data,unsigned int length)
{
        LOG_INFO("Enter: %s\n",__func__);
	
        if (EFUSTATE_NKU_PRT) {
		LOG_INFO("nku protect bit is set!\n");
		return 0;
	}

	if (cpu_load_nku(data, length) < 0) {
		LOG_ERROR("nku load failed\n");
		return -ESEC;
	}


	if (cpu_wtotp(WT_OTP_NKU) < 0) {
		LOG_ERROR("nku write failed\n");
		return -ESEC;
	}

	if (otp_w(EFUSE_PTCOFF_NKU) < 0) {
		LOG_ERROR("nku protect bit set failed\n");
		return -ESEC;
	}

	if (!EFUSTATE_NKU_PRT) {
		LOG_ERROR("nku protect bit is not set!\n");
		return -ESEC;
	}

	if (check_nku(data, length) < 0) {
		LOG_ERROR("nku check failed\n");
		return -ESEC;
	}

	return 0;

}


int cpu_burn_ukey(void *data)
{
	int ret = -ESEC;
	int i = 0;
	unsigned int encukey[4] = {0};
	volatile unsigned int *ukey = (volatile unsigned int *)MCU_TCSM_PUTUKEY;
	unsigned int *userkey = (unsigned int *)data;

	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);
		
        LOG_INFO("Enter: %s\n",__func__);

	if(EFUSTATE_UK_PRT) {
		LOG_INFO("ukey protect bit is set!\n");
		return 0;
	}

#define UKEY_LEN_WORD    8
	LOG_DEBUG("uk %d bits\n",UKEY_LEN_WORD * 32);
	for (i = 0; i < UKEY_LEN_WORD; i++) {
		ukey[i] = userkey[i];
		LOG_DEBUG("%08x ",ukey[i]);
		if((i + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	args->arg[0] = 0x0;
	args->arg[1] = MCU_TCSM_PADDR(ukey);

	ret = secall(args, SC_FUNC_BURNUK, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		LOG_ERROR("SC_FUNC_BURNUK ret = 0x%08x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
                return -ESEC;
	}


	if (cpu_wtotp(WT_OTP_UK) < 0) {
		LOG_ERROR("ukey write failed!\n");
		return -ESEC;
	}

	otp_w(EFUSE_PTCOFF_UKP);

	if (!EFUSTATE_UK_PRT) {
		LOG_ERROR("ukey protect bit is not set!\n");
		return -ESEC;
	}
	return 0;
}


static int set_efuse_timing()
{
	unsigned int rate;
	uint32_t val, ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	int flag = 0;

	rate = clk_get_rate(H2CLK);
	ns = 1000000000 / rate;
	LOG_INFO("rate = %d, ns = %d\n", rate, ns);


	for(i = 0; i < 0x4; i++)
		if((( i + 1) * ns ) > 7)
			break;
	if(i == 0x4) {
		LOG_ERROR("rd_adj fail!\n");
		return -1;
	}
	rd_adj = wr_adj = i;

	for(i = 0; i < 0x8; i++)
		if(((rd_adj + i + 5) * ns ) > 35)
			break;
	if(i == 0x8) {
		LOG_ERROR("rd_strobe fail!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 0; i < 0x7ff; i++) {
		val = (wr_adj + i + 1666) * ns;
		if(val > 11 * 1000) {
			val = (wr_adj - i + 1666) * ns;
			flag = 1;
		}
		if(val > 9 * 1000 && val < 11 * 1000)
			break;
	}
	if(i >= 0x7ff) {
		LOG_ERROR("wd_strobe fail!\n");
		return -1;
	}

	if(flag)
		i |= 1 << 11;

	wr_strobe = i;

	LOG_INFO("rd_adj = %d | rd_strobe = %d | wr_adj = %d | wr_strobe = %d\n",
			rd_adj, rd_strobe, wr_adj, wr_strobe);

	/*set configer register*/
	val = rd_adj << EFUSE_CFG_RD_ADJ | rd_strobe << EFUSE_CFG_RD_STROBE;
	val |= wr_adj << EFUSE_CFG_WR_ADJ | wr_strobe;
	REG32(EFUSE_REG_CFG) = val;

	return 0;
}

int otp_init(void)
{
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);

        LOG_INFO("Enter: %s\n",__func__);
	
        efuse_en_gpio = efuse_args->efuse_en_gpio;
	if (efuse_en_gpio == 0xffffffff || efuse_en_gpio == -1) {
		LOG_ERROR("efuse en gpio is not set!\n");
		return -ESEC;
	} else if (efuse_args->efuse_en_active != 0xffffffff &&
			efuse_args->efuse_en_active != -1) {
		efuse_en_active = efuse_args->efuse_en_active;
	}
	set_efuse_vddq(efuse_en_gpio, !efuse_en_active);
	set_efuse_timing();
	efuse_update_state();
	*(volatile unsigned int *)(MCU_TCSM_RETVAL) = SC_ERR_SUCC;

	return 0;
}

