#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <asm/reboot.h>

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

#ifdef CONFIG_X1000
#define TCSM_CODE_ADDR			(TCSM_BANK(1) + 0)
#define TCSM_SC_KEY_ADDR		(TCSM_BANK(1) + 2048)
#define MCU_TCSM_RETVAL			(TCSM_BANK(0) + 2048 + 1084) /* cal from sc_interface. */
#define MCU_TCSM_SECALL_MSG		(TCSM_BANK(0) + 2048 + 128) /* MCU_TCSM_SECALL_MSG */
#else
#define TCSM_CODE_ADDR			(TCSM_BANK(1) + 0)
#define TCSM_SC_KEY_ADDR		(TCSM_BANK(1) + 2048)
#define MCU_TCSM_RETVAL			(TCSM_BANK(0) + 2048 + 1076) /* cal from sc_interface. */
#define MCU_TCSM_SECALL_MSG		(TCSM_BANK(0) + 2048 + 128) /* MCU_TCSM_SECALL_MSG */
#endif

#define SC_MAX_SIZE_PERTIME		(2048)
#define SC_MAGIC_SIZE			(512)
#define SC_KEY_SIZE				(1536)

#define SECURE_SCBOOT_MAGIC		0x54424353

#define SPL_KENOFFSET    ((SC_MAGIC_SIZE + 16) / 4)
#define SPL_CENOFFSET    ((SC_MAGIC_SIZE + 4) / 4)
#define SPL_NLENOFFSET   ((SC_MAGIC_SIZE + 8) / 4)
#define SPL_ULENOFFSET   ((SC_MAGIC_SIZE + 12) / 4)
#define KN_OFFSET        (256)
#define KU_OFFSET        (KN_OFFSET + 256)
#define SCKEY_INFO_LEN   (256)

#define UBOOT_KENOFFSET  ((CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE + 16) / 4)
#define UBOOT_CENOFFSET  ((CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE + 4) / 4)
#define UBOOT_NLENOFFSET ((CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE + 8) / 4)
#define UBOOT_ULENOFFSET ((CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE + 12) / 4)
#define UBOOT_LENOFFSET  ((CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE) / 4)

#define IMAGE_START       0x0
#define SPL_SCKEY_START   0x00000200
#define UBOOT_SCKEY_START 0x00004200

#define CRC_POSITION        9		/* 9th bytes */
#define SPL_LENGTH_POSITION 12	/* 11th */

#define USERKEY_ENCRYPT  2
#define CHIPKEY_ENCRYPT  1

extern int sfc_nor_read(unsigned int from, unsigned int len, unsigned char *buf);

static void secure_check(void *addr, int *issig)
{
	int *ddrptr = (int *)(addr);
	int p = (unsigned long)SECURE_SCBOOT_MAGIC;

	if(*ddrptr++ == p) {
		*issig = 1;
	}
}

static int setup_sckeys(void *addr, unsigned int *len)
{
	volatile unsigned int *tcsmptr = (volatile unsigned int *)(TCSM_SC_KEY_ADDR);
	int iLoop = 0;
	int *ddrptr = (int *)(addr + SC_MAGIC_SIZE);

	/* 384 * 4 = 1536, sc_key */
	for (iLoop = 0; iLoop < SC_KEY_SIZE/4; iLoop++)
		tcsmptr[iLoop] = ddrptr[iLoop];

	*len = tcsmptr[0]; /* image length */

	/* len must 4 wrod align */
	if((*len) == 0 || (*len) % 16)
		return -1;

	return 0;
}

static int start_scboot(void *input, void *output, unsigned int binlen)
{
	volatile struct sc_args *args = (volatile struct sc_args *)(MCU_TCSM_SECALL_MSG);
	volatile unsigned int *tcsmptr = (volatile unsigned int *)(TCSM_CODE_ADDR);
	unsigned int ret;
	int iLoop = 0;
	int *srcptr = (int *)(input + SC_MAGIC_SIZE + SC_KEY_SIZE);
	int *dstptr = (int *)(output);

	{
		int newround = 1;
		int endround = 0;
		int pos = 0;

		boot_up_mcu();

		do {
			int lens = binlen > SC_MAX_SIZE_PERTIME ? SC_MAX_SIZE_PERTIME : binlen;

			if (binlen <= SC_MAX_SIZE_PERTIME)
				endround = 1;

			args->arg[0] = endround << 1 | newround;
			args->arg[1] = lens;

			for (iLoop = 0; iLoop < lens / 4; iLoop++)
				tcsmptr[iLoop] = srcptr[pos++];

			newround = 0;

			ret = secall(args, SC_FUNC_SCBOOT, 0);

			for (iLoop = 0; iLoop < lens / 4; iLoop++)
				dstptr[pos - lens / 4 + iLoop] = tcsmptr[iLoop];

			binlen -= SC_MAX_SIZE_PERTIME;
		} while (!endround);

		ret = *(volatile unsigned int *)MCU_TCSM_RETVAL;
		ret &= 0xFFFF;

	}
	return ret;
}

static void pdma_wait(void)
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

	/* start aes, pdma clk */
	tmp = cpm_readl(CPM_CLKGR);
	tmp &= ~(CPM_CLKGR_AES | CPM_CLKGR_PDMA);
	cpm_writel(tmp, CPM_CLKGR);

	for (i = 0; i < 6; i++)
		pdma_bank0_off[i] = pdma_ins[i];

	secure_check(input, &issig);

	if((EFUSTATE_SECBOOT_EN == 0) && (issig == 0)) {
		printf("Normal boot...\n");
	}
	else if(EFUSTATE_SECBOOT_EN && (issig == 0)) {
		printf("ERROR: please sign your image !!\n");
		while(1);
	}
	else if(issig == 1) {
		printf("Security boot...\n");
		ret = setup_sckeys(input, &len);
		if(ret) {
			printf("ERROR: please check image size !!\n");
		}

		ret = start_scboot(input, output, len);
		if(ret) {
			printf("ERROR: please check your image !!\n");
		}
	}

	return ret;
}

static void bin_aes(void *addr, int dataLen)
{
	volatile struct sc_args *args = (volatile struct sc_args *)(MCU_TCSM_SECALL_MSG);
	volatile unsigned int *input = (volatile unsigned int *)(MCU_TCSM_INDATA);
	volatile unsigned int *output = (volatile unsigned int *)(MCU_TCSM_OUTDATA);

	unsigned int ret;
	int iLoop = 0;
	int *srcptr = (int *)(addr);
	int *dstptr = (int *)(addr);

	int endround = 0;
	int pos = 0;
	int pos1 = 0;
	boot_up_mcu();
	do {
		memset(args, 0, sizeof(struct sc_args));
		int lens = dataLen > AES_ONETIME_MAX ? AES_ONETIME_MAX : dataLen;

		if (dataLen <= AES_ONETIME_MAX)
				endround = 1;

		args->arg[2] = MCU_TCSM_PADDR(input);
		args->arg[3] = MCU_TCSM_PADDR(output);
		args->arg[4] = lens;

		args->arg[0] = 0;
		args->arg[0] |= AES_BY_UKEY | AES_CRYPT;

		for (iLoop = 0; iLoop < lens / 4; iLoop++)
			input[iLoop] = srcptr[pos++];

		secall(args, SC_FUNC_AESBYKEY, 0);

		for (iLoop = 0; iLoop < lens / 4; iLoop++)
			dstptr[pos - lens / 4 + iLoop] = output[iLoop];

		args->arg[0] = 0;
		args->arg[0] |= AES_BY_CKEY;

		for (iLoop = 0; iLoop < lens / 4; iLoop++)
			input[iLoop] = dstptr[pos1++];

		secall(args, SC_FUNC_AESBYKEY, 0);

		for (iLoop = 0; iLoop < lens / 4; iLoop++)
			dstptr[pos1 - lens / 4 + iLoop] = output[iLoop];

		dataLen -= AES_ONETIME_MAX;
	}while (!endround);
}

void ckey_aes(void)
{
	char *read_buf = NULL;

	read_buf = (char *)malloc(SCKEY_INFO_LEN);
	memset(read_buf, 0 , SCKEY_INFO_LEN);

	sfc_nor_read(SPL_SCKEY_START, SCKEY_INFO_LEN, read_buf);

	int *spl_kencrypt = (int *)read_buf + 4;
	int *spl_cencrypt = (int *)read_buf + 1;
	int spl_len = *((int *)read_buf);
	int spl_nlen = *((int *)read_buf + 2);
	int spl_ulen = *((int *)read_buf + 3);

	memset(read_buf, 0 , SCKEY_INFO_LEN);
	sfc_nor_read(UBOOT_SCKEY_START, SCKEY_INFO_LEN, read_buf);

	int *uboot_kencrypt = (int *)read_buf + 4;
	int *uboot_cencrypt = (int *)read_buf + 1;
	int uboot_len = *((int *)read_buf);
	int uboot_nlen = *((int *)read_buf + 2);
	int uboot_ulen = *((int *)read_buf + 3);

	int image_len = uboot_len + CONFIG_UBOOT_OFFSET + SC_MAX_SIZE_PERTIME;

	if (!((*spl_kencrypt == USERKEY_ENCRYPT) || (*spl_cencrypt == USERKEY_ENCRYPT) ||
		(*uboot_kencrypt == USERKEY_ENCRYPT) || (*uboot_cencrypt == USERKEY_ENCRYPT)))
		return 0 ;

	read_buf = (char *)malloc(image_len);
	memset(read_buf, 0 , image_len);
	sfc_nor_read(IMAGE_START, image_len, read_buf);

	if (*spl_kencrypt == USERKEY_ENCRYPT) {
		spl_nlen = (spl_nlen / 8 + 15) & 0xFFFFFFF0;
		spl_ulen = (spl_ulen / 8 + 15) & 0xFFFFFFF0;
		bin_aes(read_buf + SC_MAGIC_SIZE + KN_OFFSET, spl_nlen);
		bin_aes(read_buf + SC_MAGIC_SIZE + KU_OFFSET, spl_ulen);
		*((int *)read_buf + SPL_KENOFFSET) = CHIPKEY_ENCRYPT;
	}

	if (*spl_cencrypt == USERKEY_ENCRYPT) {
		bin_aes(read_buf + SC_MAX_SIZE_PERTIME, spl_len);
		*((int *)read_buf + SPL_CENOFFSET) = CHIPKEY_ENCRYPT;

		u8 crc = sec_crc(read_buf + SC_MAX_SIZE_PERTIME, spl_len);
		memcpy(read_buf + CRC_POSITION, &crc, 1);
	}

	if (*uboot_kencrypt == USERKEY_ENCRYPT) {
		uboot_nlen = (uboot_nlen / 8 + 15) & 0xFFFFFFF0;
		uboot_ulen = (uboot_ulen / 8 + 15) & 0xFFFFFFF0;
		bin_aes(read_buf + CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE + KN_OFFSET, uboot_nlen);
		bin_aes(read_buf + CONFIG_UBOOT_OFFSET + SC_MAGIC_SIZE + KU_OFFSET, uboot_ulen);
		*((int *)read_buf + UBOOT_KENOFFSET) = CHIPKEY_ENCRYPT;
	}

	if (*uboot_cencrypt == USERKEY_ENCRYPT) {
		bin_aes(read_buf + CONFIG_UBOOT_OFFSET + SC_MAX_SIZE_PERTIME, uboot_len);
		*((int *)read_buf + UBOOT_CENOFFSET) = CHIPKEY_ENCRYPT;
	}

	if (sfc_nor_erase(IMAGE_START, image_len)) {
		printf("sfcnor erase err!\n");
		_machine_restart();
	}

	sfc_nor_write(IMAGE_START, image_len, read_buf);

	sfc_nor_read(SPL_SCKEY_START, SCKEY_INFO_LEN, read_buf);
	spl_kencrypt = (int *)read_buf + 4;
	spl_cencrypt = (int *)read_buf + 1;

	if((*spl_cencrypt == 1) && (*spl_kencrypt == 1));
		printf("Spl chipkey aes success\n");
	_machine_restart();
}
