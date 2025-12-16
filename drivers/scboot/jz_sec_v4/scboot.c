#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>

#include "rsa1.h"
#include "secall.h"
#include "pdma.h"
#include "otp.h"
#include "aes.h"

/*
 * security boot.
 * 1. prepare a bin encrypted.
 *		|---------------|-------------------|					|
 *		| SC KEY(2048)	|  CODE encrypted   |
 *		|---------------|-------------------|					|
 * 2.
 *
 * */

#undef TCSM_CODE_ADDR
#undef TCSM_SC_KEY_ADDR
#undef MCU_TCSM_RETVAL
#undef MCU_TCSM_SECALL_MSG

#define TCSM_CODE_ADDR                  (TCSM_BANK(1) + 0)
#define TCSM_SC_KEY_ADDR                (TCSM_BANK(1) + 2048)
#define MCU_TCSM_RETVAL                 (TCSM_BANK(0) + 2048 + 1108) /* cal from sc_interface. */
#define MCU_TCSM_SECALL_MSG             (TCSM_BANK(0) + 2048 + 128)  /* MCU_TCSM_SECALL_MSG */

#define SC_MAX_SIZE_PERTIME             (2048)
#define SC_MAGIC_SIZE                   (512)
#define SC_KEY_SIZE                     (1536)

#define SECURE_SCBOOT_MAGIC		0x54424353

/* rsa key */
#ifdef CONFIG_RSA3072
#define RSA_KEY_WORD_SIZE               96
#else
#define RSA_KEY_WORD_SIZE               64
#endif

#define NKU_NKEY_WORD_OFF               2
#define NKU_KUKEY_WORD_OFF              (2 + RSA_KEY_WORD_SIZE)
#define NKU_KEY_LEN                     (RSA_KEY_WORD_SIZE)

/* sc key */
#define SC_KEY_INFO_WORD_SIZE           (64)
#define SC_KEY_N_WORD_SIZE              (RSA_KEY_WORD_SIZE)
#define SC_KEY_KU_WORD_SIZE             (RSA_KEY_WORD_SIZE)
#define SC_KEY_CODESIG_WORD_SIZE        (RSA_KEY_WORD_SIZE)


#define SC_KEY_INFO_WORD_OFF            (0)
#define SC_KEY_N_WORD_OFF               (SC_KEY_INFO_WORD_OFF + SC_KEY_INFO_WORD_SIZE)
#define SC_KEY_KU_WORD_OFF              (SC_KEY_N_WORD_OFF + SC_KEY_N_WORD_SIZE)
#define SC_KEY_CODESIG_WORD_OFF         (SC_KEY_KU_WORD_OFF + SC_KEY_KU_WORD_SIZE)

#define SC_HASH_PAD_LEN_WORD_OFF        (5)

/*
      ____________  0
      | head info  |
      |___________ |
      | env        |
      |___________ | 512b
      |  code_len  |
      |___________ | 516b
      |code_encrypt|
      |___________ | 520b
      | kn_bit     |
      |___________ | 524b
      | ku_bit     |
      |___________ | 528b
      |key_encrypt |
      |___________ | 532b
      |reserved    |
      |___________ | 768b
      |key_n       |
      |___________ | 1024b
      |key_u       |
      |___________ | 1280b
      |code_sig    |
      |___________ | 1536b
*/

static struct sckey {
	unsigned int code_len;             /* the len of program code */
	unsigned int code_encrypt;         /* the way of program code crypt ---- 0:don't crypt 1: chipkey 2: userkey*/
	unsigned int kn_bit;               /* the bits of key n */
	unsigned int ku_bit;               /* the bits of key u ---- public key*/
	unsigned int key_encrypt;          /* the way of key crypt*/
	unsigned int pad_len;
	unsigned int is_ckey_aes;
	unsigned int is_burn_uboot;
	unsigned int reserved_0[56];       /* [64 - 5]  pad to 2048bit / 256 Bytes / 64 words */
};

static void secure_check(void *addr, int *issig)
{
	int *ddrptr = (int *)(addr);
	int p = (unsigned long)SECURE_SCBOOT_MAGIC;

	if(*ddrptr++ == p) {
		*issig = 1;
	}
}

extern void flush_cache_all(void);

#ifdef CONFIG_RSA3072

static int setup_sckeys(void *addr, unsigned int *len, struct sckey *sckey)
{
	volatile struct sc_args *args = (volatile struct sc_args *)(MCU_TCSM_SECALL_MSG);
	volatile unsigned int *tcsmptr = (volatile unsigned int *)(TCSM_SC_KEY_ADDR);
	volatile unsigned int *rsa_key = (volatile unsigned int *)(MCU_TCSM_SPLSHA1ENCBUF);
	int *ddrptr = (int *)(addr + SC_MAGIC_SIZE);
	int iLoop = 0;
	unsigned int ret;

	if (sckey)
		memcpy((void*)sckey, ddrptr, SC_KEY_INFO_WORD_SIZE * 4);

	/* parsing sc_key: info */
	for (iLoop = 0; iLoop < SC_KEY_INFO_WORD_SIZE; iLoop++)
		tcsmptr[SC_KEY_INFO_WORD_OFF + iLoop]
			= ddrptr[SC_KEY_INFO_WORD_OFF + iLoop];

	/* parsing sc_key: codesig; (Note:Soft RSA needs to switch word between big and small end !) */
	for (iLoop = 0; iLoop < SC_KEY_CODESIG_WORD_SIZE; iLoop++)
		tcsmptr[SC_KEY_CODESIG_WORD_OFF + iLoop]
			= ddrptr[SC_KEY_CODESIG_WORD_OFF + (SC_KEY_CODESIG_WORD_SIZE - 1) - iLoop];


	*len = tcsmptr[0]; /* len in spl structure */

	/* len must 4 wrod align */
	if((*len) == 0 || (*len) % 16)
		return -1;


	if (sckey->key_encrypt) {
		/* parsing sc_key: n + ku */
		for (iLoop = 0; iLoop < SC_KEY_N_WORD_SIZE; iLoop++)
			tcsmptr[SC_KEY_N_WORD_OFF + iLoop] = ddrptr[SC_KEY_N_WORD_OFF + iLoop];

		for (iLoop = 0; iLoop < SC_KEY_KU_WORD_SIZE; iLoop++)
			tcsmptr[SC_KEY_KU_WORD_OFF + iLoop] = ddrptr[SC_KEY_KU_WORD_OFF + iLoop];

		args->arg[0] = AES_BY_UKEY | AES_CRYPT | (AES_256BIT << 12);
		args->arg[2] = MCU_TCSM_PADDR(&tcsmptr[SC_KEY_N_WORD_OFF]);
		args->arg[3] = MCU_TCSM_PADDR(&rsa_key[NKU_NKEY_WORD_OFF]);
		args->arg[4] = NKU_KEY_LEN * 2 * 4;

		secall(args, SC_FUNC_AESBYKEY, 0, 1);
		if (SC_RETVAL != SC_ERR_SUCC) {
			printf("Failed to rsakey decrypt, ret = %x\n", SC_RETVAL);
			return ret;
		}

#if 0
		int i;
		for (i = 0; i < SC_KEY_N_WORD_SIZE; i++) {
			if ((i%4) == 0)
				printf("\n");
			printf("%x ", rsa_key[NKU_NKEY_WORD_OFF + i]);
		}
		printf("\n");
#endif
	} else {

		for (iLoop = 0; iLoop < SC_KEY_N_WORD_SIZE; iLoop++)
			rsa_key[NKU_NKEY_WORD_OFF + iLoop] = ddrptr[SC_KEY_N_WORD_OFF + iLoop];
		for (iLoop = 0; iLoop < SC_KEY_KU_WORD_SIZE; iLoop++)
			rsa_key[NKU_KUKEY_WORD_OFF + iLoop] = ddrptr[SC_KEY_KU_WORD_OFF + iLoop];
	}

	/* rsa public decrtpt codesig: */
	{
		/* for rsa n; (Note:Soft RSA needs to switch word between big and small end !) */
		for (iLoop = 0; iLoop < SC_KEY_N_WORD_SIZE; iLoop++) {
			tcsmptr[SC_KEY_N_WORD_OFF + iLoop]
				= rsa_key[NKU_NKEY_WORD_OFF + (NKU_KEY_LEN - 1) - iLoop];
		}

		/* for rsa ku; (Note:Soft RSA needs to switch word between big and small end !) */
		tcsmptr[SC_KEY_KU_WORD_OFF]
			= rsa_key[NKU_KUKEY_WORD_OFF + (NKU_KEY_LEN - 1)];

		f_rsa_public_decrypt(tcsmptr + SC_KEY_CODESIG_WORD_OFF,
				tcsmptr + SC_KEY_CODESIG_WORD_OFF,
				SC_KEY_CODESIG_WORD_SIZE,
				tcsmptr + SC_KEY_N_WORD_OFF,
				tcsmptr + SC_KEY_KU_WORD_OFF,
				SC_KEY_N_WORD_SIZE);
	}

	return 0;
}

static int rsa3072_verify_signature(void *addr, struct sckey *sckey)
{
	volatile struct sc_args *args = (volatile struct sc_args *)(MCU_TCSM_SECALL_MSG);
	volatile unsigned int *tcsm_key = (volatile unsigned int *)(TCSM_SC_KEY_ADDR);
	volatile unsigned int *tcsm_code = (volatile unsigned int *)(TCSM_CODE_ADDR);
	volatile unsigned int *m_hash = (volatile unsigned int*)(MCU_TCSM_NKU);
	volatile unsigned int *em_hash = tcsm_key + SC_KEY_CODESIG_WORD_OFF;
	unsigned int *code = (unsigned int*)addr;
	int i, ret, len = 0, padlen = 0;

	if (!sckey) {
		serial_debug("ERROR: Invalid security information! %s:%d\n",__func__,__LINE__);
		return -1;
	}

	len = sckey->code_len;
	padlen = sckey->pad_len;

	//serial_debug("code len %d, pad len %d\n", len, padlen);
	secall(args, SC_FUNC_INIT, 0, 1);

	memset(m_hash, 0, 12);

	if (0) { //(padlen) {

		args->arg[0] = HASH_DMAMODE | HASH_SET(HASH_SELECT_SHA384);
		args->arg[1] = virt_to_phys(code);
		args->arg[2] = MCU_TCSM_PADDR(m_hash);
		args->arg[3] = (len / 64) + ((len % 64) ? 1 : 0);

		flush_cache_all();
		ret = secall(args, SC_FUNC_HASH, 0, 1);
		flush_cache_all();

		if (SC_RETVAL != SC_ERR_SUCC) {
			printf("Failed to code hash, ret = %x\n", SC_RETVAL);
			return ret;
		}

	} else {
		int newround = 1;
		int endround = 0;
		int code_size = len - padlen;
		int chunk_size;
		int pos = 0;

		while (!endround){
			chunk_size = code_size > SC_MAX_SIZE_PERTIME ? SC_MAX_SIZE_PERTIME : code_size;

			if (code_size <= SC_MAX_SIZE_PERTIME)
				endround = 1;

			for (i = 0; i < chunk_size / 4; i++) {
				tcsm_code[i] = code[pos++];
			}

			args->arg[0] = (chunk_size / 4) | newround << 16 | endround << 18 | HASH_SET(HASH_SELECT_SHA384);
			args->arg[1] = MCU_TCSM_PADDR(tcsm_code);
			args->arg[2] = MCU_TCSM_PADDR(m_hash);

			flush_cache_all();
			secall(args, SC_FUNC_HASH, 0, 1);
			flush_cache_all();

			if (SC_RETVAL != SC_ERR_SUCC) {
				serial_debug("Failed to code hash, ret = %x\n", SC_RETVAL);
			}

			newround = 0;
			code_size -= SC_MAX_SIZE_PERTIME;
		}
	}
#if 0
	printf("em hash:");
	for (i = 0; i < 96; i++) {
		if ((i%4) == 0)
			printf("\n");
		printf("%x ",em_hash[i]);
	}
	printf("\n");

	printf("m hash:");
	for (i = 0; i < 12; i++) {
		if ((i%4) == 0)
			printf("\n");
		printf("%x ",m_hash[i]);
	}
	printf("\n");
#endif
	args->arg[0] = SC_KEY_N_WORD_SIZE;
	args->arg[1] = m_hash;
	args->arg[2] = em_hash;
	ret = sign_verify(args->arg);

	serial_debug("RSA3072 verify signature %s!\n", ret & 0xFF ? "failure" : "success");
	return ret & 0xFF;
}

static int mcu_aes_decrypt(unsigned int *input, unsigned int *output, struct sckey *sckey)
{
	volatile struct sc_args *args = (volatile struct sc_args *)GET_SC_ARGS();
	unsigned int ret = 0, len = 0;

	if (!sckey) {
		serial_debug("ERROR: Invalid security information! %s:%d\n",__func__,__LINE__);
		return -1;
	}

	len = sckey->code_len;

	args->arg[0] = AES_BY_UKEY | AES_CRYPT | (AES_256BIT << 12) | 1 << 1;
	args->arg[2] = virt_to_phys(input);
	args->arg[3] = virt_to_phys(output);
	args->arg[4] = len;

	flush_cache_all();
	secall(args, SC_FUNC_AESBYKEY, 0, 1);
	flush_cache_all();

	if (SC_RETVAL != SC_ERR_SUCC) {
		printf("Failed to rsakey decrypt, ret = %x\n", SC_RETVAL);
		return ret & 0xFF;
	}

	return 0;
}

static int start_scboot(void *input, void *output, struct sckey *sckey)
{
	int ret = 0;

	if (!sckey) {
		serial_debug("ERROR: Invalid security information! %s:%d\n",__func__,__LINE__);
		return -1;
	}

	if (sckey->code_encrypt) {
		ret = mcu_aes_decrypt(input, output, sckey);
		if(ret) {
			serial_debug("ERROR: check your image, ret = %x!!\n", ret);
			return SC_RETVAL & 0xFF;
		}
	}

	ret = rsa3072_verify_signature(output, sckey);
	if (ret) {
		serial_debug("Failed to verify kernel signature!\n");
		return SC_RETVAL & 0xFF;
	}

	return ret;
}
#else

static int setup_sckeys(void *addr, unsigned int *len, struct sckey *sckey)
{
	volatile struct sc_args *args = (volatile struct sc_args *)(MCU_TCSM_SECALL_MSG);
	volatile unsigned int *tcsmptr = (volatile unsigned int *)(TCSM_SC_KEY_ADDR);
	unsigned int *rsa_key = (unsigned int *)(TCSM_SC_KEY_ADDR + 1024);
	int *ddrptr = (int *)(addr + SC_MAGIC_SIZE);
	int iLoop = 0;
	unsigned int ret;

	if (sckey)
		memcpy((void*)sckey, ddrptr, SC_KEY_INFO_WORD_SIZE * 4);

	/* parsing sc_key: info */
	for (iLoop = 0; iLoop < SC_KEY_INFO_WORD_SIZE; iLoop++)
		tcsmptr[SC_KEY_INFO_WORD_OFF + iLoop]
			= ddrptr[SC_KEY_INFO_WORD_OFF + iLoop];

	/* parsing sc_key: codesig; (Note:Soft RSA needs to switch word between big and small end !) */
	for (iLoop = 0; iLoop < SC_KEY_CODESIG_WORD_SIZE; iLoop++)
		tcsmptr[SC_KEY_CODESIG_WORD_OFF + iLoop]
			= ddrptr[SC_KEY_CODESIG_WORD_OFF + (SC_KEY_CODESIG_WORD_SIZE - 1) - iLoop];

	/* parsing sc_key: n + ku */
	rsa_key[0] = tcsmptr[2];
	rsa_key[1] = tcsmptr[3];

	for (iLoop = 0; iLoop < SC_KEY_N_WORD_SIZE; iLoop++)
		rsa_key[NKU_NKEY_WORD_OFF + iLoop] = ddrptr[SC_KEY_N_WORD_OFF + iLoop];
	for (iLoop = 0; iLoop < SC_KEY_KU_WORD_SIZE; iLoop++)
		rsa_key[NKU_KUKEY_WORD_OFF + iLoop] = ddrptr[SC_KEY_KU_WORD_OFF + iLoop];

	*len = tcsmptr[0]; /* len in spl structure */

	/* len must 4 wrod align */
	if((*len) == 0 || (*len) % 16)
		return -1;

	/*
	 * security init:
	 * 1.clear scram KEYDONE segment
	 * 2.nku verify
	 */
	args->arg[0] = tcsmptr[4];
	args->arg[1] = MCU_TCSM_PADDR(rsa_key);

	ret = secall(args, SC_FUNC_INIT, 0, 1);
	/* rsa public decrtpt codesig: */
	{
		/* for rsa n; (Note:Soft RSA needs to switch word between big and small end !) */
		for (iLoop = 0; iLoop < SC_KEY_N_WORD_SIZE; iLoop++) {
			tcsmptr[SC_KEY_N_WORD_OFF + iLoop]
				= rsa_key[NKU_NKEY_WORD_OFF + (NKU_KEY_LEN - 1) - iLoop];
		}

		/* for rsa ku; (Note:Soft RSA needs to switch word between big and small end !) */
		tcsmptr[SC_KEY_KU_WORD_OFF]
			= rsa_key[NKU_KUKEY_WORD_OFF + (NKU_KEY_LEN - 1)];

		f_rsa_public_decrypt(tcsmptr + SC_KEY_CODESIG_WORD_OFF,
				tcsmptr + SC_KEY_CODESIG_WORD_OFF,
				SC_KEY_CODESIG_WORD_SIZE,
				tcsmptr + SC_KEY_N_WORD_OFF,
				tcsmptr + SC_KEY_KU_WORD_OFF,
				SC_KEY_N_WORD_SIZE);
	}

	return 0;
}

static int start_scboot(void *input, void *output, struct sckey *sckey)
{
	struct sc_args *args = (struct sc_args *)(MCU_TCSM_SECALL_MSG);
	unsigned int *tcsmptr = (unsigned int *)(TCSM_CODE_ADDR);
	unsigned int ret;
	int iLoop = 0;
	int *srcptr = (int *)input;
	int *dstptr = (int *)output;

	if (!sckey) {
		serial_debug("ERROR: Invalid security information! %s:%d\n",__func__,__LINE__);
		return -1;
	}

	int binlen = sckey->code_len;
	int dmamode = sckey->pad_len > 0 ? 1 : 0;

	serial_debug("SCBOOT: input = 0x%x, output = 0x%x, mode = %s\n",
		     srcptr, dstptr, dmamode ? "cpu" : "dma");

	if (!dmamode) {
		int newround = 1;
		int endround = 0;
		int pos = 0;

		do {
			int lens = binlen > SC_MAX_SIZE_PERTIME ? SC_MAX_SIZE_PERTIME : binlen;

			if (binlen <= SC_MAX_SIZE_PERTIME)
				endround = 1;

			args->arg[0] = endround << 1 | newround;
			args->arg[1] = lens;

			for (iLoop = 0; iLoop < lens / 4; iLoop++)
				tcsmptr[iLoop] = srcptr[pos++];

			newround = 0;

			ret = secall(args, SC_FUNC_SCBOOT, 0, 1);

			for (iLoop = 0; iLoop < lens / 4; iLoop++)
				dstptr[pos - lens / 4 + iLoop] = tcsmptr[iLoop];

			binlen -= SC_MAX_SIZE_PERTIME;
		} while (!endround);

	} else {

		args->arg[0] = 1 | (1 << 1) | (1 << 2); //bit 0:newround bit 1:endround bit 2:dmamode
		args->arg[2] = virt_to_phys(srcptr);

		flush_cache_all();
		ret = secall(args, SC_FUNC_SCBOOT, 0, 1);
		flush_cache_all();

		int diff = dstptr - srcptr;
		if (diff == 0) {
		} else if (diff < 0) {
			memmove(dstptr, srcptr, binlen);
		} else if(diff > binlen) {
			memcpy(dstptr, srcptr, binlen);
		} else {
			serial_debug("SCBOOT: destination address error!\n");
			while(1);
		}

	}

	ret = *(volatile unsigned int *)MCU_TCSM_RETVAL;
	ret &= 0xFFFF;

	return ret;
}
#endif

void pdma_wait(void)
{
	__asm__ volatile (
		"	.set	push		\n\t"
		"	.set	noreorder	\n\t"
		"	.set	mips32		\n\t"
		"	li	$26, 0		\n\t"
		"	mtc0	$26, $12	\n\t"
		"	nop			\n\t"
		"1:				\n\t"
		"	wait			\n\t"
		"	b	1b		\n\t"
		"	nop			\n\t"
		"	.set	reorder		\n\t"
		"	.set	pop		\n\t"
		);
}

int secure_scboot(void *input, void *output)
{
	unsigned int ret = 0;
	unsigned int len;
	unsigned int *pdma_ins = (unsigned int *)pdma_wait;
	volatile unsigned int *pdma_bank0_off = (unsigned int *)TCSM_BANK0;
	int issig = 0;
	int tmp, i;
	struct sckey sckey = {0};

	/* start aes, pdma clk */
//	tmp = cpm_readl(CPM_CLKGR);
//	tmp &= ~(CPM_CLKGR_AES | CPM_CLKGR_PDMA);
//	cpm_writel(tmp, CPM_CLKGR);

	for (i = 0; i < 6; i++)
		pdma_bank0_off[i] = pdma_ins[i];

	boot_up_mcu();

	secure_check(input, &issig);

	if(EFUSTATE_SECBOOT_EN == 0) {
		if (issig == 0) {
			serial_debug("Normal boot...\n");
			return 0;
		} else {
			serial_debug("ERROR: check image size !!\n");
			return -1;
		}
	} else if (EFUSTATE_SECBOOT_EN) {
		if(issig == 1) {
			serial_debug("Security boot...\n");
			ret = setup_sckeys(input, &len, &sckey);
			if(ret) {
				serial_debug("ERROR: check image size, ret = %x !!\n", ret);
				return -1;
			}

			ret = start_scboot(input + SC_MAGIC_SIZE + SC_KEY_SIZE, output, &sckey);
			if(ret) {
				serial_debug("ERROR: check your image, ret = %x!!\n", ret);
				return -1;
			}
		} else {
			serial_debug("ERROR: sign your image !!\n");
			return -1;
		}
	}

	return ret;
}

int is_security_boot(void)
{
#define EFUSE_REG_STAT 0xb3480008
#define EFUSTATE_SECBOOT_EN_SFT (0x1 << 8)
	return *(volatile unsigned int *)(EFUSE_REG_STAT) & EFUSTATE_SECBOOT_EN_SFT;
}
