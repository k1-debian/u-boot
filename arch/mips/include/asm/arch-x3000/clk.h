/*
 * Ingenic X3000 clock definitions.
 */
#ifndef __X3000_CLK_H__
#define __X3000_CLK_H__

#include <asm/arch/base.h>

enum clk_id {
	DDR,
	MACPHY,
	LCD,
	MSC,
	MSC0 = MSC,
	MSC1,
	MSC2,
	SFC,
	CGU_CNT,
	CPU = CGU_CNT,
	H2CLK,
	APLL,
	MPLL,
	EPLL,
	EXCLK,
	USBPHY,
};

struct clk_cgu_setting {
	unsigned int addr;
	unsigned int val;
	unsigned ce:8;
	unsigned busy:8;
	unsigned stop:8;
	unsigned sel_src:8;
	unsigned sel_val;
	unsigned int div_mask;
	unsigned int valid;
};

#define SRC_EOF -1

unsigned int clk_get_rate(int clk);
void clk_set_rate(int clk, unsigned long rate);
void clk_init(void);
void enable_uart_clk(void);
void clk_prepare(void);

enum otg_mode_t {
	OTG_MODE = 0,
	DEVICE_ONLY_MODE,
	HOST_ONLY_MODE,
};

#endif /* __X3000_CLK_H__ */
