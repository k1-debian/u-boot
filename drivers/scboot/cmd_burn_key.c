/*
 * Ingenic sc test command
 *
 * Copyright (c) 2013 pzqi <aric.pzqi@ingenic.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include <asm/io.h>
#include "sc.h"
#include "otp.h"
#include "jz_pdma.h"
#include "secall.h"

static void bitcpy(const unsigned int *s,unsigned int *d,
			        const int ss,const int ds,int bsz)
{
	int ss_int = ss / 32;
	int ss_bit = ss % 32;

	int ds_int = ds / 32;
	int ds_bit = ds % 32;
#define MMIN(a,b) (a) > (b) ? (b) : (a)
	while(bsz != 0){
		unsigned int src,dst,tmp,bmsk,smsk,dmsk;
		int min = MMIN(32 - ss_bit,32 - ds_bit);
		min = MMIN(min,bsz);
		bmsk = 0xffffffff >> (32 - min);
		src = s[ss_int] >> ss_bit;
		src &= bmsk;

		dst = d[ds_int];
		dst &= ~(bmsk << ds_bit);
		dst |= src << ds_bit;

		d[ds_int] = dst;
		ds_bit += min;
		if(ds_bit >= 32){
			ds_int++;
			ds_bit = 0;

		}
		ss_bit += min;
		if(ss_bit >= 32){
			ss_int++;
			ss_bit = 0;

		}
		//              printf("bsz = %d min = %d\n",bsz,min);
		bsz -= min;

	}
}

static int decode(unsigned int *s,int bits,unsigned int *d)
{
	int i,p,j;
	int xor = 0;
	i = 0;
	p = 0;
	j = 0;
	while(j < bits){
		int curindex,bsz;
		curindex = (1 << p);
		if(j + 1 != curindex){
			bsz = (curindex - j - 1);
			if((bits - j) < bsz){
				bsz = bits - j;

			}
			bitcpy(s,d,j,i,bsz);
			i += bsz;
			j += bsz;
		}else{
			j++;
			p++;
		}
	}

	return i;

}

static unsigned int ckey[8] = {0};
static int read_ckey()
{
	unsigned int ret, i;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *output = (volatile unsigned int *)(MCU_TCSM_OUTDATA);

//	gpio_output_value(AVDD_EFUSE_GPIO, 0);
	args->arg[0] = SC_OTP_SEL_CKEY;
	args->arg[1] = MCU_TCSM_PADDR(output);
	ret = secall(args,SC_FUNC_SCOTP,0,1);
	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		fprintf(stderr, "%s failed\n", __func__);
		return -1;
	}

	printf("key setted(actual): %08x-%08x-%08x-%08x\n", output[0], output[1], output[2], output[3]);
	printf("                    %08x-%08x-%08x-%08x-%08x\n", output[4], output[5], output[6], output[7], output[8]);
	decode(output, 34 * 8, ckey);

	printf("Ckey:\n");
	for (i = 0; i < 8; i++) {
		printf("%04x ", ckey[i]);
	}

	printf("Ckey:end\n");

	return 0;
}

static unsigned int userkey[8] = {0};
static unsigned int userkey1[8] = {0};

static int read_ukey(int ukey_flag, unsigned int *ukey)
{
	unsigned int ret, i;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *output = (volatile unsigned int *)(MCU_TCSM_OUTDATA);
	unsigned int tmp[8];

//	gpio_output_value(AVDD_EFUSE_GPIO, 0);
	args->arg[0] = ukey_flag;
	args->arg[1] = MCU_TCSM_PADDR(output);
	ret = secall(args,SC_FUNC_SCOTP,0,1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		fprintf(stderr, "%s failed\n", __func__);
		return -1;
	}

	printf("key setted(actual): %08x-%08x-%08x-%08x\n", output[0], output[1], output[2], output[3]);
	printf("                    %08x-%08x-%08x-%08x-%08x\n", output[4], output[5], output[6], output[7], output[8]);

	if(ukey_flag == SC_OTP_SEL_UKEY) {
		unsigned char *tmp1 = (unsigned char *)output;
		memcpy(output, &tmp1[2], 34);
	}
	printf("key setted(actual): %08x-%08x-%08x-%08x\n", output[0], output[1], output[2], output[3]);
	printf("                    %08x-%08x-%08x-%08x-%08x\n", output[4], output[5], output[6], output[7], output[8]);
	decode(output, 34 * 8, ukey);

	printf("ukey:\n");

	for (i = 0; i < 8; i++) {
		printf("%04x\n", ukey[i]);

	}

	printf("ukey end\n");

	return 0;
}

static unsigned int nkusig[8] = {0};

static int read_nkusig()
{
	unsigned int ret, i;
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	volatile unsigned int *output = (volatile unsigned int *)(MCU_TCSM_OUTDATA);
	*((volatile unsigned int *)(MCU_TCSM_OUTDATA)) = 1;
	unsigned int tmp[8];

//	gpio_output_value(AVDD_EFUSE_GPIO, 0);
	args->arg[0] = SC_OTP_SEL_NKU;
	args->arg[1] = MCU_TCSM_PADDR(output);
	ret = secall(args, SC_FUNC_SCOTP, 0,1);

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		fprintf(stderr, "%s failed\n", __func__);
		return -1;
	}

	printf("ukusig _burn(actual): %08x-%08x-%08x-%08x\n", output[0], output[1], output[2], output[3]);
	printf("                    %08x-%08x-%08x-%08x-%08x\n", output[4], output[5], output[6], output[7], output[8]);

	unsigned char *tmp1 = (unsigned char *)output;
	memcpy(output, &tmp1[2], 34);
	printf("ukusig _burn(actual): %08x-%08x-%08x-%08x\n", output[0], output[1], output[2], output[3]);
	printf("                    %08x-%08x-%08x-%08x-%08x\n", output[4], output[5], output[6], output[7], output[8]);
	decode(output, 34 * 8, nkusig);

	printf("nkusig:\n");
	for (i = 0; i < 8; i++) {
		printf("%04x ", nkusig[i]);

	}

	printf("nkusig end\n");

	return 0;
}


static unsigned int ukey[16] = {
	   0x81e82f09, 0xa5286870, 0x2510b9b8, 0x5c6e1987,
	   0x015c7167, 0x577643c8, 0x3a5754bf, 0xd4966ce8,
	   0x81647150, 0x6b6312e5, 0x024503c5, 0x20815f6b,
	   0x40eed4fc, 0x9e00efde, 0x2e3b7bdc, 0x509d22d1
};


static unsigned int nku[64 + 64 + 2] = {0x40, 0x40,
	0x99fa5ca4, 0x9316fe89, 0xa4aa3733, 0x0d853017,
	0xece38063, 0xf5e41225, 0x85b08a07, 0x88909b78,
	0xd9e000af, 0xe221dcec, 0xad3fa84e, 0xe20118c8,
	0xa69010ca, 0x4d8cca9f, 0x2dddd6cc, 0xbabbeb4d,
	0x9a1eeef3, 0x97ea7e0f, 0x35940c4f, 0x491f29c9,
	0x2e6166c0, 0xa79c7d21, 0xb760d306, 0x108e55a8,
	0x394878c5, 0x051bc7e6, 0xc347d18e, 0xe5432d5d,
	0xe84db60c, 0x439e7546, 0x264667be, 0x2941cceb,
	0x6627963b, 0xb0713bda, 0xb68a67e8, 0x87265e54,
	0xd54a6e64, 0xa3ea0b8e, 0xf90dc88d, 0x69981570,
	0x59379b8e, 0x06d738fe, 0xd91a82a1, 0x54332043,
	0xac6227d7, 0x206bea00, 0xddcc6a1f, 0x940bd914,
	0x1abf3aa0, 0x377532ca, 0x3a68f0a7, 0xd6ccc2e4,
	0xdf3e556d, 0x7c6186f7, 0x9b819e9a, 0xb8fc5dd7,
	0x7618df55, 0x8f826563, 0x84bb2228, 0xc2551c1d,
	0xcedc2d6f, 0x34fb3f09, 0xf9e7538c, 0x8d1799f7,

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
	0x00000000, 0x00000000, 0x00000000, 0x01000100
};

static int hash(const void *in, void *out, const size_t len)
{
	int index = 0;
	unsigned int *in_s = in;
	unsigned int *out_s = out;
	volatile unsigned int *in_t = (volatile unsigned int *)(MCU_TCSM_INDATA);
	volatile unsigned int *out_t = (volatile unsigned int *)(MCU_TCSM_OUTDATA);
	volatile struct sc_args *args = (volatile struct sc_args *)GET_SC_ARGS();
	for (index = 0; index < len; index++) {
		in_t[index] = in_s[index];
	//	printf("%x\n", in_t[index]);
	}

	args->arg[0] = len | 0x01 << 16 | 0x01 << 18 | 0x03 << 19/*hash 256*/;
	args->arg[1] = MCU_TCSM_PADDR(in_t);
	args->arg[2] = MCU_TCSM_PADDR(out_t);

	secall(args, SC_FUNC_HASH, 0,1);
	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		fprintf(stderr, "%s failed\n", __func__);
		return -1;
	}

	for (index = 0; index < 8; index++)
		out_s[index] = out_t[index];

	printf("hash\n");
	for (index = 0; index < 256 / 8 / 4; index++) {
		if (index != 0 && index % 4 == 0)
			printf("\n");

		printf("%x ", out_s[index]);
	}

	printf("\nhash end\n");
	return 0;
}

static int aes(const void *key, const void *in, void *out, const size_t len)
{
	int index = 0;
	unsigned int *in_s = in;
	unsigned int *out_s = out;
	unsigned int *key_s = key;
	volatile unsigned int *key_t = (volatile unsigned int *)(MCU_TCSM_INDATA);
	volatile struct sc_args *args = (volatile struct sc_args *)GET_SC_ARGS();
	for (index = 0; index < 8; index++) {
		key_t[index] = key_s[index];
		printf("%x\n", key_t[index]);
	}

	args->arg[0] =  0x01 << 1 /*dma*/ | 0x02 << 12;
	args->arg[1] = MCU_TCSM_PADDR(key_t);
	args->arg[2] = virt_to_phys(in_s);
	args->arg[3] = virt_to_phys(out_s);
	args->arg[4] = len;
	args->arg[6]  = 0x01 | 0x01 << 1;

	flush_cache_all();
	secall(args, SC_FUNC_AES, 0, 1);
	flush_cache_all();

	if (*(volatile unsigned int *)(MCU_TCSM_RETVAL) != SC_ERR_SUCC) {
		fprintf(stderr, "%s failed\n", __func__);
		return -1;
	}

	printf("aes\n");
	for (index = 0; index < len; index++) {
		if (index != 0 && index % 4 == 0)
			printf("\n");

		printf("%x ", out_s[index]);
	}

	printf("\naes end\n");
	return 0;
}
static unsigned int serom_code[] = {
         #include "./mcu_sc.hex"
};

int load_serom_firmware(struct pdma_message *pdma_msg)
{
	int i;
	unsigned int *src_ptr = serom_code;
	unsigned int *dst_ptr = (unsigned int *)(TCSM_BANK1);//cacheable
	unsigned int *debug_ptr = (unsigned int *)(TCSM_BANK1);//cacheable

#define SEROM_ADDR_START        (0xf5000000)
	unsigned int serom_addr = SEROM_ADDR_START;
	unsigned int serom_end  = SEROM_ADDR_START + sizeof(serom_code);

#define FINISH(x) ((((x)) & 0x80000000) == 0x80000000)
#define RETURN(x) (((x)) & (~0x80000000))
#define TRANSFER_SIZE   (4096)

	while(serom_addr < serom_end) {
		dst_ptr = (unsigned int *)(TCSM_BANK1);//cacheable
		debug_ptr = (unsigned int *)(TCSM_BANK1);

		/* every 4096Bytes a time */
		for(i = 0; i < (TRANSFER_SIZE)/ 4;i++) {
			*dst_ptr++ = *src_ptr++;

		}

		reset_mcu();
		pdma_msg->msg_id = 0;
		pdma_msg->msg[0] = 0xf4001000;                          /*src: bank1 for mcu*/
		pdma_msg->msg[1] = serom_addr;                          /*dst: serom for mcu*/
		pdma_msg->msg[2] = TRANSFER_SIZE;                       /*cnt: size per time */
		pdma_msg->ret = 0;
		boot_up_mcu();

		printf("boot_up_mcu end\n");
		while (!FINISH(pdma_msg->ret)){
			udelay(1000*1000);
			printf("wait copy done!\n");

		}
		printf("FINISH ret end\n");

		if(RETURN(pdma_msg->ret) == 0) {
			printf("success.\n");

		}else{
			printf("fail!\n");
			return -1;

		}
                serom_addr += TRANSFER_SIZE;


	}
	printf("ok!\n");
	return 0;
}
static unsigned int pdma_code[] = {
         #include "./pdma.hex"

};
void load_pdma_firmware()
{
	int i;
	unsigned int *src_ptr = pdma_code;
	unsigned int *dst_ptr = (unsigned int *)(TCSM_BANK0);//cacheable

	printf("xxx load pdma firmware!\n");
	for(i=0; i < ARRAY_SIZE(pdma_code); i++)
		*dst_ptr++ = *src_ptr++;
}

static int init_seboot_t()
{
	unsigned int ret, i;
	volatile struct sc_args *args;
	volatile struct pdma_message *pdma_msg;
	args = (volatile struct sc_args *)GET_SC_ARGS();
	pdma_msg = (volatile struct pdma_message *)GET_PDMA_MESSAGE();
//	gpio_direction_output(33, 0);
	reset_mcu();
	load_pdma_firmware();
	boot_up_mcu();

	mdelay(30);

	if(load_serom_firmware(pdma_msg)) {
		printf("load serom firmware error!!!!!!\n");
		return -1;

	}

	return 0;
}
/*sc_test */
#define SC_OTP_SEL_UKEY 0x2
#define SC_OTP_SEL_UKEY1 0x4

unsigned int a = 0x12345678;
unsigned int test_hash[8] = {0};
unsigned int b[4] = {0x12345678,};
unsigned int c[4] = {0};
unsigned k[8] = {0x01};
unsigned int chipkey[8] = {0};
unsigned int ukey_en[8] = {0};
unsigned int ukey1_en[8] = {0};
unsigned int nkusig_en[8] = {0};
unsigned int nkusig_cmp[8] = {0};

static int otp_r()
{
	REG32(EFUSE_REG_CTRL) = (0x1a << EFUSE_REGOFF_CRTL_ADDR | 0x0 << EFUSE_REGOFF_CRTL_LENG);

	REG32(EFUSE_REG_CTRL) |= EFUSE_REG_CTRL_RDEN;

	while(!(REG32(EFUSE_REG_STAT) & EFUSE_REG_STAT_RDDONE));

	printf("REG32(EFUSE_REG_DAT1) = %x\n",REG32(EFUSE_REG_DAT1));
	return 0;
}

static int do_sct(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int index = 0;
	if(argc < 2) {
		return CMD_RET_USAGE;
	}

	if(strcmp(argv[1], "init") == 0) {
		if(init_seboot_t() < 0) {
			printf("init seboot fialed.\n");
			return 0;
		}

	} else if(strcmp(argv[1], "read_ckey") == 0) {
		if (read_ckey() < 0) {
			printf("read ckey failed\n");
			return 0;
		}
		if (hash(ckey, chipkey, 8) < 0) {
			printf("get chip key failed\n");
			return 0;
		}
	} else if(strcmp(argv[1], "read_ukey") == 0) {
		if (read_ukey(SC_OTP_SEL_UKEY, userkey) < 0) {
			printf("read_ukey failed\n");
			return 0;
		}
		if (read_ukey(SC_OTP_SEL_UKEY1, userkey1) < 0) {
			printf("read_ukey1 failed\n");
			return 0;
		}

	} else if(strcmp(argv[1], "read_nkusig") == 0) {
		if (read_nkusig() < 0) {
			printf("read nkusig failed\n");
			return 0;
		}

	} else if(strcmp(argv[1], "read_bootmode") == 0) {

		otp_r();

	} else if(strcmp(argv[1], "cmp_ukey") == 0) {
		if (aes(chipkey, ukey, ukey_en, 8) < 0) {
			printf("cmp_ukey failed\n");
			return 0;
		}

	} else if(strcmp(argv[1], "cmp_ukey1") == 0) {
		if (aes(chipkey, &ukey[8], ukey1_en, 8) < 0) {
			printf("cmp_ukey1 failed\n");
			return 0;
		}

	} else if(strcmp(argv[1], "cmp_nkusig") == 0) {
		if (hash(nku + 2, nkusig_cmp, 64 + 64) < 0) {
			printf("cmp_nkusig failed\n");
			return 0;
		}

		if (aes(chipkey, nkusig_cmp, nkusig_en, 8) < 0) {
			printf("cmp_nkusig aes failed\n");
			return 0;
		}

	} else if(strcmp(argv[1], "burn_ckey") == 0) {
		if (cpu_burn_rckey() < 0) {
			printf("burn_ckey failed\n");
			return 0;
		}
	} else if(strcmp(argv[1], "burn_ukey") == 0) {
		if (cpu_burn_ukey(ukey) < 0) {
			printf("burn_ukey failed\n");
			return 0;
		}

	} else if(strcmp(argv[1], "burn_nkusig") == 0) {
		if (cpu_burn_nku(nku, 520) < 0) {
			printf("burn_nkusig failed\n");
			return 0;
		}
	} else if(strcmp(argv[1], "burn_scen") == 0) {
		if (cpu_burn_secboot_enable() < 0) {
			printf("burn_sc_en failed\n");
			return 0;
		}
	} else if (strcmp(argv[1], "test") == 0) {
		//hash(&a, test_hash, 1);
		for (index = 0; index < 4; index++)
			printf("%x\n", b[index]);
		aes(k, b, c, 4);
	}else {
		printf("cmd error!!\n");
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(sct, 2, 1, do_sct,
	"Ingenic security test program",
	"sctest init -- load firmware to pdma and se-rom.\n"
	"sctest scboot -- test scboot function.\n"
	"sctest xxx	-- test to be add!!\n"
);
