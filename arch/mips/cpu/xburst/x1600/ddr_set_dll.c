/*
 * DDR driver for Synopsys DWC DDR PHY.
 * Used by Jz4775, JZ4780...
 *
 * Copyright (C) 2013 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <ykli@ingenic.cn>
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

/* #define DEBUG */
#include <config.h>
#include <common.h>
#include <ddr/ddr_common.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>

#ifndef CONFIG_FPGA
void reset_dll(void)
{
/*
 * WARNING: 2015-01-08
 * 	DDR CLK GATE(CPM_DRCG 0xB00000D0), BIT6 must set to 1 (or 0x40).
 * 	If clear BIT6, chip memory will not stable, gpu hang occur.
 */

//	serial_debug("----------CPM_DRCG: %x, CPM_DRCG: %x\n", readl(0xb0000000 + CPM_DRCG), CPM_DRCG);
//	writel(3 | (1<<6), CPM_DRCG);
//	mdelay(5);
//	writel(0x7d | (1<<6), CPM_DRCG);
//	mdelay(5);
//	serial_debug("----------CPM_DRCG: %x\n", readl(CPM_DRCG));

	cpm_outl(0x73 | (1 << 6) , CPM_DRCG);
	mdelay(1);
	cpm_outl(0x71 | (1 << 6), CPM_DRCG);
	mdelay(1);
}
#endif
