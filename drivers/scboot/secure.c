#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>

#include "secall.h"
#include "pdma.h"

#include "secure.h"

#ifndef CONFIG_X2000_FPGA
static void pdma_wait(void)
{
	__asm__ volatile (
		".set noreorder\n\t"
		".set mips32\n\t"
		"li $26, 0          \n\t"
		"mtc0   $26, $12        \n\t"
		"nop                \n\t"
		"1:             \n\t"
		"wait               \n\t"
		"b  1b          \n\t"
		"nop                \n\t"
		".set reorder           \n\t"
		);
}

static void load_pdma_firmware(void)
{
	int i;
	unsigned int *pdma_ins = (unsigned int *)pdma_wait;
	unsigned int *dst_ptr = (unsigned int *)(TCSM_BANK0);//cacheable

//	printf("xxxxxxxx load pdma firmware!\n");
	for(i=0; i < 6; i++)
		dst_ptr[i] = pdma_ins[i];
}
#else
extern int load_serom_firmware(struct pdma_message *pdma_msg);
extern void load_pdma_firmware();
#endif

int init_seboot(void)
{
	//reset mcu
	volatile struct sc_args *args;
	args = (volatile struct sc_args *)GET_SC_ARGS();
#ifdef CONFIG_X2000_FPGA
	volatile struct pdma_message *pdma_msg;
	pdma_msg = (volatile struct pdma_message *)GET_PDMA_MESSAGE();
#else
	volatile unsigned int * clkgate = (volatile unsigned int *)0xb0000020;
	*clkgate = 0;
#endif
	reset_mcu();
	printf("reset_mcu %x\n", REG32(PDMA_BASE + DMCS_OFF));
	printf("MCU_TCSM_RETVAL 0x%08x\n", REG32(MCU_TCSM_RETVAL));

	load_pdma_firmware();
	boot_up_mcu();
	udelay(50 * 1000);
#ifdef CONFIG_X2000_FPGA
	load_serom_firmware(pdma_msg);
#else
	otp_init();
#endif

//	printf("mcu control status: %x\n", REG32(PDMA_BASE + DMCS_OFF));

	return 0;
}
