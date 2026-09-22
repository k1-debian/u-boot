#ifndef __DDR_REG_VALUES_H__
#define __DDR_REG_VALUES_H__

#include "ddr_param.h"

#if defined(CONFIG_DDR_TYPE_DDR3)

/*
 * DDR3 parameter block captured from ddr3.txt.
 *
 * Flow-control/status registers such as SWCTL, PCTRL, and the PHY training
 * update register are handled by the initialization code. Snapshot values
 * without fields in struct ddr_param:
 *   DDRC_INIT1=0x00000000, DDRC_INIT0=0x00030003,
 *   DDRC_SWCTLSTATIC=0x00000001, DDRC_SWCTL=0x00000000,
 *   DDRC_SCHED=0x00002000, DDRC_PCTRL0..8=0x00000001,
 *   PHY train_reg_update=0xffff59af, PHY trfc=0x30c02c82,
 *   PHY auto-result/delay registers.
 *
 * This object is static because this header may be included by more than one
 * translation unit. Assign g_ddr_param to its address from the board-specific
 * initialization code when this parameter block is selected.
 */
static struct ddr_param ddr_reg_values = {
	.h = {
		.name = "x3000-ddr3-snapshot",
		.id = 0x00000001,
		.type = DDR3,
		.freq = 20000000, /* ddr3.txt: DDR clk rate 0x01312d00 */
	},

	.ddrc = {
		.MSTR_VALUE = 0x01040001,
		/* MSTR1/MSTR2 are not dumped; use their controller reset defaults. */
		.DRAMTMG_VALUE = {
			[0] = 0x0c101b0e, [1] = 0x00020213,
			[2] = 0x0000070c, [3] = 0x00002006,
			[4] = 0x05020205, [5] = 0x04040302,
			[8] = 0x00000802,
		},
		.RFSHCTL0_VALUE = 0x00000000,
		.RFSHCTL1_VALUE = 0x00000000,
		.RFSHCTL2_VALUE = 0x00000000,
		.RFSHCTL3_VALUE = 0x00000002,
		.RFSHCTL4_VALUE = 0x00000000,
		.RFSHTMG_VALUE = 0x00070003,
		.RFSHTMG1_VALUE = 0x00000000,
		.DFITMG0_VALUE = 0x06010001,
		.DFITMG1_VALUE = 0x00080307,
		.DFIMISC_VALUE = 0x00000061,
		.DFIUPD0_VALUE = 0x80400003,
		.DFIUPD1_VALUE = 0x00000000,
		.ODTCFG_VALUE = 0x00000000,
		.ODTMAP_VALUE = 0x00000000,
		.PWRCTL_VALUE = 0x00000000,
		.PWRTMG_VALUE = 0x00000000,
		.ZQCTL0_VALUE = 0x00000000,
		.ZQCTL1_VALUE = 0x00000000,
		.ADDRMAP_VALUE = {
			[0] = 0x0000001f, [1] = 0x00171717,
			[2] = 0x00000000, [3] = 0x00000000,
			[4] = 0x00001f1f, [5] = 0x04040404,
			[6] = 0x0f040404,
		},
		.INIT3_VALUE = 0x02100001,
		.INIT4_VALUE = 0x00000000,
		.INIT5_VALUE = 0x00000000,
		.INIT6_VALUE = 0x00000000,
		.INIT7_VALUE = 0x00000000,
	},

	.ddrp = {
		.rank_num = 1,
		.MEMCFG_VALUE = 0x000003a7,
		.AL_VALUE = 0x00000000,
		.CL_VALUE = 0x05050000,
		.CWL_VALUE = 0x05050000,
		.VREF_VALUE = { 0x00000000, 0x00000000,
				0x00000000, 0x00000000 },
	},

	.dram = {
		.MR7_VALUE = 0x00000000,
		.MR63_VALUE = 0x00000000,
		.CHIP_0_SIZE = 0,
		.CHIP_1_SIZE = 0,
	},

	.drvodt = {
		.use_drvodt_config = 0,
	},

	.deskew = {
		.use_deskew = 0,
	},

	.pinmap = {
		.use_pinmap = 0,
	},
};

#elif defined(CONFIG_DDR_TYPE_DDR4)

/*
 * DDR4 parameter block captured from the X3000 bring-up configuration.
 *
 * The register fields present in the supplied bring-up snapshot are filled
 * below.  Flow-control/status registers such as SWCTL, PCTRL, and the PHY
 * training update register are handled by the initialization code.
 *
 * Snapshot values without fields in struct ddr_param:
 *   DDRC_INIT1=0x00000000, DDRC_INIT0=0x00030003,
 *   DDRC_SWCTLSTATIC=0x00000001, DDRC_SWCTL=0x00000000,
 *   DDRC_SCHED=0x00002000, DDRC_PCTRL0..8=0x00000001,
 *   PHY train_reg_update=0xffff59af, PHY trfc=0x25808c82,
 *   PHY MEMCFG before setup=0x0001ff87.
 *
 * This object is static because this header may be included by more than one
 * translation unit.  Assign g_ddr_param to its address from the board-specific
 * initialization code when this parameter block is selected.
 */
static struct ddr_param ddr_reg_values = {
	.h = {
		.name = "W664GG6RB-DDR4-3200-4Gb-x16",
		.id = 0,
		.type = 0,
		.freq = 400000000,	/* DDR 时钟（DDR 域）= 400 MHz。本 IP 的 DFI:DDR 恒为 1:2，
					 * 所以 DFI 1x 时钟（控制器/PCLK 域）= 此值/2 = 200 MHz */
		.bus_width = 16,
	},
	.ddrc = {
		.MSTR_VALUE = 0x81040010,
		.DRAMTMG_VALUE = {
			[ 0] = 0x0a0e0d07,	/* DRAMTMG0  0x100 */
			[ 1] = 0x0002050b,	/* DRAMTMG1  0x104 */
			[ 2] = 0x05060409,	/* DRAMTMG2  0x108 */
			[ 3] = 0x0000400c,	/* DRAMTMG3  0x10c */
			[ 4] = 0x03030204,	/* DRAMTMG4  0x110 */
			[ 5] = 0x03030202,	/* DRAMTMG5  0x114 */
			[ 6] = 0x01010000,	/* DRAMTMG6  0x118 */
			[ 7] = 0x00000101,	/* DRAMTMG7  0x11c */
			[ 8] = 0x0a0a0a0a,	/* DRAMTMG8  0x120 */
			[ 9] = 0x00020208,	/* DRAMTMG9  0x124 */
			[10] = 0x00000000,	/* DRAMTMG10  0x128 */
			[11] = 0x00000000,	/* DRAMTMG11  0x12c */
			[12] = 0x00000000,	/* DRAMTMG12  0x130 */
			[13] = 0x00000000,	/* DRAMTMG13  0x134 */
			[14] = 0x0000012b,	/* DRAMTMG14  0x138 */
			[15] = 0x00000000,	/* DRAMTMG15  0x13c */
			[16] = 0x00000000,	/* DRAMTMG16  0x140 */
			[17] = 0x00000000,	/* DRAMTMG17  0x144 */
		},
		.RFSHCTL0_VALUE = 0x00000010,
		.RFSHCTL1_VALUE = 0x00000000,
		.RFSHCTL2_VALUE = 0x00000000,
		.RFSHCTL3_VALUE = 0x00000000,
		.RFSHCTL4_VALUE = 0x00000000,
		.RFSHTMG_VALUE = 0x00300034,
		.RFSHTMG1_VALUE = 0x008c008c,
		.DFITMG0_VALUE = 0x06040003,
		.DFITMG1_VALUE = 0x00010404,
		.DFIMISC_VALUE = 0x00000001,
		.DFIUPD0_VALUE = 0x00400003,
		.DFIUPD1_VALUE = 0x00010001,
		.ODTCFG_VALUE = 0x06000608,
		.ODTMAP_VALUE = 0x00000011,
		.PWRCTL_VALUE = 0x00000000,
		.PWRTMG_VALUE = 0x00010010,
		.ZQCTL0_VALUE = 0x00400010,
		.ZQCTL1_VALUE = 0x02000100,
		.DBICTL_VALUE = 0x00000001,	/* DM 模式   DM_EN=1 WR_DBI=0 RD_DBI=0，须与 INIT6 的 MR5[10:12] 一致 */
		.DFILPCFG0_VALUE = 0x00000000,	/* PD_EN=0 SR_EN=0 DPD_EN=0 */
		.DFILPCFG1_VALUE = 0x00000000,	/* MPSM_EN=0 */
		.ADDRMAP_VALUE = {
			[ 0] = 0x0000001f,	/* ADDRMAP0  0x200 */
			[ 1] = 0x003f1717,	/* ADDRMAP1  0x204 */
			[ 2] = 0x00000000,	/* ADDRMAP2  0x208 */
			[ 3] = 0x00000000,	/* ADDRMAP3  0x20c */
			[ 4] = 0x00001f1f,	/* ADDRMAP4  0x210 */
			[ 5] = 0x04040404,	/* ADDRMAP5  0x214 */
			[ 6] = 0x0f040404,	/* ADDRMAP6  0x218 */
			[ 7] = 0x00000f0f,	/* ADDRMAP7  0x21c */
			[ 8] = 0x00003f19,	/* ADDRMAP8  0x220 */
			[ 9] = 0x00000000,	/* ADDRMAP9  0x224 */
			[10] = 0x00000000,	/* ADDRMAP10  0x228 */
			[11] = 0x00000000,	/* ADDRMAP11  0x22c */
		},
		.INIT3_VALUE = 0x01100301,	/* MR0=0x0110 (CL/WR/DLL)  MR1=0x0301 (DLL/DS/RTT_NOM) */
		.INIT4_VALUE = 0x00000000,	/* MR2=0x0000 (CWL/RTT_WR)  MR3=0x0000 */
		.INIT5_VALUE = 0x00020000,	/* dev_zqinit_x32=2（tZQOPER/32） */
		.INIT6_VALUE = 0x00000400,	/* MR4=0x0000 (Preamble/CS2CMD Latency)  MR5=0x0400 (RTT_PARK/DM/DBI) */
		.INIT7_VALUE = 0x00000019,	/* MR6=0x0019 (VrefDQ) */
	},
	.ddrp = {
		.rank_num = 1,
		.MEMCFG_VALUE = 0x000603c7,
		.AL_VALUE = 0x00000000,
		.CL_VALUE = 0x0b0b0b0b,
		.CWL_VALUE = 0x09090909,
		.VREF_VALUE = { 0, 0 },
	},
	.dram = {
		.MR7_VALUE = 0x00000000,
		.MR63_VALUE = 0x00000000,
		.CHIP_0_SIZE = 512,
		.CHIP_1_SIZE = 0,
	},
	.drvodt = {
		/* 阻抗配置。**两组值都填**：
		 *   控制位（5-bit）= 生成期按 Table 22 从下面的欧姆换算来的，
		 *                    驱动启动时**原样 writel，零计算**（快路径）；
		 *   欧姆(Ω)        = 界面填的原值，给人核对 / 驱动兜底换算用。
		 * 控制位 = 0 表示该项不配置（驱动跳过，保持 PHY 复位默认）。
		 * 档位表见 PHY Table 22：23 25 26 27 28 30 32 34 36 38 41 45 49
		 * 55 60 68 79 90 108 135 178 270 535。*/
		.use_drvodt_config = 0,
		/* ---- DQ（Data）per-byte：byte0..1 = A_l(低)/A_h(高) ---- */
		.drv_pu  = { 0x1f, 0x1f },  /* Ω 535/535 */
		.drv_pd  = { 0x1f, 0x1f },  /* Ω 535/535 */
		.odt_pu  = { 0x1f, 0x1f},  /* Ω 535/535 */
		.odt_pd  = { 0x1f, 0x1f },  /* Ω 535/535 */
		.dq_drv_pu_ohm = { 535, 535 },
		.dq_drv_pd_ohm = { 535, 535 },
		.dq_odt_pu_ohm = { 535, 535 },
		.dq_odt_pd_ohm = { 535, 535 },
		/* ---- CMD（除 CK）：全局一份 ---- */
		.cmd_drv_pu = 0x1f,  /* Ω 535 */
		.cmd_drv_pd = 0x1f,  /* Ω 535 */
		.cmd_drv_pu_ohm = 535,
		.cmd_drv_pd_ohm = 535,
		/* ---- CK：**独立寄存器**（与 CMD 不共用）---- */
		.ck_drv_pu = 0x1f,  /* Ω 535 */
		.ck_drv_pd = 0x1f,  /* Ω 535 */
		.ck_drv_pu_ohm = 535,
		.ck_drv_pd_ohm = 535,
	},
	.dqsbp = { .use_dqs_bypass = 0 },
	.deskew = { .use_deskew = 0 },
	.pinmap = {
		/* 复位默认即一一映射：cmd_wrap[i]=i、dq_bit_wrap[b][j]=j、dm_bit_wrap[b]=8 */
		.use_pinmap = 0,
		.cmd_wrap = {
			0, 1, 2, 3, 4, 5, 6, 7,
			8, 9, 10, 11, 12, 13, 14, 15,
			16, 17, 18, 19, 20, 21, 22, 23,
			24, 25, 26, 27, 28, 29, 30,
		},
		.cke_ck_cmd_pad = 0x00008101,
		.byte_wrap = { 0, 1, 2, 3, 4, 5, 6, 7,
			8, },
		.dq_bit_wrap = {
			{ 2, 7, 1, 5, 4, 3, 6, 0, },
			{ 2, 3, 7, 0, 1, 6, 4, 5, },
		},
		.dm_bit_wrap = { 8, 8 },
		/* cat_wrap 是 LPDDR4 专用（2-bit，复位 0），DDR4 保持复位默认 */
		.cat_wrap = { 0, 0 },
	},
};


#else
#error "Select CONFIG_DDR_TYPE_DDR3 or CONFIG_DDR_TYPE_DDR4"
#endif

#endif /* __DDR_REG_VALUES_H__ */
