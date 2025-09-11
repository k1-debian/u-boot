//#define DEBUG

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/arch/clk.h>
#include <asm/arch/cpm.h>
#include <asm/arch/efuse.h>
#include <cloner/cloner.h>
#include <efuse.h>

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

static int cmp_data(unsigned int *src, unsigned int *dst,unsigned int len)
{
        unsigned int *start = src;
        unsigned int *end_src = src + len;
        while(start < end_src)
        {
//		LOG_INFO("cmp data  src:%08x, dst:%08x\n", *start, *dst);
                if(*start++ != *dst++) {
                        LOG_ERROR("cmp data error: src:%08x, dst:%08x\n", *(start-1), *(dst-1));
                        return (start - src);
                }
        }
        return 0;
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
	LOG_INFO("rate = %d, ns = %d\n", rate, ns);

	for(i = 0; i <= 0xf; i++) {
		if((i + 2) * ns > 15)
			break;
	}

	if(i > 0xf) {
		LOG_ERROR("rd_adj and wr_adj fail!\n");
		return -1;
	}

	rd_adj = wr_adj = i;

	for(i = 0; i <= 0x1f; i++) {
		if(((rd_adj + i + 48) * ns) > 150)
			break;
	}
	if(i > 0x1f) {
		LOG_ERROR("rd_strobe failed!\n");
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
		LOG_ERROR("wr_strobe failed!\n");
		return -1;
	}

	wr_strobe = i;

	if(negative_flag)
		wr_strobe |= (1 << 10);


	LOG_INFO("rd_adj = %d | rd_strobe = %d | wr_adj = %d | wr_strobe = %d\n",
			rd_adj, rd_strobe, wr_adj, wr_strobe);

	/*set configer register*/
	val = (rd_adj << EFUSE_REG_CFG_RD_ADJ) | (rd_strobe << EFUSE_REG_CFG_RD_STROBE);
	val |= (wr_adj << EFUSE_REG_CFG_WR_ADJ) | wr_strobe;

	REG32(EFUSE_REG_CFG) = val;

	return 0;
}

static int efuse_update_state(void)
{
	mdelay(10); /* Introduce delay between consecutive operations */
	REG32(EFUSE_REG_STAT) = 0;
	REG32(EFUSE_REG_CTRL) = EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	LOG_INFO("PROT = %x\n", REG32(EFUSE_REG_DAT0));
	LOG_INFO("STAT = %x\n", REG32(EFUSE_REG_STAT));

//	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/
	mdelay(10); /* Introduce delay between consecutive operations */
	REG32(EFUSE_REG_CTRL) = 0;

	return 0;
}

static int redundancy_rd(void)
{
	mdelay(10); /* Introduce delay between consecutive operations */
	REG32(EFUSE_REG_DAT0) = 0;
	REG32(EFUSE_REG_CTRL) = (0x1f << EFUSE_REGOFF_CRTL_ADDR) | (1 << EFUSE_REGOFF_CRTL_LENG) | EFUSE_REG_CTRL_RWL;
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;
	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));
	LOG_INFO("RIR0 = 0x%08x\n", REG32(EFUSE_REG_DAT0));
	LOG_INFO("RIR1 = 0x%08x\n", REG32(EFUSE_REG_DAT1));

//	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/
	mdelay(10); /* Introduce delay between consecutive operations */
	REG32(EFUSE_REG_CTRL) = 0;

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
	REG32(EFUSE_REG_CTRL) = 0;
	REG32(EFUSE_REG_DAT0) = PRT_REDUNDANCY << offset;

	efuse_1v8_output(efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*power on*/
	mdelay(10);

	REG32(EFUSE_REG_CTRL) |= (EFUSE_ADDR_PROT << EFUSE_REGOFF_CRTL_ADDR) | (0 << EFUSE_REGOFF_CRTL_LENG);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_WTEN; /*write en*/

	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_WTDONE));

	REG32(EFUSE_REG_CTRL) &= ~EFUSE_REG_CTRL_WTEN;
	REG32(EFUSE_REG_CTRL) &= ~EFUSE_REG_CTRL_PGEN;
	REG32(EFUSE_REG_CTRL) &= ~EFUSE_REG_CTRL_PS; /*power on*/

//	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/
	mdelay(10);

	efuse_1v8_output(!efuse_args->efuse_en_active);

	redundancy_rd();
	efuse_update_state();

	return 0;
}



static int mcu_wtotp(int opera)
{
	unsigned int ret = 0;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *retval = (volatile unsigned int *)MCU_TCSM_RETVAL;

	mdelay(10); /* Introduce delay between consecutive operations */
	REG32(EFUSE_REG_STAT) = 0;
	REG32(EFUSE_REG_CTRL) = 0;

	efuse_1v8_output(efuse_args->efuse_en_active);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PS; /*power on*/
	mdelay(10);
	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PGEN; /*pg en*/

	args->arg[0] = opera;
	ret = secall(args, SC_FUNC_WTOTP, 0, 1);

	efuse_1v8_output(!efuse_args->efuse_en_active);

	mdelay(10);
	REG32(EFUSE_REG_CTRL) &= ~EFUSE_REG_CTRL_PGEN;
	REG32(EFUSE_REG_CTRL) &= ~EFUSE_REG_CTRL_PS;
//	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_PD; /*power down*/
//	REG32(EFUSE_REG_CTRL) = 0;

	redundancy_rd();
	efuse_update_state();

	if (*retval != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_WTOTP failed, ret = 0x%08x\n", *retval);
		return -ESEC;
	}

	return 0;
}

int otp_init(void)
{
	int ret;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	secall(args, SC_FUNC_INIT_SCRAM, 0, 1);
	secall(args, SC_FUNC_INIT, 0, 1);

	LOG_INFO("Enter: %s\n",__func__);
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

	ret = set_efuse_timing();
	if (ret < 0)
		return ret;

	redundancy_rd();
	efuse_update_state();
	*(volatile unsigned int *)(MCU_TCSM_RETVAL) = 0;
	return 0;
}
#if 0
static int test_sha256(void)
{
	unsigned int ret;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *input = (volatile unsigned int *)MCU_TCSM_INDATA;
	volatile unsigned int *output = (volatile unsigned int *)MCU_TCSM_NKUSIG;

	int iLoop = 0;
	int hash_endround = 1;
	int hash_newround = 1;

	unsigned int src[] = {
		0xed19e044, 0xe39195c2, 0xf9865834, 0xf71f5e4c,
		0x254e42e3, 0xe2152a64, 0xcedf12f5, 0x90368c26,
		0x8e45a322, 0x32dcb23f, 0xdc93ba6c, 0x1f413023,
		0xc572c8d9, 0xbf32d4da, 0x8abfc305, 0x34073d4c,
		0x68cc7971, 0xd2528711, 0x502aba47, 0xb746dcc2,
		0xfd6ca9ef, 0x502781ac, 0x7865995a, 0xa28061e3,
		0x83a86f69, 0xe97ad4c7, 0x215acc4a, 0x9b1f84c3,
		0x0aacd5e5, 0xebe243bb, 0x07373439, 0xc51bb560,
		0x1ee0b212, 0x7c6adceb, 0x443915e6, 0x954b43a8,
		0xc81d2341, 0x9e7139bf, 0xa0883018, 0xb9fe557f,
		0x6b7e04e5, 0x670e85fe, 0x824215c7, 0x41beb5bb,
		0xaddd9ea1, 0x0a8dbfe2, 0x17a25cfe, 0xdc0384c7,
		0xee3ab9aa, 0x629408b6, 0xb0f4f830, 0xaf8cd49b,
		0x083329dc, 0xe9b861ba, 0x1bd6336f, 0x66e0006f,
		0x673c2d51, 0x046a242a, 0x817724a6, 0x204c3daa,
		0x705bef57, 0x7c494b98, 0x7a1cfc4c, 0x71c0dcc3,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00010001,
#if 0
		0x34333231,
		0x38373635,
		0x31313039,
		0x0a313131
#endif
	};
	unsigned int srclen = ARRAY_SIZE(src);

	unsigned int expect[] = {
		0xf5591380,
		0xcad31ffb,
		0xeb94779f,
		0x7c8dd17b,
		0xab00b044,
		0xa93edb36,
		0x8a77e53c,
		0xae8aaedd,
#if 0
		0x29a31111,
		0x0bcdaadb,
		0x78be4168,
		0xa3b4ad9e,
		0x8d0c8370,
		0x8011be5c,
		0xc8c567a5,
		0x852038dd
#endif
	};
	unsigned int explen = ARRAY_SIZE(expect);

	for(iLoop = 0; iLoop < 32; iLoop++)
		output[iLoop] = 0;

	printf("input:\n");
	for (iLoop = 0; iLoop < srclen; iLoop++){
		input[iLoop] = src[iLoop];
		printf("0x%08x\n", input[iLoop]);
	}
	printf("\n");

	args->arg[0] = srclen | hash_endround << 16 | hash_newround << 18 | HASH_SET(HASH_SELECT_SHA256);
	args->arg[1] = MCU_TCSM_PADDR(input);
	args->arg[2] = MCU_TCSM_PADDR(output);

	ret = secall(args, SC_FUNC_HASH, 0, 1);

	printf("output:\n");
	for(iLoop = 0; iLoop < 8;  iLoop++)
		printf("0x%08x\n", output[iLoop]);
	printf("\n");

	ret = cmp_data(output, expect, explen);

	if(ret) {
		printf("****************hash test failed******************\n");
		return -1;
	}

	printf("****************hash test success******************\n");

	return 0;
}
#endif

int cpu_burn_rckey(void)
{
	unsigned int ret;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	LOG_INFO("Enter: %s\n",__func__);

#if 1
	if(EFUSTATE_CK_PRT) {
		LOG_INFO("EFUSTATE: chipkey protection bit is set!\n");
		return 0;
	}

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
		LOG_INFO("chipkey check exists!\n");
	} else if (ret != SC_ERR_SUCC && ret != SC_ERR_RIR) {
		LOG_INFO("SC_FUNC_BURNCK failed, ret = %x\n",
                             *(volatile unsigned int *)(MCU_TCSM_RETVAL));
		return -ESEC;
	}

	if (mcu_wtotp(WT_OTP_CK) < 0) {
		LOG_ERROR("chipkey write failed\n");
		return -ESEC;
	}

	otp_w(EFUSE_PTCOFF_CKP);
#else
	while(1)
	test_sha256();
#endif
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
	volatile unsigned int *nkusig = (volatile unsigned int *)MCU_TCSM_NKUSIG;
	volatile unsigned int *retval = (volatile unsigned int *)MCU_TCSM_RETVAL;
	unsigned int *pbuf = idata + 2;

	secall(args, SC_FUNC_INIT, 0, 1);

	LOG_INFO("Enter: %s\n",__func__);

	rsakeylen = (length - 8) / 2;
	nku[0] = rsakeylen * 8;
	nku[1] = rsakeylen * 8;
	rsa_key_word = rsakeylen / 4;

	LOG_DEBUG("N %d BITS\n",nku[0]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2] = pbuf[iLoop];

		LOG_DEBUG("%08x ", nku[iLoop + 2]);
		if((iLoop + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	LOG_DEBUG("KU %d BITS\n",nku[1]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2 + rsa_key_word] = pbuf[iLoop + rsa_key_word];

		LOG_DEBUG("%08x ", nku[iLoop + 2 + rsa_key_word]);
		if((iLoop + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_BURNNKU, 0, 1);

	if (*retval != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_BURNNKU failed, ret = %x\n", *retval);
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
	volatile unsigned int *nkusig = (volatile unsigned int *)MCU_TCSM_NKUSIG;
	volatile unsigned int *retval = (volatile unsigned int *)MCU_TCSM_RETVAL;
	unsigned int *pbuf = idata + 2;

	LOG_INFO("Enter: %s\n",__func__);

	secall(args, SC_FUNC_INIT, 0, 1);

	rsakeylen = (length - 8) / 2;
	nku[0] = rsakeylen * 8;
	nku[1] = rsakeylen * 8;
	rsa_key_word = rsakeylen / 4;

	LOG_DEBUG("N %d BITS\n",nku[0]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2] = pbuf[iLoop];

		LOG_DEBUG("%08x ", nku[iLoop + 2]);
		if((iLoop + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	LOG_DEBUG("KU %d BITS\n",nku[1]);
	for (iLoop = 0; iLoop < rsa_key_word; iLoop++) {
		nku[iLoop + 2 + rsa_key_word] = pbuf[iLoop + rsa_key_word];

		LOG_DEBUG("%08x ", nku[iLoop + 2 + rsa_key_word]);
		if((iLoop + 1) % 4 == 0)
			LOG_DEBUG("\n");
	}

	args->arg[0] = MCU_TCSM_PADDR(nku);
	ret = secall(args, SC_FUNC_CHECKNKU, 0, 1);

	if (*retval != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_CHECKNKU failed, ret = %x\n", *retval);
		return -ESEC;
	}
	LOG_INFO("SC_FUNC_CHECKNKU Success\n");
	return 0;
}

int cpu_burn_custid(void *idata,unsigned int length)
{
	unsigned int rdata[CUSTID0_WORD_NUM * 2] = {0};
	unsigned int len = length / 2;
	int ret;

	LOG_INFO("Enter: %s\n",__func__);

	if (EFUSTATE_CUSTID0_PRT) {
		LOG_INFO("EFUSTATE: custid0 protection bit is set!\n");
		return 0;
	}

	if (len != CUSTID0_BIT_NUM / 8) {
		LOG_ERROR("data length error! CUSTID0 length = %d\n",CUSTID0_BIT_NUM / 8);
		return -ESEC;
	}

#if 0
	ret = efuse_init(efuse_args->efuse_en_gpio, efuse_args->efuse_en_active);
	if (ret) {
		LOG_ERROR("efuse init error\n");
		return -ESEC;
	}
#endif

	LOG_INFO("write custid0\n");
	ret = efuse_write(idata, length, CUSTID0);
	if (ret) {
		LOG_ERROR("efuse write error\n");
		return -ESEC;
	}

	LOG_INFO("read custid0\n");
	ret = efuse_read_id((void*)rdata, length, CUSTID0);
	if (ret != length) {
		LOG_ERROR("efuse read error\n");
		return -ESEC;
	}

	LOG_INFO("check custid0 read back data\n");
	ret = strncmp(idata, rdata, length - 1);
	if (ret) {
		LOG_ERROR("compare data error\n");
		return -ESEC;
	}
	LOG_INFO("set custid0 protection bit\n");
	if (otp_w(EFUSE_PTCOFF_UID) < 0) {
		LOG_ERROR("custid0 protection bit set failed\n");
		return -ESEC;
	}

	LOG_INFO("check custid0 protection bit\n");
	if (!EFUSTATE_CUSTID0_PRT) {
		LOG_ERROR("custid0 protection bit is not set!\n");
		return -ESEC;
	}

	return 0;
}

int cpu_burn_nku(void *idata,unsigned int length)
{
	unsigned int ret = 0;

	LOG_INFO("Enter: %s\n",__func__);

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
	if (mcu_wtotp(WT_OTP_NKU) < 0) {
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

static int mcu_aes_encrypt(unsigned int *input, unsigned int *output, int data_len,
		unsigned int *key, int key_len, int key_flag)
{
	unsigned int ret, i;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	volatile unsigned int *mcu_key = (volatile unsigned int *)(MCU_TCSM_INDATA);
	volatile unsigned int *mcu_input = (volatile unsigned int *)(MCU_TCSM_INDATA + 0x30);
	volatile unsigned int *mcu_output = (volatile unsigned int *)(MCU_TCSM_OUTDATA);
	volatile unsigned int *retval = (volatile unsigned int *)(MCU_TCSM_RETVAL);


	for(i = 0; i < data_len; i++) {
		mcu_input[i] = input[i];
	}
	for(i = 0; i < 8; i++) {
		mcu_output[i] = 0;
	}

	if (key_flag) {
		args->arg[0] = key_flag | (AES_256BIT << 12);
		args->arg[2] = MCU_TCSM_PADDR(mcu_input);
		args->arg[3] = MCU_TCSM_PADDR(mcu_output);
		args->arg[4] = data_len;

		ret = secall(args, SC_FUNC_AESBYKEY, 0, 1);
	} else {

		for(i = 0; i < key_len; i++) {
			mcu_key[i] = key[i];
		}
		args->arg[0] = AES_256BIT << 12;
		args->arg[1] = MCU_TCSM_PADDR(mcu_key);
		args->arg[2] = MCU_TCSM_PADDR(mcu_input);
		args->arg[3] = MCU_TCSM_PADDR(mcu_output);
		args->arg[4] = data_len / 4;
//		args->arg[5] = MCU_TCSM_PADDR(iv);
		args->arg[6] = 3;

		ret = secall(args, SC_FUNC_AES, 0, 1);
	}

	if (*retval != SC_ERR_SUCC) {
		LOG_INFO("SC_FUNC_AES failed, ret = %x\n", *retval);
		return -ESEC;
	}

	for(i = 0; i < 8; i++) {
		LOG_DEBUG("output[%d]: %08x\n", i, mcu_output[i]);
		output[i] = mcu_output[i];
	}

	return 0;
}

static int check_ukey(unsigned int *ukey, int key_len, int key_flag)
{
	unsigned int ori_data[] = {0x34333231,0x38373635,0x31313039,0x0a313131};
	unsigned int in_key_enc_data[] = {0,0,0,0,0,0,0,0};
	unsigned int ex_key_enc_data[] = {0,0,0,0,0,0,0,0};
	int ori_data_len = ARRAY_SIZE(ori_data);
	int enc_data_len = ARRAY_SIZE(in_key_enc_data);

	mcu_aes_encrypt(ori_data, ex_key_enc_data, ori_data_len, ukey, key_len, 0);
	mcu_aes_encrypt(ori_data, in_key_enc_data, ori_data_len, NULL, 0, key_flag);

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
	volatile unsigned int *retval = (volatile unsigned int *)MCU_TCSM_RETVAL;
	unsigned int *pbuf = (unsigned int *)idata;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();

	LOG_INFO("Enter: %s\n",__func__);

	if(EFUSTATE_UK_PRT && EFUSTATE_UK1_PRT) {
		LOG_INFO("EFUSTATE: userkey0/1 protection bits is set!\n");
		return 0;
	}


#define UKEY_LEN_WORD    8
#define UKEY_F_OFFSET    0x02
#define UKEY1_F_OFFSET   0x03

        if (!EFUSTATE_UK_PRT) {
		secall(args, SC_FUNC_INIT, 0, 1);

                LOG_INFO("UK0 loaded into mcu sram\n");
                LOG_DEBUG("UK0 %d WORD\n", UKEY_LEN_WORD);
                for (iLoop = 0; iLoop < UKEY_LEN_WORD; iLoop++) {
                        ukey[iLoop] = pbuf[iLoop];

                        LOG_DEBUG("%08x ",ukey[iLoop]);
                        if((iLoop + 1) % 4 == 0)
                                LOG_DEBUG("\n");
                }


                args->arg[0] = (0x01 << UKEY_F_OFFSET);
                args->arg[1] = MCU_TCSM_PADDR(ukey);

                ret = secall(args, SC_FUNC_BURNUK, 0, 1);

                if (*retval != SC_ERR_SUCC) {
                        LOG_INFO("SC_FUNC_BURNUK failed, ret = %x\n", *retval);
                        return -ESEC;
                }

                LOG_INFO("write UK0 to efuse\n");
                if (mcu_wtotp(WT_OTP_UK) < 0) {
                        LOG_ERROR("UK0 mcu write failed!\n");
                        return -ESEC;
                }

                LOG_INFO("set UK0 protection bit\n");
                otp_w(EFUSE_PTCOFF_UKP);
                if(!EFUSTATE_UK_PRT) {
                        LOG_ERROR("UK0 protection bit set failed!\n");
                        return -ESEC;
                }

		if (check_ukey(ukey, UKEY_LEN_WORD, AES_BY_UKEY)) {
                        LOG_ERROR("UK0 check failed!\n");
                        return -ESEC;
		}
		LOG_INFO("UK0 check success!\n");
        }


        if (!EFUSTATE_UK1_PRT) {
		secall(args, SC_FUNC_INIT, 0, 1);
                memset(ukey, 0, MCU_TCSM_KEYLEN);

                LOG_INFO("UK1 loaded into mcu sram\n");
                LOG_DEBUG("UK1 %d WORD\n", UKEY_LEN_WORD);
                for (iLoop = 0; iLoop < UKEY_LEN_WORD; iLoop++) {
                        ukey[iLoop] = pbuf[iLoop + UKEY_LEN_WORD];

                        LOG_DEBUG("%08x ",ukey[iLoop]);
                        if((iLoop + 1) % 4 == 0)
                                LOG_DEBUG("\n");
                }

                args->arg[0] = (0x01 << UKEY1_F_OFFSET);
                args->arg[2] = MCU_TCSM_PADDR(ukey);

                ret = secall(args, SC_FUNC_BURNUK, 0, 1);

                if (*retval != SC_ERR_SUCC) {
                        LOG_INFO("SC_FUNC_BURNUK failed, ret = %x\n", *retval);
                        return -ESEC;
                }


                LOG_INFO("write UK1 to efuse\n");
                if (mcu_wtotp(WT_OTP_UK1) < 0) {
                        LOG_ERROR("UK1 mcu write failed!\n");
                        return -ESEC;
                }

                LOG_INFO("set UK1 protection bit\n");
                otp_w(EFUSE_PTCOFF_UKP1);
                if(!EFUSTATE_UK1_PRT) {
                        LOG_ERROR("UK1 protection bit set failed!\n");
                        return -ESEC;
                }

		if (check_ukey(ukey, UKEY_LEN_WORD, AES_BY_UKEY1)) {
                        LOG_ERROR("UK1 check failed!\n");
                        return -ESEC;
		}
		LOG_INFO("UK1 check success!\n");
	}
	return 0;
}

int cpu_burn_secboot_enable(void)
{
	LOG_INFO("Enter: %s\n",__func__);

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
	otp_w(EFUSE_PTCOFF_SEC);
	if (!EFUSTATE_SECBOOT_EN) {
		LOG_ERROR("security enable bit set failed!\n");
		return -ESEC;
	}

	LOG_INFO("set disable jtag bit\n");
	otp_w(EFUSE_PTCOFF_DJG);
	if (!EFUSTATE_DIS_JTAG) {
		LOG_ERROR("disable jtag bit set failed!\n");
		return -ESEC;
	}

	LOG_INFO("set security protection bit\n");
	otp_w(EFUSE_PTCOFF_SCB);
	if (!EFUSTATE_SCB_PRT) {
		LOG_ERROR("security protection bit set failed!\n");
		return -ESEC;
	}

	LOG_INFO("\nsecurity enable successful!\n");

	return 0;
}
