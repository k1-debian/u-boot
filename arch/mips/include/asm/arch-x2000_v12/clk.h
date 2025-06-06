/*
 * X2000 clock definitions
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co.,Ltd
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

#ifndef __CLK_H__
#define __CLK_H__

#include <asm/arch/base.h>

enum clk_id {
	DDR,
	MACPHY,
	MACTX0,
	MACTX1,
	MACPTP,
	I2S0,
	I2S1,
	I2S2,
	I2S3,
	LCD,
	MSC0,
	MSC1,
	MSC2,
	SFC,
	SSI,
	CIM,
	PWM,
	ISP,
	RSA,
	CGU_CNT,
	CPU = CGU_CNT,
	H2CLK,
	APLL,
	MPLL,
	EPLL,
	EXCLK,
};
struct clk_cgu_setting {
	unsigned int addr;
	unsigned int val;
	unsigned ce:8;
	unsigned busy:8;
	unsigned stop:8;
	unsigned sel_src:8;
	unsigned sel_val;
};
#define SRC_EOF -1

unsigned int clk_get_rate(int clk);
void clk_set_rate(int clk, unsigned long rate);
void clk_init(void);
void enable_uart_clk(void);
enum otg_mode_t {
	OTG_MODE = 0,
	DEVICE_ONLY_MODE,
	HOST_ONLY_MODE,
};
#endif /* __CLK_H__ */
