/*
 * ddr_param.h - DDR 公共参数协议（叶子头文件）
 *
 * 本文件是 DDR 参数协议载体：由 ddr_param_creator 工具（或烧录工具）按本
 * 结构体生成参数，DDR 驱动与工具均只依赖本头文件（无其他依赖）。
 * 驱动侧通过 `extern struct ddr_param *g_ddr_param;` 使用。
 *
 * 字段划分原则（工具生成 vs 驱动流程）：
 *   工具按 颗粒 datasheet / 目标频率 / 板级布线 计算出的值放这里；
 *   驱动侧的流程（复位时序、校准触发/轮询、MR 写入流程、训练调度）不放。
 * 字段值为对应寄存器的【整寄存器配置值】，驱动直接写入。
 */
#ifndef __DDR_PARAM_H__
#define __DDR_PARAM_H__


/* ============================ 参数头 ============================ */
struct ddr_param_header {
	unsigned char name[32];	/* DDR 名称/标识 */
	unsigned int  id;	/* DDR ID */
	unsigned int  type;	/* DDR 芯片类型（见 include/ddr/ddr_params.h 中的 enum ddr_type） */
	/*
	 * DDR **时钟频率**（Hz），指的是 DDR 域时钟。
	 * 本 IP 的 DFI:DDR 恒为 1:2（MSTR 无 frequency_ratio 字段），
	 * 所以 **DFI 1x 时钟 = freq / 2**（控制器/PCLK 域）。
	 */
	unsigned int  freq;

	/*
	 * 数据总线位宽：8 或 16。
	 * 决定 PHY 实际用几个 byte（8bit -> 只有 byte0；16bit -> byte0~1），
	 * 训练 / 结果打印 / pinmap 都按它决定遍历范围 —— **不要写死 2**。
	 * 与 ddrp.MEMCFG_VALUE 里的 channel_en 必须一致。
	 */
	unsigned int  bus_width;
};

/* ============================ PHY 模拟 tuning（板级调优） ============================ */
/* 驱动强度 / ODT：byte0..3 = A_l/A_h/B_l/B_h */
struct ddr_drvodt_config {
	unsigned int use_drvodt_config;	/* 0: 驱动默认值，1: 使用本结构体 */
	unsigned int drv_pu[4];		/* byte0..3 驱动 pull-up（ddrp_regs.h: *abutprcompdq） */
	unsigned int drv_pd[4];		/* byte0..3 驱动 pull-down（*abutnrcompdq） */
	unsigned int odt_pu[4];		/* byte0..3 ODT pull-up（*abutodtpudq） */
	unsigned int odt_pd[4];		/* byte0..3 ODT pull-down（*abutodtpddq） */
};

/* ==================== Rx-DQS Gating 手动（bypass）配置 ====================
 * 初期用软件 training 遍历 DQS gating 窗口时用：把 reg_calib_bypass 置 1，
 * 由软件直接给 gating 延迟，不再走 PHY 自动校准结果。
 * 依据：Databook v3p4 **6.2.3 Bypass Rx DQS Gating Training** + Table 45。
 *
 * 三个值与自动校准结果（ddrp_dump_rx_dqs_result 打的那三列）一一对应、
 * 单位相同：cyc_dly -> cycsel（1x）、oph_dly -> ophsel（0.5UI）、
 * dll_dly -> dllsel（4UI/256，即 delay line）。
 *
 * 注：lane 0..3 = A_l/A_h/B_l/B_h；rank 0..1 = RANK0/RANK1
 *     （寄存器里是 rxmen0 / rxmen1，两者**各自独立**）。
 */
struct ddr_dqs_bypass_config {
	unsigned int use_dqs_bypass;	/* 0: 用自动校准结果，1: 用手填值 */
	unsigned int calib_mode_sel;	/* 1 = Read Preamble（仅 DDR4），0 = Normal Read */
	unsigned int freq_choose_wr_t;	/* reg_freq_choose_wr_t，选频率点 */
	unsigned int cyc_dly[4][2];	/* [lane][rank] 1x     延迟，3 位 */
	unsigned int oph_dly[4][2];	/* [lane][rank] 0.5UI  延迟，3 位 */
	unsigned int dll_dly[4][2];	/* [lane][rank] 4UI/256 延迟，5 位 */
};

/* per-bit skew（训练后回写 / 手工调优） */
struct ddr_deskew_config {
	unsigned int use_deskew;	/* 0: 不使用，1: 使用 */
	unsigned int dqs_skew[8];	/* 每 DQS Tx/Rx skew */
	unsigned int dq_skew[4][8];	/* [byte][dq0..7] per-bit skew */
};

/* ==================== PHY 引脚映射（板级 PCB 走线，工具生成） ====================
 * PHY -> SDRAM 颗粒的连接关系（手册 4.2.1 CMD Pad Map / 4.3.1 DQ Byte Map）。
 * 正常 PCB 一一映射；实际工程按走线做重映射时由工具按本结构体生成。
 * 全部对应 ddrp_regs.h 的 DDRP_*_WRAP_SEL / DDRP_*_BIT_WRAP_SEL 寄存器。
 * 注：byte0..3 = A_l/A_h/B_l/B_h。
 */
struct ddr_pinmap_config {
	unsigned int MSTR1_VALUE;	/* DDRC_MSTR1      0x0008 */
	unsigned int MSTR2_VALUE;	/* DDRC_MSTR2      0x0028 */
	unsigned int MRCTRL0_VALUE;	/* DDRC_MRCTRL0    0x0010（可选） */
	unsigned int MRCTRL1_VALUE;	/* DDRC_MRCTRL1    0x0014（可选） */
	unsigned int MRCTRL2_VALUE;	/* DDRC_MRCTRL2    0x001c（可选） */
	unsigned int use_pinmap;	/* 0: 使用 PHY 默认映射，1: 使用本结构体 */
	unsigned int cmd_wrap[31];	/* CMD pad 映射（5-bit/个）：DDRP_CMD0..30_WRAP_SEL
					 * 值 = SDRAM 信号索引（默认一一映射即索引本身）：
					 * 0..17=A0..A17, 18=ACTN, 19/20=BA0/1, 21/22=BG0/1,
					 * 23..30=CK/CKB/CKE0/CSB0/1/ODT0/1/CKE1
					 * （手册 4.2.1：DDR2/3/4 的 CK/CKB/CSB/ODT/CKE/RESETN
					 *  不可重映射，须保持默认） */
	unsigned int cke_ck_cmd_pad;	/* CMD 重映射配套：DDRP_CKE_CK_CMD_PAD_T（0x080，32-bit）
					 * 标记 CK/CKE 实际所在 pad（bit 置 1）；复位默认 0x00008101 */
	unsigned int byte_wrap[9];	/* DQ byte 映射（4-bit/个）：DDRP_BYTE0..8_WRAP_SEL
					 * dfi 数据 byte -> PHY 顶层 A/B byte（跨 byte 任意组合）
					 * 默认一一映射（值 = 索引）
					 * 注意：cat_wrap 必须与本字段同值（手册 4.3.3 Note） */
	unsigned int dq_bit_wrap[4][8];	/* byte 内 DQ 位映射（4-bit/个）：DDRP_{a/b}_{l/h}_DQ{0..7}_BIT_WRAP_SEL
					 * 值域 0..7 = SDRAM DQ0..7（不能跨 byte）
					 * 注意：DM 不能与 DQ 交换映射（手册 4.3.4 Caution） */
	unsigned int dm_bit_wrap[4];	/* byte 内 DM 映射：DDRP_{a/b}_{l/h}_DM_BIT_WRAP_SEL */
	unsigned int cat_wrap[4];	/* CA 位映射：DDRP_{a/b}_{l/h}_CAT_WRAP_SEL */
	unsigned int rdtrain_check_wrap[4][2]; /* 读训练检查图案（8-bit/个）：
					 * DDRP_{a/b}_{l/h}_RDTRAIN_CHECK_WRAP{0,1}
					 * DQ 顺序改变后必须同步（手册 4.3.1 注） */
	unsigned int rdtrain_check_value_en; /* RDTRAIN 图案使能：DDRP_RD_TRAIN_CHECK_VALUE_EN（0x0a4, 3）
					 * 置 1 才使用 rdtrain_check_wrap（手册 4.3.1：配图案须同时置位） */
};

/* ============================ DDRC 配置（工具生成） ============================ */
/* offset 对应 ddrc_regs.h 中的 DDRC_* 宏 */
struct ddr_ddrc_config {
	unsigned int MSTR_VALUE;	/* DDRC_MSTR       0x0000 */
	unsigned int DRAMTMG_VALUE[18];	/* DDRC_DRAMTMG0..17 0x0100..0x0144 */
	unsigned int RFSHCTL0_VALUE;	/* DDRC_RFSHCTL0   0x0050 */
	unsigned int RFSHCTL1_VALUE;	/* DDRC_RFSHCTL1   0x0054 */
	unsigned int RFSHCTL2_VALUE;	/* DDRC_RFSHCTL2   0x0058 */
	unsigned int RFSHCTL3_VALUE;	/* DDRC_RFSHCTL3   0x0060 */
	unsigned int RFSHCTL4_VALUE;	/* DDRC_RFSHCTL4   0x005c */
	unsigned int RFSHTMG_VALUE;	/* DDRC_RFSHTMG    0x0064 */
	unsigned int RFSHTMG1_VALUE;	/* DDRC_RFSHTMG1   0x0068 */
	unsigned int DFITMG0_VALUE;	/* DDRC_DFITMG0    0x0190 */
	unsigned int DFITMG1_VALUE;	/* DDRC_DFITMG1    0x0194 */
	unsigned int DFIMISC_VALUE;	/* DDRC_DFIMISC    0x01b0 */
	/* DM/DBI（0x0180）：从 MR5 派生，须与 INIT6 的 MR5[10:12] 一致 */
	unsigned int DBICTL_VALUE;	/* DDRC_DBICTL     0x0180 */
	unsigned int DFILPCFG0_VALUE;	/* DDRC_DFILPCFG0  0x0198：DFI 低功耗
					 * [0]PD_EN [8]SR_EN [16]DPD_EN，
					 * wakeup 字段单位 32 DFI clk */
	unsigned int DFILPCFG1_VALUE;	/* DDRC_DFILPCFG1  0x019c：MPSM [0]EN */
	unsigned int DFIUPD0_VALUE;	/* DDRC_DFIUPD0    0x01a0 */
	unsigned int DFIUPD1_VALUE;	/* DDRC_DFIUPD1    0x01a4 */
	unsigned int ODTCFG_VALUE;	/* DDRC_ODTCFG     0x0240 */
	unsigned int ODTMAP_VALUE;	/* DDRC_ODTMAP     0x0244 */
	unsigned int PWRCTL_VALUE;	/* DDRC_PWRCTL     0x0030 */
	unsigned int PWRTMG_VALUE;	/* DDRC_PWRTMG     0x0034 */
	unsigned int ZQCTL0_VALUE;	/* DDRC_ZQCTL0     0x0180 */
	unsigned int ZQCTL1_VALUE;	/* DDRC_ZQCTL1     0x0184 */
	unsigned int ADDRMAP_VALUE[12];	/* DDRC_ADDRMAP0..11 0x0200..0x024c */
	/* ---- SDRAM 自动初始化（uMCTL2 发 MR，DDR4 必需） ----
	 * INIT3/4/6/7 存 DDR4 MR0-6 值，由 uMCTL2 在 SDRAM 初始化序列【自动】写入颗粒，
	 * 驱动不用 MRCTRL 逐个写 MR（与 Ingenic 参考的 MRCTRL 模式不同——那是 DDR2/3 方案）。 */
	unsigned int INIT3_VALUE;	/* DDRC_INIT3 0x00dc：MR0[31:16](mr) + MR1[15:0](emr) */
	unsigned int INIT4_VALUE;	/* DDRC_INIT4 0x00e0：MR2[31:16](emr2) + MR3[15:0](emr3) */
	unsigned int INIT5_VALUE;	/* DDRC_INIT5 0x00e4：dev_zqinit_x32(23:16) ZQ 校准时间 */
	unsigned int INIT6_VALUE;	/* DDRC_INIT6 0x00e8：MR4[31:16] + MR5[15:0] */
	unsigned int INIT7_VALUE;	/* DDRC_INIT7 0x00ec：MR6[15:0]（MR22 是 LPDDR4 用） */
};

/* ============================ DDRP（PHY）配置（工具生成） ============================ */
/* offset 对应 ddrp_regs.h 中的 DDRP_* 宏 */
struct ddr_ddrp_config {
	unsigned int rank_num;		/* rank 数（1/2，训练完成后 rank 选择用） */
	unsigned int MEMCFG_VALUE;	/* ddrp 0x000 整值：mem_select_t/channel_en/burst/... */
	unsigned int AL_VALUE;		/* ddrp 0x008 整值：AL_FRE_OP0..3（4 频率点） */
	unsigned int CL_VALUE;		/* ddrp 0x00c 整值：CL_FRE_OP0..3 */
	unsigned int CWL_VALUE;		/* ddrp 0x010 整值：CWL_FRE_OP0..3 */
	unsigned int VREF_VALUE[4];	/* 每 byte Vref margsel（0 表示保持默认） */
};

/* ============================ 颗粒 / 板级（工具生成） ============================ */
/* 各类型需要的 MR 值分布（DW UMCTL2 机制）：
 *   MR0-6  ：DDR4 由 uMCTL2 自动初始化写入，值放在 ddrc 段 INIT3/4/6/7 寄存器
 *            （INIT3: MR0+MR1, INIT4: MR2+MR3, INIT6: MR4+MR5, INIT7: MR6）。
 *   MR7    ：DDR4 RCD(register clock driver)，仅 RDIMM 用；普通 UDIMM/颗粒直连不用。
 *   MR63   ：Innosilicon 自定义 ZQ 校准，需软件通过 MRCTRL 写（见 ddr.c）。
 *   MR10/11：DDR4 的 ZQ/读写训练，由 uMCTL2 自动发 MRW，无需驱动手动配。
 * 未用字段填 0，驱动按 h.type 跳过（见 ddr.c 的 ddr_mr_write_sequence）。 */
struct ddr_dram_config {
	unsigned int MR7_VALUE;		/* SDRAM MR7（DDR4 RCD/register，仅 RDIMM；UDIMM 填 0） */
	unsigned int MR63_VALUE;	/* SDRAM MR63（Innosilicon 自定义 ZQ 校准，软件写） */
	unsigned int CHIP_0_SIZE;	/* 颗粒 0 容量（MB） */
	unsigned int CHIP_1_SIZE;	/* 颗粒 1 容量（MB） */
};

/* ============================ 总参数结构 ============================ */
struct ddr_param {
	struct ddr_param_header  h;
	struct ddr_ddrc_config   ddrc;	/* DDRC 配置（工具生成） */
	struct ddr_ddrp_config   ddrp;	/* PHY 配置（工具生成） */
	struct ddr_dram_config   dram;	/* 颗粒/板级（工具生成） */
	struct ddr_drvodt_config drvodt;/* 驱动/ODT tuning（板级调优） */
	struct ddr_dqs_bypass_config dqsbp;/* Rx-DQS gating 手动遍历（可选） */
	struct ddr_deskew_config deskew;/* per-bit skew（可选） */
	struct ddr_pinmap_config pinmap;/* PHY 引脚映射（板级 PCB 走线，工具生成） */
};

extern struct ddr_param *g_ddr_param;

#endif /* __DDR_PARAM_H__ */
