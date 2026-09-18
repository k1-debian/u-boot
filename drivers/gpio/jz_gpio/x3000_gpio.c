/*
 * Ingenic X3000 GPIO definitions
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

static struct jz_gpio_func_def uart_gpio_func[] = {
	/* UART0: PD01 RX, PD02 TX, function 0. */
	[0] = { .port = GPIO_PORT_D, .func = GPIO_FUNC_0 | GPIO_PULLUP, .pins = 0x3 << 1 },

	/* UART1: PC25 TX, PC26 RX, function 0. */
	[1] = { .port = GPIO_PORT_C, .func = GPIO_FUNC_0 | GPIO_PULLUP, .pins = 0x3 << 25 },

	/* UART2: PC21 TX, PC22 RX, function 0. */
	[2] = { .port = GPIO_PORT_C, .func = GPIO_FUNC_0 | GPIO_PULLUP, .pins = 0x3 << 21 },

	/* UART3: PC18 TX, PC19 RX, function 0. */
	[3] = { .port = GPIO_PORT_C, .func = GPIO_FUNC_0 | GPIO_PULLUP, .pins = 0x3 << 18 },

#ifdef CONFIG_SYS_UART4_PB22
	/* UART4: PB22 TX, PB23 RX, function 3. */
	[4] = { .port = GPIO_PORT_B, .func = GPIO_FUNC_3 | GPIO_PULLUP, .pins = 0x3 << 22 },
#else
	/* UART4: PB29 TX, PB30 RX, function 0. */
	[4] = { .port = GPIO_PORT_B, .func = GPIO_FUNC_0 | GPIO_PULLUP, .pins = 0x3 << 29 },
#endif

#ifdef CONFIG_SYS_UART5_PB20
	/* UART5: PB20 TX, PB21 RX, function 3. */
	[5] = { .port = GPIO_PORT_B, .func = GPIO_FUNC_3 | GPIO_PULLUP, .pins = 0x3 << 20 },
#else
	/* UART5: PB24 TX, PB25 RX, function 1. */
	[5] = { .port = GPIO_PORT_B, .func = GPIO_FUNC_1 | GPIO_PULLUP, .pins = 0x3 << 24 },
#endif

#ifdef CONFIG_SYS_UART6_PE
	/* UART6: PE20 RX, PE21 TX, function 1. */
	[6] = { .port = GPIO_PORT_E, .func = GPIO_FUNC_1 | GPIO_PULLUP, .pins = 0x3 << 20 },
#else
	/* UART6: PD00 RX, PD03 TX, function 2. */
	[6] = { .port = GPIO_PORT_D, .func = GPIO_FUNC_2 | GPIO_PULLUP,
		.pins = (0x1 << 0) | (0x1 << 3) },
#endif

	/* UART7: PA12 TX, PA13 RX, function 1. */
	[7] = { .port = GPIO_PORT_A, .func = GPIO_FUNC_1 | GPIO_PULLUP, .pins = 0x3 << 12 },
};

static struct jz_gpio_func_def gpio_func[] = {
#if defined(CONFIG_JZ_MMC_MSC0_PD)
	/* MSC0: PD04-PD09, function 0. */
	{ .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = 0x3f << 4 },
#endif

#if defined(CONFIG_JZ_MMC_MSC1_PD)
	/* MSC1: PD14-PD19, function 0. */
	{ .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = 0x3f << 14 },
#endif

#if defined(CONFIG_JZ_MMC_MSC2_PD)
	/* MSC2: PD20-PD25, function 0. */
	{ .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = 0x3f << 20 },
#endif

#if defined(CONFIG_JZ_SFC_PD)
	/* SFC0: PD26-PD31, function 0. */
	{ .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = 0x3f << 26 },
#endif
};
