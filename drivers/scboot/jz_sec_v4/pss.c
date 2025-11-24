#include <common.h>
#include <asm/io.h>
#include "jz_pdma.h"
#include "secall.h"
#include "sc.h"


#define EVP_MAX_MD_SIZE 64 /* longest known is SHA512 */

#ifdef CONFIG_RSA3072
#define EVP_MD_SIZE          48
#define EVP_MD_WORD_SIZE     12
#define HASH_SELECT          HASH_SELECT_SHA384
#else
#define EVP_MD_SIZE          32
#define EVP_MD_WORD_SIZE     8
#define HASH_SELECT          HASH_SELECT_SHA256
#endif


static int cmp_data(unsigned int *src,unsigned int *dst,unsigned int word_len)
{
        unsigned int *start = src;
        unsigned int *end_src = src + word_len;
        while(start < end_src)
        {
                if(*start++ != *dst++)
                        return (start - src);
        }
        return 0;
}

void pss_hash(unsigned int *input, unsigned int len, unsigned int *output)
{
	volatile struct sc_args *args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *tcsm_in = (volatile unsigned int *)MCU_TCSM_INDATA;
	volatile unsigned int *tcsm_out = (volatile unsigned int *)MCU_TCSM_NKUSIG;
	int i;

	for (i = 0; i < len/4; i++) {
		tcsm_in[i] = input[i];
	}
        args->arg[0] = (len / 4) | HASH_NEWROUND | HASH_ENDROUND | HASH_SET(HASH_SELECT);
	args->arg[1] = MCU_TCSM_PADDR(tcsm_in);
        args->arg[2] = MCU_TCSM_PADDR(tcsm_out);

	secall(args, SC_FUNC_HASH, 0, 1);
	if (SC_RETVAL != SC_ERR_SUCC) {
		printf("pss hash failed, ret = %x\n",SC_RETVAL);
	}

	for (i = 0; i < 12; i++) {
		output[i] = tcsm_out[i];
	}
}


static void mgf1(unsigned char *dst, unsigned int dst_len, unsigned char *src, unsigned int src_len)
{
	unsigned char tmp[EVP_MAX_MD_SIZE] = {0}, tmp2[EVP_MAX_MD_SIZE];
	unsigned char *ctr;
	unsigned int mask_len, hash_len = src_len, i;
	memcpy(tmp, src, src_len);
	ctr = tmp + src_len;
	while((int)dst_len > 0) {
		pss_hash((unsigned int *)tmp, src_len + 4, (unsigned int *)tmp2);
		mask_len = dst_len < hash_len ? dst_len : hash_len;
		for(i = 0; i < mask_len; i++){
			*dst++ ^= tmp2[i];
		}
		dst_len -= mask_len;
		ctr[3]++;
	}
}

unsigned int sign_verify(int *paddr)
{
	unsigned int *mHash = (unsigned int *)paddr[1], selfmap[EVP_MAX_MD_SIZE], map1[EVP_MD_WORD_SIZE];
	unsigned char tmp[8 + EVP_MAX_MD_SIZE * 2]={0};
	unsigned char *map, *salt;
	unsigned int hLen = EVP_MD_SIZE, MSBits;
	unsigned int maskedDBLen, i = 0;
	int nlen = paddr[0];
	unsigned int emLen = paddr[0] * 4;
	unsigned char *EM = (unsigned char *)paddr[2];
	SC_RETVAL = SC_ERR_SUCC;

	MSBits = (nlen * 32 - 1) & 0x7;

	if (EM[emLen - 1] != 0xbc) {
		SC_RETVAL = SC_ERR_ILLEGAL_PSS;
		return SC_RETVAL;
	}

	maskedDBLen = emLen - 1 - hLen;
	map = EM + maskedDBLen;

	mgf1(EM, maskedDBLen, map, hLen);

	if (MSBits)
		EM[0] &= 0xFF >> (8 - MSBits);

	for(i = 0;EM[i] == 0 && i<(maskedDBLen - 1); i++);

	if (EM[i++] != 0x01){
		SC_RETVAL = SC_ERR_ILLEGAL_PSS;
		return SC_RETVAL;
	}

	salt = EM + i;

	memcpy((tmp + 8), mHash, hLen);
	memcpy((tmp + 8 + hLen), salt, (maskedDBLen - i));
	pss_hash((unsigned int *)tmp, 8 + hLen + (maskedDBLen -i), (unsigned int *)selfmap);

	for(i = 0; i < 32; i++) {
		((unsigned char *)map1)[i] = map[i];
	}

	if (cmp_data(map1, selfmap, EVP_MD_WORD_SIZE)) {
		SC_RETVAL = SC_ERR_ILLEGAL_PSS;
		return SC_RETVAL;
	}
	return SC_RETVAL;
}

