/*
 * X1000 efuse definitions
 *
 * Copyright (C) 2026 Ingenic Semiconductor Co.,Ltd
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

#ifndef __EFUSE_H__
#define __EFUSE_H__

/* efuse registers, relative to EFUSE_BASE (see asm/arch/base.h) */
#define EFUSE_CTRL		0x00
#define EFUSE_CFG		0x04
#define EFUSE_STATE		0x08
#define EFUSE_DATA(n)		(0x0c + (n) * 4)

/* efuse ctrl bits */
#define EFUSE_CTRL_ADDR		21
#define EFUSE_CTRL_ADDR_MASK	(0x1ff << EFUSE_CTRL_ADDR)
#define EFUSE_CTRL_LEN		16
#define EFUSE_CTRL_LEN_MASK	(0x1f << EFUSE_CTRL_LEN)
#define EFUSE_CTRL_RDEN		(1 << 0)

/* efuse cfg bits */
#define EFUSE_CFG_RD_ADJ	20
#define EFUSE_CFG_RD_ADJ_MASK	(0xf << EFUSE_CFG_RD_ADJ)
#define EFUSE_CFG_RD_STROBE	16
#define EFUSE_CFG_RD_STROBE_MASK (0xf << EFUSE_CFG_RD_STROBE)
#define EFUSE_CFG_WR_ADJ	12
#define EFUSE_CFG_WR_ADJ_MASK	(0xf << EFUSE_CFG_WR_ADJ)
#define EFUSE_CFG_WR_STROBE	0
#define EFUSE_CFG_WR_STROBE_MASK (0x1f << EFUSE_CFG_WR_STROBE)

/* efuse state bits */
#define EFUSE_STA_WR_DONE	(1 << 1)
#define EFUSE_STA_RD_DONE	(1 << 0)

/*
 * efuse rom segments, byte addresses in the efuse rom space.  The value
 * written into the EFUSE_CTRL address field is (segment address - EFUSE_ROM_BASE).
 */
#define EFUSE_ROM_BASE		0x200

#define EFUSE_CHIPID_ADDR	0x200
#define EFUSE_CHIPID_END	0x20f
#define EFUSE_CHIPID_BIT_NUM	128
#define EFUSE_TRIM1_ADDR	0x234
#define EFUSE_TRIM1_END         0x237
#define EFUSE_TRIM1_BIT_NUM	32
#define EFUSE_SOCID_ADDR	0x23c
#define EFUSE_SOCID_END         0x23d
#define EFUSE_SOCID_BIT_NUM	16
#define EFUSE_PRT_ADDR		0x23e
#define EFUSE_PRT_END		0x23f
#define EFUSE_PRT_BIT_NUM	16

#define EFUSE_PRT_MASK		0xffff

#define EFUSE_SOCID_MASK	0xffff

/* TRIM1 SLT version and independent DDR override flags. */
#define EFUSE_TRIM1_SLT_VERSION_MASK	0xff
#define EFUSE_TRIM1_SLT_ESMT_KGD	1
#define EFUSE_TRIM1_DDR_ODT_DISABLE	(1U << 8)
#define EFUSE_TRIM1_DDR_AUTOSR_DISABLE	(1U << 9)
#define EFUSE_TRIM1_DDR_DS0_TXP_6	(1U << 10)

/* socid segment values, see cpu/xburst/x1000/check_socid.c */
#define SOCID_X1000		0xff00
#define SOCID_X1000E		0xff01
#define SOCID_X1500		0xff02
#define SOCID_X1500L		0xff04
#define SOCID_X1501		0xff05
#define SOCID_X1000_NEW		0xff08
#define SOCID_X1000E_NEW	0xff09
#define SOCID_X1500_NEW		0xff0a

#endif /* __EFUSE_H__ */
