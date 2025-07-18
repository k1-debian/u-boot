//#define DEBUG

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#include <cloner/cloner.h>

#include "secall.h"
#include "pdma.h"
#include "aes.h"
#include "otp.h"


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
		serial_debug("EFUSE_EN_N gpio(%d) output %s!\n", efuse_en_gpio, value == 0 ? "low" : "high");
		mdelay(2);
	}
#ifdef CONFIG_PMU_RICOH6x
	else {
		mdelay(1);		/* delay 1ms for power down. prevent miss of WT_DONE. */
		if(value == 0) {
			regulator_set_voltage(efuse_1v8, 1800000, 1800000);
			regulator_enable(efuse_1v8);
		} else {
			regulator_disable(efuse_1v8);
		}
		mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	}
#endif
}

static int set_efuse_timing(void)
{
	unsigned int rate;
	uint32_t val, ns;
	uint32_t rd_adj, wr_adj;
	int rd_strobe, wr_strobe;
	int i;
	int negative_flag = 0;

	rate = clk_get_rate(H2CLK);
	ns = 1000000000 / rate;
	serial_debug("rate = %d, ns = %d\n", rate, ns);

	for(i = 0; i <= 0xf; i++) {
		if((i + 2) * ns > 15)
			break;
	}

	if(i > 0xf) {
		serial_debug("Error: rd_adj and wr_adj fail!\n");
		return -1;
	}

	rd_adj = wr_adj = i;

	for(i = 0; i <= 0x1f; i++) {
		if(((rd_adj + i + 48) * ns) > 150)
			break;
	}
	if(i > 0x1f) {
		serial_debug("Error: rd_strobe failed!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 0; i <= 0x3ff; i++) {
		val = (wr_adj + i + 3000) * ns;
		if(val > 13000) {
			val = (wr_adj - i + 3000) * ns;
			negative_flag = 1;
		}

                if (val >= 11500 && val <= 12500) {
			break;
		}
	}

	if(i > 0x3ff) {
		serial_debug("Error: wr_strobe failed!\n");
		return -1;
	}

	wr_strobe = i;

	if(negative_flag)
		wr_strobe |= (1 << 10);


	serial_debug("rd_adj = %d | rd_strobe = %d | wr_adj = %d | wr_strobe = %d\n",
			rd_adj, rd_strobe, wr_adj, wr_strobe);

	/*set configer register*/
	val = (rd_adj << EFUSE_REG_CFG_RD_ADJ) | (rd_strobe << EFUSE_REG_CFG_RD_STROBE);
	val |= (wr_adj << EFUSE_REG_CFG_WR_ADJ) | wr_strobe;

	REG32(EFUSE_REG_CFG) = val;

	return 0;
}

static int efuse_update_state(void)
{
	REG32(EFUSE_REG_STAT) = 0;
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	serial_debug("EFUSTATE: data0 = %x\n", REG32(EFUSE_REG_DAT0));
	serial_debug("EFUSTATE: state = %x\n", REG32(EFUSE_REG_STAT));
	REG32(EFUSE_REG_STAT) = 0;
}

static int redundancy_rd(void)
{
	REG32(EFUSE_REG_DAT0) = 0;
	serial_debug("EFUSE_REG_DAT0 = 0x%08x\n", REG32(EFUSE_REG_DAT0));
	REG32(EFUSE_REG_CTRL) = (0x1f << EFUSE_REGOFF_CRTL_ADDR) | (1 << EFUSE_REGOFF_CRTL_LENG) | EFUSE_REG_CTRL_RWL;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	serial_debug("EFUSE_REG_DAT0 = 0x%08x\n", REG32(EFUSE_REG_DAT0));
	serial_debug("EFUSE_REG_DAT1 = 0x%08x\n", REG32(EFUSE_REG_DAT1));
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
	serial_debug("EFUSE_REG_DAT0 = %x\n",REG32(EFUSE_REG_DAT0));
	REG32(EFUSE_REG_STAT) = 0;

	return 0;
}

static int otp_w(unsigned int offset)
{
	if (offset >= 16) {
		fprintf(stderr, "Error: offset too big!\n");
		return -1;
	}
	unsigned int ret;
#define PRT_REDUNDANCY  0x00010001
	REG32(EFUSE_REG_DAT0) = PRT_REDUNDANCY << offset;
	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) = (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR) | (0 << EFUSE_REGOFF_CRTL_LENG);

	efuse_1v8_output(efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*pg en*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN; /*write en*/

	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));

	efuse_1v8_output(!efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/

	otp_r();

	return 0;
}



static int mcu_wtotp(int opera)
{
	unsigned int ret = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	REG32(EFUSE_REG_STAT) = 0;
	REG32(EFUSE_REG_CTRL) = 0;

	efuse_1v8_output(efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*power on*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/

	args->arg[0] = opera;
	ret = secall(args, SC_FUNC_WTOTP, 0, 1);

	efuse_1v8_output(!efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/
	mdelay(2);		/* mdelay 2ms after clear CTRL_PGEN, waiting for AVDEFUSE down. */

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		serial_debug("SC_FUNC_WTOTP failed, ret = 0x%08x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -1;

	}

	efuse_update_state();

	return 0;
}

int otp_init(void)
{
	int ret;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);
	secall(args, SC_FUNC_INIT, 0, 1);

	serial_debug("\nEnter: %s\n",__func__);
	efuse_en_gpio = efuse_args->efuse_en_gpio;
	if(efuse_en_gpio != 0xffffffff || efuse_en_gpio != -1) {
		serial_debug("EFUSE_EN_N gpio(%d) output high!\n", efuse_en_gpio);
		gpio_direction_output(efuse_en_gpio, efuse_args->efuse_en_active);
	}
#ifdef CONFIG_PMU_RICOH6x
	else {
		ret = ricoh61x_regulator_init();
		if(ret < 0) {
			serial_debug("Error: regulator init error!\n");
			return -ESEC;
		}

		efuse_1v8 = regulator_get(PMU_EFUSE_1V8);
		if(efuse_1v8 == NULL){
			serial_debug("Error: regulator get efuse 1.8v error!\n");
			return -ESEC;
		}
	}
#endif

	ret = set_efuse_timing();
	if (ret < 0)
		return ret;

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

	serial_debug("\nEnter: %s\n",__func__);
	if(EFUSTATE_CK_PRT) {
		serial_debug("EFUSTATE: chipkey protection bit is set!\n");
		return 0;
	}

	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT, 0, 1);

	mdelay(1);		/* wait for EFUSE IO power for mdelay(1). */
	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*power on*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/

	ret = secall(args, SC_FUNC_BURNCK, 0, 1);

	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/

	ret = *(volatile unsigned int *)(MCU_TCSM_RETVAL);
	if(ret == SC_ERR_CK_EXISTENCE) {
		serial_debug("chipkey check exists!\n");
	} else if (ret != SC_ERR_SUCC && ret != SC_ERR_RIR) {
		serial_debug("SC_FUNC_BURNCK failed, ret = %x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	if (mcu_wtotp(WT_OTP_CK) < 0) {
		serial_debug("Error: chipkey write failed\n");
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

	serial_debug("\nEnter: %s\n",__func__);

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

	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_BURNNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		serial_debug("SC_FUNC_BURNNKU failed, ret = %x\n",
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
	serial_debug("\nEnter: %s\n",__func__);

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

	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_CHECKNKU, 0, 1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		serial_debug("SC_FUNC_CHECKNKU failed! ret = 0x%08x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -1;
	}
	serial_debug("SC_FUNC_CHECKNKU Success\n");
	return 0;
}


int cpu_burn_nku(void *idata,unsigned int length)
{
	unsigned int ret = 0;
	volatile int *rir_ret = (volatile unsigned int *)MCU_TCSM_RETRIR;
	memset(rir_ret, 0, 16);

	serial_debug("\nEnter: %s\n",__func__);
	if (EFUSTATE_NKU_PRT) {
		serial_debug("EFUSTATE: nku protection bit is set!\n");
		return 0;
	}

        serial_debug("NKU loaded into mcu sram\n");
	if (cpu_load_nku(idata, length) < 0) {
		serial_debug("Error: nku load failed\n");
		return -ESEC;
	}

        serial_debug("NKU write to efuse\n");
	if (mcu_wtotp(WT_OTP_NKU) < 0) {
		serial_debug("Error: nku write failed\n");
		return -ESEC;
	}

        serial_debug("set NKU protection bit\n");
	if (otp_w(EFUSE_PTCOFF_NKU) < 0) {
		serial_debug("Error: nku protection bit set failed\n");
		return -ESEC;
	}

	if (!EFUSTATE_NKU_PRT) {
		serial_debug("Error: nku protection bit is not set!\n");
		return -ESEC;
	}

        serial_debug("check NKU\n");
	if (check_nku(idata, length) < 0) {
		serial_debug("Error: nku check failed\n");
		return -ESEC;
	}

	return 0;

}

int cpu_get_enckey(unsigned int *odata)
{
	return 0;
}

static int mcu_aes_encrypt(unsigned int *input, unsigned int *output, unsigned int *key, int key_len, int data_len)
{
	unsigned int ret, i;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	volatile unsigned int *mcu_key = (volatile unsigned int *)(MCU_TCSM_INDATA);
	volatile unsigned int *mcu_input = (volatile unsigned int *)(MCU_TCSM_INDATA + 0x30);
	volatile unsigned int *mcu_output = (volatile unsigned int *)(MCU_TCSM_OUTDATA);


	for(i = 0; i < data_len; i++) {
		mcu_input[i] = input[i];
	}

	if (key != NULL) {
		for(i = 0; i < key_len; i++) {
			mcu_key[i] = key[i];
		}
		args->arg[0] = AES_256BIT << 12;
		args->arg[1] = MCU_TCSM_PADDR(mcu_key);
		args->arg[2] = MCU_TCSM_PADDR(mcu_input);
		args->arg[3] = MCU_TCSM_PADDR(mcu_output);
		args->arg[4] = data_len / 4;
		//	args->arg[5] = MCU_TCSM_PADDR(iv);
		args->arg[6] = 3;

		flush_cache_all();
		ret = secall(args, SC_FUNC_AES, 0, 1);
		flush_cache_all();
	} else {
		args->arg[0] = AES_BY_UKEY | (AES_256BIT << 12);
		args->arg[2] = MCU_TCSM_PADDR(mcu_input);
		args->arg[3] = MCU_TCSM_PADDR(mcu_output);
		args->arg[4] = data_len;

		ret = secall(args, SC_FUNC_AESBYKEY, 0, 1);
	}

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		serial_debug("SC_FUNC_AES failed, ret = %x\n",
			     *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	for(i = 0; i < data_len; i++) {
		debug("output[%d]: %08x\n", i, mcu_output[i]);
		output[i] = mcu_output[i];
	}

	serial_debug("mcu aes encrypt success!\n");
	return 0;
}

static int cmp_data(unsigned long *src,unsigned long *dst,unsigned long len)
{
        unsigned long *start = src;
        unsigned long *end_src = src + len;
        while(start < end_src)
        {
                if(*start++ != *dst++) {
                        serial_debug("cmp data error: src:%08x, dst:%08x\n", *(start-1), *(dst-1));
                        return (start - src);
                }
        }
        return 0;
}

static int check_ukey(unsigned int *ukey, int key_len)
{
	unsigned int ori_data[] = {0x34333231,0x38373635,0x31313039,0x0a313131};
	unsigned int in_key_enc_data[] = {0,0,0,0,0,0,0,0};
	unsigned int ex_key_enc_data[] = {0,0,0,0,0,0,0,0};
	int ori_data_len = ARRAY_SIZE(ori_data);
	int enc_data_len = ARRAY_SIZE(in_key_enc_data);

	mcu_aes_encrypt(ori_data, ex_key_enc_data, ukey, key_len, ori_data_len);
	mcu_aes_encrypt(ori_data, in_key_enc_data, NULL, 0, ori_data_len);

	if (cmp_data(ex_key_enc_data, in_key_enc_data, enc_data_len)) {
		return -1;
	}
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

	serial_debug("\nEnter: %s\n",__func__);

	if(EFUSTATE_UK_PRT && EFUSTATE_UK1_PRT) {
		serial_debug("EFUSTATE: userkey0/1 protection bits is set!\n");
		return 0;
	}

//	do_rsa(rsaukey, rsakeylen, encukey, rsakey, rsakeylen);
//	for(iLoop = 0; iLoop < 4; iLoop++)
//		serial_debug("encukey[%d]: %x\n", iLoop, encukey[iLoop]);

#define UKEY_LEN_WORD    8
#define UKEY_F_OFFSET    0x02
#define UKEY1_F_OFFSET   0x03

        if (!EFUSTATE_UK_PRT) {
		secall(args, SC_FUNC_INIT, 0, 1);
                
                serial_debug("UK0 loaded into mcu sram\n");
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
                        serial_debug("SC_FUNC_BURNUK failed, ret = %x\n",
                                     *(volatile unsigned int *)(MCU_TCSM_RETVAL));
                        return -ESEC;
                }

                serial_debug("write UK0 to efuse\n");
                if (mcu_wtotp(WT_OTP_UK) < 0) {
                        serial_debug("Error: UK0 mcu write failed!\n");
                        return -ESEC;
                }

                serial_debug("set UK1 protection bit\n");
                otp_w(EFUSE_PTCOFF_UKP);

                if(!EFUSTATE_UK_PRT) {
                        serial_debug("Error: UK0 protection bit set failed!\n");
                        return -ESEC;
                }

		if (check_ukey(ukey, UKEY_LEN_WORD)) {
                        serial_debug("Error: UK0 check failed!\n");
                        return -ESEC;
		}
        }


        if (!EFUSTATE_UK1_PRT) {
		secall(args, SC_FUNC_INIT, 0, 1);
                memset(ukey, 0, MCU_TCSM_KEYLEN);
                
                serial_debug("UK1 loaded into mcu sram\n");
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
                        serial_debug("SC_FUNC_BURNUK failed, ret = %x\n",
                                     *(volatile unsigned int *)(MCU_TCSM_RETVAL));
                        return -ESEC;
                }


                serial_debug("write UK1 to efuse\n");

                if (mcu_wtotp(WT_OTP_UK1) < 0) {
                        serial_debug("Error: UK1 mcu write failed!\n");
                        return -ESEC;
                }

                serial_debug("set UK1 protection bit\n");
                otp_w(EFUSE_PTCOFF_UKP1);

                if(!EFUSTATE_UK1_PRT) {
                        serial_debug("Error: UK1 protection bit set failed!\n");
                        return -ESEC;
                }
	}
	return 0;
}

int cpu_burn_secboot_enable(void)
{
	serial_debug("\nEnter: %s\n",__func__);

	efuse_update_state();

	if (!EFUSTATE_UK_PRT || !EFUSTATE_UK1_PRT) {
		serial_debug("Error: userkey protection bit is not set!\n");
		return -ESEC;
	}

	if (!EFUSTATE_NKU_PRT) {
		serial_debug("Error: nku protection bit is not set!\n");
		return -ESEC;
	}

	if (!EFUSTATE_SECBOOT_EN && EFUSTATE_SCB_PRT) {
		serial_debug("Error: security protection bit is set, but enable bit is not set!\n");
		return -ESEC;
	}


	serial_debug("set security enable bit\n");
	otp_w(EFUSE_PTCOFF_SEC);
	if (!EFUSTATE_SECBOOT_EN) {
		serial_debug("Error: security enable bit set failed!\n");
		return -ESEC;
	}

	serial_debug("set disable jtag bit\n");
	otp_w(EFUSE_PTCOFF_DJG);
	if (!EFUSTATE_DIS_JTAG) {
		serial_debug("Error: disable jtag bit set failed!\n");
		return -ESEC;
	}

	serial_debug("set security protection bit\n");
	otp_w(EFUSE_PTCOFF_SCB);
	if (!EFUSTATE_SCB_PRT) {
		serial_debug("Error: security protection bit set failed!\n");
		return -ESEC;
	}

	serial_debug("\nsecurity enable successful!\n");

	return 0;
}
