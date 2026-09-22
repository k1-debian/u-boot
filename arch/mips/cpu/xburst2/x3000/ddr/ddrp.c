/*
 * ddrp.c - DDRP（Innosilicon PHY）驱动实现
 *
 * 寄存器访问复用 ddrp_regs.h 的 ddrp_read32/ddrp_write32/ddrp_read_reg/
 * ddrp_write_reg（基地址 DDR_PHY_BASE，见 ddrp_regs.h）。
 * 配置直接取自 g_ddr_param->ddrp（工具生成），无驱动侧副本。
 *
 * 流程：ddrp_reset() -> ddrp_load_param()（配置）-> ddrp_zqcalib()
 *     -> ddrp_write_leveling() -> ddrp_rx_dqs_calib() -> ddrp_apply_drvodt()
 *
 * 寄存器寻址约定：
 *   - PHY 手册按位域命名（无整寄存器名），ddrp_regs.h 中每个宏展开为
 *     (offset, hi, lo) 三元组；
 *   - byte 通道寄存器（VREF/drv/odt/zqcali/wrap）分四组
 *     A_l/A_h/B_l/B_h（基址 0x200/0x300/0x400/0x500），本文件以静态
 *     查找表逐 lane 引用同名宏，避免裸偏移与裸位号（宏不能参与算术）。
 *
 * TODO(待确认)：
 *   - WL/Rx-DQS 校准与 DDRC 触发初始化的先后顺序；
 *   - PHY PLL 初始化（本 PHY 无独立 PLL 寄存器？按文档确认）；
 *   - read/write/CA training 调度需 MC/DFI 配合（未在本文件）。
 */
#include <common.h>
#include "ddrp.h"

/* 整寄存器偏移：PHY 手册无整寄存器名，此处按驱动语义命名，
 * 位域组成见 ddrp_regs.h 中对应字段宏。 */
#define DDRP_REG_MEMCFG		0x000	/* mem_select_t/channel_en/burst_sel/dq,dm_invalid */
#define DDRP_REG_AL		0x008	/* AL_FRE_OP0..3 */
#define DDRP_REG_CL		0x00c	/* CL_FRE_OP0..3 */
#define DDRP_REG_CWL		0x010	/* CWL_FRE_OP0..3 */

static int s_last_err = DDR_OK;

static void set_err(int e)
{
	s_last_err = e;
}

static int wait_field(uint32_t reg, uint8_t hi, uint8_t lo,
		      uint32_t expect, uint32_t timeout_us)
{
	uint32_t waited = 0;
	const uint32_t step = 100;
	while (waited < timeout_us) {
		if (ddrp_read_reg(reg, hi, lo) == expect)
			return DDR_OK;
		udelay(1000);
		waited += step;
	}
	return DDR_ERR_TIMEOUT;
}

static int wait_field_nonzero(uint32_t reg, uint8_t hi, uint8_t lo,
			      uint32_t timeout_us)
{
	uint32_t waited = 0;
	const uint32_t step = 100;
	while (waited < timeout_us) {
		if (ddrp_read_reg(reg, hi, lo) != 0u)
			return DDR_OK;
		udelay(1000);
		waited += step;
	}
	return DDR_ERR_TIMEOUT;
}

/* ==================== lane 寄存器查找表（宏展开为三元组） ====================
 * byte0..3 对应 A_l/A_h/B_l/B_h 四组通道寄存器；表项直接以 ddrp_regs.h
 * 的宏初始化，编译器展开为 {offset, hi, lo}，驱动内不再出现裸偏移/裸位号。 */
struct ddrp_lane_reg {
	uint32_t off;
	uint8_t hi;
	uint8_t lo;
};

/* Vref 电平选择：reg_{lane}_vref1_margsel_reg */
static const struct ddrp_lane_reg s_vref1_margsel[2] = {
	{ DDRP_A_L_VREF1_MARGSEL_REG },	/* 0x200, 31, 23 */
	{ DDRP_A_H_VREF1_MARGSEL_REG },
};

/* 驱动强度 P 端：reg_{lane}_abutprcompdq_reg */
static const struct ddrp_lane_reg s_drv_pu[2] = {
	{ DDRP_A_L_ABUTPRCOMPDQ_REG },	/* 0x204, 20, 16 */
	{ DDRP_A_H_ABUTPRCOMPDQ_REG },
};

/* 驱动强度 N 端：reg_{lane}_abutnrcompdq_reg */
static const struct ddrp_lane_reg s_drv_pd[2] = {
	{ DDRP_A_L_ABUTNRCOMPDQ_REG },	/* 0x204, 28, 24 */
	{ DDRP_A_H_ABUTNRCOMPDQ_REG },
};

/* ODT P 端：reg_{lane}_abutodtpudq_reg */
static const struct ddrp_lane_reg s_odt_pu[2] = {
	{ DDRP_A_L_ABUTODTPUDQ_REG },	/* 0x204,  4,  0 */
	{ DDRP_A_H_ABUTODTPUDQ_REG },
};

/* ODT N 端：reg_{lane}_abutodtpddq_reg */
static const struct ddrp_lane_reg s_odt_pd[2] = {
	{ DDRP_A_L_ABUTODTPDDQ_REG },	/* 0x204, 12,  8 */
	{ DDRP_A_H_ABUTODTPDDQ_REG },
};

/*
 * CMD / CK 的驱动寄存器（0x0c8）。**与 DQ 那四张表完全独立** ——
 * 手册 Table 21 把 CMD 明确写成 "CMD **except for CK**"，CK 另有一组；
 * 而且 CMD/CK **没有 ODT** 寄存器（ODT 只有 DQ 有），别照搬 DQ 那四张。
 *   [28:24] reg_cmd_abutnrcomp_reg      CMD(除CK) pull-down
 *   [20:16] reg_cmd_abutprcomp_reg      CMD(除CK) pull-up
 *   [12: 8] reg_cmd_abutnrcomp_ck0_reg  CK pull-down
 *   [ 4: 0] reg_cmd_abutprcomp_ck0_reg  CK pull-up
 */
static const struct ddrp_lane_reg s_cmd_drv_pu = { DDRP_CMD_ABUTPRCOMP_REG };
static const struct ddrp_lane_reg s_cmd_drv_pd = { DDRP_CMD_ABUTNRCOMP_REG };
static const struct ddrp_lane_reg s_ck_drv_pu = { DDRP_CMD_ABUTPRCOMP_CK0_REG };
static const struct ddrp_lane_reg s_ck_drv_pd = { DDRP_CMD_ABUTNRCOMP_CK0_REG };

/* ZQ 校准结果写模拟电路使能：reg_{lane}_dq_odt/drv_zqcali_en */
static const struct ddrp_lane_reg s_odt_zqcali_en[2] = {
	{ DDRP_A_L_DQ_ODT_ZQCALI_EN },	/* 0x200, 22, 22 */
	{ DDRP_A_H_DQ_ODT_ZQCALI_EN },
};

static const struct ddrp_lane_reg s_drv_zqcali_en[2] = {
	{ DDRP_A_L_DQ_DRV_ZQCALI_EN },	/* 0x200, 21, 21 */
	{ DDRP_A_H_DQ_DRV_ZQCALI_EN },
};

/* DQ 引脚绕线：reg_{lane}_dq{0..7}_bit_wrap_sel（byte 内 DQ 位重排） */
static const struct ddrp_lane_reg s_dq_wrap[2][8] = {
	{ { DDRP_A_L_DQ0_BIT_WRAP_SEL }, { DDRP_A_L_DQ1_BIT_WRAP_SEL },
	  { DDRP_A_L_DQ2_BIT_WRAP_SEL }, { DDRP_A_L_DQ3_BIT_WRAP_SEL },
	  { DDRP_A_L_DQ4_BIT_WRAP_SEL }, { DDRP_A_L_DQ5_BIT_WRAP_SEL },
	  { DDRP_A_L_DQ6_BIT_WRAP_SEL }, { DDRP_A_L_DQ7_BIT_WRAP_SEL } },
	{ { DDRP_A_H_DQ0_BIT_WRAP_SEL }, { DDRP_A_H_DQ1_BIT_WRAP_SEL },
	  { DDRP_A_H_DQ2_BIT_WRAP_SEL }, { DDRP_A_H_DQ3_BIT_WRAP_SEL },
	  { DDRP_A_H_DQ4_BIT_WRAP_SEL }, { DDRP_A_H_DQ5_BIT_WRAP_SEL },
	  { DDRP_A_H_DQ6_BIT_WRAP_SEL }, { DDRP_A_H_DQ7_BIT_WRAP_SEL } },
};

/* DM 引脚绕线 / CA 绕线：reg_{lane}_dm/cat_bit_wrap_sel */
static const struct ddrp_lane_reg s_dm_wrap[2] = {
	{ DDRP_A_L_DM_BIT_WRAP_SEL },	/* 0x25c, 13, 10 */
	{ DDRP_A_H_DM_BIT_WRAP_SEL },
};

static const struct ddrp_lane_reg s_cat_wrap[2] = {
	{ DDRP_A_L_CAT_WRAP_SEL },	/* 0x25c, 15, 14 */
	{ DDRP_A_H_CAT_WRAP_SEL },
};

/* CMD pad 映射：reg_cmd{0..30}_wrap_sel（每 pad 5-bit）
 * 索引 0..17=A0..A17, 18=ACTN, 19/20=BA0/1, 21/22=BG0/1,
 * 23..30=CK/CKB/CKE0/CSB0/1/ODT0/1/CKE1（不可重映射，保持默认） */
static const struct ddrp_lane_reg s_cmd_wrap[31] = {
	{ DDRP_CMD0_WRAP_SEL }, { DDRP_CMD1_WRAP_SEL },
	{ DDRP_CMD2_WRAP_SEL }, { DDRP_CMD3_WRAP_SEL },
	{ DDRP_CMD4_WRAP_SEL }, { DDRP_CMD5_WRAP_SEL },
	{ DDRP_CMD6_WRAP_SEL }, { DDRP_CMD7_WRAP_SEL },
	{ DDRP_CMD8_WRAP_SEL }, { DDRP_CMD9_WRAP_SEL },
	{ DDRP_CMD10_WRAP_SEL }, { DDRP_CMD11_WRAP_SEL },
	{ DDRP_CMD12_WRAP_SEL }, { DDRP_CMD13_WRAP_SEL },
	{ DDRP_CMD14_WRAP_SEL }, { DDRP_CMD15_WRAP_SEL },
	{ DDRP_CMD16_WRAP_SEL }, { DDRP_CMD17_WRAP_SEL },
	{ DDRP_CMD18_WRAP_SEL }, { DDRP_CMD19_WRAP_SEL },
	{ DDRP_CMD20_WRAP_SEL }, { DDRP_CMD21_WRAP_SEL },
	{ DDRP_CMD22_WRAP_SEL }, { DDRP_CMD23_WRAP_SEL },
	{ DDRP_CMD24_WRAP_SEL }, { DDRP_CMD25_WRAP_SEL },
	{ DDRP_CMD26_WRAP_SEL }, { DDRP_CMD27_WRAP_SEL },
	{ DDRP_CMD28_WRAP_SEL }, { DDRP_CMD29_WRAP_SEL },
	{ DDRP_CMD30_WRAP_SEL },
};

/* DQ byte 映射：reg_byte{0..8}_wrap_sel（每 byte 4-bit） */
static const struct ddrp_lane_reg s_byte_wrap[9] = {
	{ DDRP_BYTE0_WRAP_SEL }, { DDRP_BYTE1_WRAP_SEL },
	{ DDRP_BYTE2_WRAP_SEL }, { DDRP_BYTE3_WRAP_SEL },
	{ DDRP_BYTE4_WRAP_SEL }, { DDRP_BYTE5_WRAP_SEL },
	{ DDRP_BYTE6_WRAP_SEL }, { DDRP_BYTE7_WRAP_SEL },
	{ DDRP_BYTE8_WRAP_SEL },
};

/* 读训练检查图案：reg_{a/b}_{l/h}_rdtrain_check_wrap{0,1}（各 8-bit） */
static const struct ddrp_lane_reg s_rdtrain_wrap[2][2] = {
	{ { DDRP_A_L_RDTRAIN_CHECK_WRAP0 }, { DDRP_A_L_RDTRAIN_CHECK_WRAP1 } },
	{ { DDRP_A_H_RDTRAIN_CHECK_WRAP0 }, { DDRP_A_H_RDTRAIN_CHECK_WRAP1 } },
};

/*
 * 配读训练检查图案（实现在文件后面的 pinmap 段）。
 * 前置声明放这里：ddrp_read_train() 会先调它 —— 训练不能依赖调用方
 * 记得调 ddrp_apply_pinmap()。
 */
static void rdtrain_check_pattern_apply(void);

static void lane_write(const struct ddrp_lane_reg *r, uint32_t val)
{
	ddrp_write_reg(r->off, r->hi, r->lo, val);
}

/* ==================== Rx 采样相位（DQ per-bit）====================
 *
 * 这是 ddrp_rx_dqs_scan() 的**前提**：扫描靠"写一段数据再读回来比对"判断
 * gating 窗口，而读回来的数据对不对，取决于 **DQ 的采样相位**是否落在数据
 * 眼内。所以扫 gating 之前，DQ（以及 DQS 自身）的延迟必须先大致调好，
 * 否则整片区域都读错，根本看不出窗口边界在哪。
 *
 * 寄存器（手册 Table 26 / 4.3.2 Per-Bit Phase Tuning for Rx Data）：
 * 每个 (byte, rank) 的 DQ0..7 / DM / DQS / DQSB 各有一条独立延迟线
 *   reg_{a/b}_{l/h}_cs{0/1}_{dq0..7|dm|dqs|dqsb}_invdelayselrx
 * 7 位，调整范围 2-UI。**注意别和 Tx 的 *_invdelaysel（9 位）搞混**。
 *
 * 手动刷值流程（手册 Table 27 第一行 "Register (Bypass Read Training)"）：
 *   dq_rd_train_en = 0 / rd_train_perdef_en = 0   —— 关掉自动 read training
 *   train_reg_update_en = 1                       —— 开时钟门
 *   写 invdelayselrx
 *   产生 rd_train_freq_update 的上升沿（0->1->0） —— 把值刷进 PHY
 *   （手册强调要"上升沿"：所以先写 0 再写 1）
 */
struct ddrp_rx_dq_regs {
	struct ddrp_lane_reg dq[8];	/* DQ0..DQ7，各 7 位 */
	struct ddrp_lane_reg dqs;	/* DQS 自身 */
	struct ddrp_lane_reg dqsb;	/* DQS 反相；**必须与 DQS 同值**
					 * （只写 DQS 不写 DQSB，反相端拿不到
					 * 同样的 delay，采样边沿会错开） */
};

/* [byte][rank]
 *
 * X3000 数据通路**最大 16bit = 2 个 byte**，就这两个：
 *   byte0 = 低 byte = A_l（对应 A_DQ0~A_DQ7）
 *   byte1 = 高 byte = A_h（对应 A_DQ8~A_DQ15）
 * **没有 B 组** —— 手册里的 `b_` 是另一组 16bit 通道（byte2/byte3，
 * 对应 B_DQ0~7 / B_DQ8~15），X3000 不引出，别去引用。
 *
 * rank 由 **后缀 `_cs0_` / `_cs1_`** 区分（CS0 = rank0、CS1 = rank1），
 * 与 byte 是两个独立维度，所以 (byte, rank) 直接就是 [A_l/A_h][CS0/CS1]。
 */
static const struct ddrp_rx_dq_regs s_rx_dq[2][2] = {
	{	/* byte 0 = 低 byte = A_l */
		{	/* rank 0 = CS0 */
			{ { DDRP_A_L_CS0_DQ0_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ1_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ2_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ3_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ4_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ5_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ6_INVDELAYSELRX },
			  { DDRP_A_L_CS0_DQ7_INVDELAYSELRX } },
			{ DDRP_A_L_CS0_DQS_INVDELAYSELRX },
			{ DDRP_A_L_CS0_DQSB_INVDELAYSELRX },
		},
		{	/* rank 1 = CS1 */
			{ { DDRP_A_L_CS1_DQ0_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ1_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ2_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ3_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ4_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ5_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ6_INVDELAYSELRX },
			  { DDRP_A_L_CS1_DQ7_INVDELAYSELRX } },
			{ DDRP_A_L_CS1_DQS_INVDELAYSELRX },
			{ DDRP_A_L_CS1_DQSB_INVDELAYSELRX },
		},
	},
	{	/* byte 1 = 高 byte = A_h */
		{	/* rank 0 = CS0 */
			{ { DDRP_A_H_CS0_DQ0_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ1_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ2_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ3_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ4_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ5_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ6_INVDELAYSELRX },
			  { DDRP_A_H_CS0_DQ7_INVDELAYSELRX } },
			{ DDRP_A_H_CS0_DQS_INVDELAYSELRX },
			{ DDRP_A_H_CS0_DQSB_INVDELAYSELRX },
		},
		{	/* rank 1 = CS1 */
			{ { DDRP_A_H_CS1_DQ0_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ1_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ2_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ3_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ4_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ5_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ6_INVDELAYSELRX },
			  { DDRP_A_H_CS1_DQ7_INVDELAYSELRX } },
			{ DDRP_A_H_CS1_DQS_INVDELAYSELRX },
			{ DDRP_A_H_CS1_DQSB_INVDELAYSELRX },
		},
	},
};

/*
 * 把刚写的 DQ/DQS 延迟值刷进 PHY（手册 Table 27「Register (Bypass Read
 * Training)」的手动刷值流程）。
 *
 * 这里的步骤是 **u-boot 侧已在硬件上跑过的 per-bit 流程** 与驱动原流程的
 * **并集**，缺一不可：
 *   1) RX lock code bypass —— 不旁路掉 lock code 检查，写进去的延迟不生效；
 *   2) RDTRAIN_CS_SEL = rank —— 指明本次刷的是哪个 rank（0=CS0, 1=CS1）；
 *   3) 关自动读训练 -> 开时钟门 ->（调用者已写好值）-> rd_train_freq_update
 *      产生上升沿 -> 收时钟门。
 */
static void rx_dq_commit(unsigned int rank)
{
	ddrp_write_reg(DDRP_RX_LOCK_CODE_BP_EN, 1u);
	ddrp_write_reg(DDRP_RX_LOCK_CODE_BP_VALUE, 0xffu);
	ddrp_write_reg(DDRP_RDTRAIN_CS_SEL, rank & 0x1u);

	ddrp_write_reg(DDRP_DQ_RD_TRAIN_EN, 0u);	/* 关自动训练 */
	ddrp_write_reg(DDRP_RD_TRAIN_PERDEF_EN, 0u);
	ddrp_write_reg(DDRP_TRAIN_REG_UPDATE_EN, 1u);	/* 开时钟门 */
	ddrp_write_reg(DDRP_RD_TRAIN_FREQ_UPDATE, 0u);	/* 先清，保证下面有上升沿 */
	ddrp_write_reg(DDRP_RD_TRAIN_FREQ_UPDATE, 1u);
	ddrp_write_reg(DDRP_RD_TRAIN_FREQ_UPDATE, 0u);
	ddrp_write_reg(DDRP_TRAIN_REG_UPDATE_EN, 0u);	/* 收时钟门 */
}

/*
 * lane 是**数据 byte 索引**（0..1 = byte0/byte1），不是 PHY 寄存器组号；
 * rank 0/1 = CS0/CS1。“(byte, rank) -> 寄存器组”的跨组映射由 s_rx_dq
 * 表本身承担（rank0 走 A 组、rank1 走 B 组，见表上方注释）。
 */
int ddrp_set_rx_dq_delay(unsigned int lane, unsigned int rank,
			 unsigned int dq, unsigned int dly)
{
	if (lane > 1u || rank > 1u || dq > 7u)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	lane_write(&s_rx_dq[lane][rank].dq[dq], dly & 0x7fu);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_set_rx_dqs_delay(unsigned int lane, unsigned int rank,
			  unsigned int dly)
{
	if (lane > 1u || rank > 1u)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	lane_write(&s_rx_dq[lane][rank].dqs, dly & 0x7fu);
	/*
	 * DQSB 是 DQS 的反相端，延迟必须同值 —— 只写 DQS 不写 DQSB 的话，
	 * 正/反相采样边沿会错开，读数据眼会偏。
	 */
	lane_write(&s_rx_dq[lane][rank].dqsb, dly & 0x7fu);
	return (set_err(DDR_OK), DDR_OK);
}

/* 改完一批值后调一次，把值刷进 PHY（比逐次刷快）；rank = 0/1 = CS0/CS1 */
void ddrp_rx_dq_update(unsigned int rank)
{
	rx_dq_commit(rank);
}

/*
 * 一次设好某个 rank 下两个 byte 的 RX DQ/DQS/DQSB 延迟（同 byte 内 8 个 DQ
 * 取同值）并刷值 —— 初始化阶段用。
 *
 * CS0/CS1 由 rank 区分，所以 **不需要再单开 *_cs0 / *_cs1 两个函数**：
 *   rank  0 = CS0、1 = CS1
 *   l_dq / h_dq     低 byte(A_l) / 高 byte(A_h) 的 DQ 延迟（7 位）
 *   l_dqs / h_dqs   低 / 高 byte 的 DQS 延迟（DQSB 自动取同值）
 */
void ddrp_set_rx_delay_rank(unsigned int rank,
			    unsigned int l_dq, unsigned int h_dq,
			    unsigned int l_dqs, unsigned int h_dqs)
{
	const unsigned int dq_v[2] = { l_dq, h_dq };
	const unsigned int dqs_v[2] = { l_dqs, h_dqs };
	unsigned int lane, dq;

	if (rank > 1u)
		return;
	for (lane = 0; lane < 2u; lane++) {
		for (dq = 0; dq < 8u; dq++)
			(void)ddrp_set_rx_dq_delay(lane, rank, dq, dq_v[lane]);
		(void)ddrp_set_rx_dqs_delay(lane, rank, dqs_v[lane]);
	}
	ddrp_rx_dq_update(rank);
}

static uint32_t lane_read(const struct ddrp_lane_reg *r)
{
	return ddrp_read_reg(r->off, r->hi, r->lo);
}

/*
 * 每个 lane 的训练结果寄存器组（A_l/A_h/B_l/B_h = 0x280/0x380/0x480/0x580
 * 与 0x284/0x384/0x484/0x584）。两类训练结果：
 *
 *   WL（write leveling, 0x280 family, 8 位一组）
 *     tdqs_invdelaysel0 [23:16]  RANK0 的 WL 结果（高 8 位）
 *     tdqs_invdelaysel1 [ 7: 0]  RANK1 的 WL 结果（低 8 位）
 *
 *   Rx-DQS 校准（0x284 家族，三段合起来才是完整延迟）
 *     cycsel [18:16]  1x    延迟
 *     ophsel [10: 8]  0.5UI 延迟
 *     dllsel [ 4: 0]  1/64UI 延迟
 */
struct ddrp_train_result {
	const char *name;
	struct ddrp_lane_reg wl_rank0;
	struct ddrp_lane_reg wl_rank1;
	struct ddrp_lane_reg cycsel;
	struct ddrp_lane_reg ophsel;
	struct ddrp_lane_reg dllsel;
};

static const struct ddrp_train_result s_train_result[2] = {
	{ "A_l", { DDRP_A_L_TDQS_INVDELAYSEL0 }, { DDRP_A_L_TDQS_INVDELAYSEL1 },
	  { DDRP_A_L_CYCSEL }, { DDRP_A_L_OPHSEL }, { DDRP_A_L_DLLSEL } },
	{ "A_h", { DDRP_A_H_TDQS_INVDELAYSEL0 }, { DDRP_A_H_TDQS_INVDELAYSEL1 },
	  { DDRP_A_H_CYCSEL }, { DDRP_A_H_OPHSEL }, { DDRP_A_H_DLLSEL } },
};


static void pll_postdiv_sel(uint32_t clk1x_mhz, uint32_t *en, uint32_t *div)
{
	if (clk1x_mhz > 400u) {
		*en = 0u;
		*div = 0u;
	} else if (clk1x_mhz > 200u) {
		*en = 1u;
		*div = 1u;
	} else if (clk1x_mhz > 100u) {
		*en = 1u;
		*div = 2u;
	} else {
		*en = 1u;
		*div = 3u;
	}
}

int ddrp_pll_init(uint32_t timeout_us)
{
	uint32_t clk1x_mhz;
	uint32_t en, div;

	/* 已经锁上就别动它（重走初始化会打断工作中的 PLL） */
	if (ddrp_read_reg(DDRP_LOCK_PLL_DQCMD) != 0u)
		return (set_err(DDR_OK), DDR_OK);

	if (!g_ddr_param)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);

	/*
	 * 手册 4.1.1.2 的频率关系：
	 *     fpll_4xclk = fpll_refclk * 4
	 *     fpll_1xclk = fpll_4xclk / 4      => 1xclk 与参考同频
	 * 其中 fpll_refclk 即 dfi_clk1x_in，于是 dfi_clk1x = DDR 时钟 / 2
	 * （1:2 模式；手册例子 2133Mbps -> 533.25MHz 可验）。
	 * g_ddr_param->h.freq 存的是 DDR 时钟 Hz：DDR4-1600 -> 800e6 -> 400MHz。
	 */
	clk1x_mhz = g_ddr_param->h.freq / 2000000u;
	if (clk1x_mhz == 0u)
		clk1x_mhz = 400u;	/* 频率没填时按"不使能后分频"兜底 */
	pll_postdiv_sel(clk1x_mhz, &en, &div);

	/*
	 * POSTDIV 只配 FSP0：手册 5.6 明确 PHY **从 FSP0 启动**，训练完成后才由
	 * dfi_frequency 切到 FSP1..3；单频点流程其余 FSP 保持复位值。
	 */
	ddrp_write_reg(DDRP_PLLPOSTDIV_FSP0, div);
	ddrp_write_reg(DDRP_PLLPOSTDIVEN_FSP0, en);

	ddrp_write_reg(DDRP_PLLPREDIV_DQCMD, 1u);

	/*
	 * lock 判定用 PLL 模块给的真 lock（bypass=0），比用计数器估更可靠；
	 * PLLPD 交给寄存器控制（bypass=1），免得 DFI 低功耗流程中途改这一位。
	 */
	ddrp_write_reg(DDRP_PLL_LOCK_BYPASS, 0u);
	ddrp_write_reg(DDRP_PLLPD_BYPASS, 1u);

	/*
	 * 兜底计数：手册说 PLL 启动后 5000 个参考时钟周期内锁定，
	 * reg_wait_cnt 是 16 位，填满即可（真 lock 方式下这一位不生效）。
	 */
	ddrp_write_reg(DDRP_WAIT_CNT, 0xffffu);

	/* Step 1/2：先关输出、把 pd 置位，清干净内部状态 */
	ddrp_write_reg(DDRP_PLLCLKOUTEN_DQCMD_T, 0u);
	ddrp_write_reg(DDRP_PLLPD_DQCMD_T, 1u);		/* pd = 1（复位 PLL） */
	ddrp_write_reg(DDRP_LOCKENB_DQCMD, 1u);		/* 1 = disable */
	udelay(2);

	/*
	 * Step 3：分频位已在上方按 Table 17 配好（POSTDIV/PREDIV），此处不再改。
	 *
	 * Step 4：释放 pd。手册要求 pd=1 至少保持 1us。
	 */
	udelay(2);
	ddrp_write_reg(DDRP_PLLPD_DQCMD_T, 0u);		/* pd = 0（使能） */
	ddrp_write_reg(DDRP_LOCKENB_DQCMD, 0u);		/* 0 = enable */
	ddrp_write_reg(DDRP_PLLCLKOUTEN_DQCMD_T, 1u);	/* 打开时钟输出 */

	if (wait_field_nonzero(DDRP_LOCK_PLL_DQCMD, timeout_us) != DDR_OK) {
		printf("DDRP Wait PLL Lock Timeout!\n");
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	}

	return (set_err(DDR_OK), DDR_OK);
}

/* ============================ 复位 / 配置 ============================ */
int ddrp_reset(void)
{
	/* soft_reset/soft_reset0/soft_reset1 低有效，先保持复位再释放 */

	ddrp_write_reg(DDRP_SOFT_RESET0, 1u);
	ddrp_write_reg(DDRP_SOFT_RESET1, 1u);
	udelay(1000);
	ddrp_write_reg(DDRP_SOFT_RESET0, 0u);
	ddrp_write_reg(DDRP_SOFT_RESET1, 0u);
	udelay(1000);


	ddrp_pll_init(10000);

	return (set_err(DDR_OK), DDR_OK);
}

/* ==================== Rx-DQS Gating 手动（bypass） ==================== */
/*
 * Table 45：每个 (lane, rank) 都有独立的三组寄存器。
 * rxmen0 = RANK0、rxmen1 = RANK1；lane 0..3 = A_l/A_h/B_l/B_h。
 * 三个值分别对应自动校准结果的 cycsel / ophsel / dllsel。
 */
struct ddrp_dqs_bp_regs {
	struct ddrp_lane_reg delay;	/* 1x      延迟，3 位 */
	struct ddrp_lane_reg ophsel;	/* 0.5UI   延迟，3 位 */
	struct ddrp_lane_reg sdlltap;	/* 4UI/256 延迟，5 位 */
};

static const struct ddrp_dqs_bp_regs s_dqs_bp[2][2] = {
	{	/* lane 0 = A_l */
		{ { DDRP_A_L_RXMEN0_DELAY_BP }, { DDRP_A_L_RXMEN0_OPHSEL_BP },
		  { DDRP_A_L_RXMEN0_SDLLTAP_BP } },	/* rank 0 */
		{ { DDRP_A_L_RXMEN1_DELAY_BP }, { DDRP_A_L_RXMEN1_OPHSEL_BP },
		  { DDRP_A_L_RXMEN1_SDLLTAP_BP } },	/* rank 1 */
	},
	{	/* lane 1 = A_h */
		{ { DDRP_A_H_RXMEN0_DELAY_BP }, { DDRP_A_H_RXMEN0_OPHSEL_BP },
		  { DDRP_A_H_RXMEN0_SDLLTAP_BP } },
		{ { DDRP_A_H_RXMEN1_DELAY_BP }, { DDRP_A_H_RXMEN1_OPHSEL_BP },
		  { DDRP_A_H_RXMEN1_SDLLTAP_BP } },
	},
};

/*
 * ⚠️ 上面的 s_dqs_bp 是 **4 组**的表（A_l/A_h = 组0/1、B_l/B_h = 组2/3），
 * 但 X3000 **只做 16bit = 2 个 byte**，所以只用到组 **0/1**
 * （= A_l/A_h = 低 byte/高 byte）。组 2/3（B 组，手册里的 byte2/byte3，
 * 即 B_DQ0~B_DQ15 那组通道）X3000 不引出，**不要访问**。
 *
 * rank 不在这里做跨组映射：CS0/CS1 由后缀 `rxmen0`/`rxmen1` 区分，
 * 和 byte 是正交的两个维度，所以索引就是 [byte][rank]，byte 号即组号。
 */

int ddrp_dqs_bypass_enable(unsigned int enable)
{
	/* 1 = 用 bypass 寄存器控制 Rx-DQS gating；0 = 用自动校准结果 */
	ddrp_write_reg(DDRP_CALIB_BYPASS, enable ? 1u : 0u);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_set_dqs_bypass(unsigned int lane, unsigned int rank,
			unsigned int cyc_dly, unsigned int oph_dly,
			unsigned int dll_dly)
{
	const struct ddrp_dqs_bp_regs *r;

	/* lane 是**数据 byte 索引**（0..1），不是寄存器组号； */
	if (lane > 1u || rank > 1u)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	/*
	 * lane 是**数据 byte 索引**（0 = 低 byte = A_l，1 = 高 byte = A_h），
	 * 值域卡在 1 是为了不越界到 B 组（组 2/3，X3000 不引出 —— 手册里的
	 * B 组是 byte2/byte3，即另一组 16bit 通道 B_DQ0~15）。
	 */
	r = &s_dqs_bp[lane][rank];

	/*
	 * 手册 6.2.3 的 Step 1~6：
	 *   Step1 开时钟门
	 *   Step2 写三个延迟（1x / 0.5UI / delay line）
	 *   Step3 选频率点 reg_freq_choose_wr_t
	 *   Step4/5 reg_calib_freq_update 0->1->0，把值刷到 PHY
	 *         （手册强调必须"从 0 变 1"才触发更新，所以先写 0）
	 *   Step6 收回时钟门
	 */
	ddrp_write_reg(DDRP_TRAIN_REG_UPDATE_EN, 1u);

	lane_write(&r->delay, cyc_dly & 0x7u);
	lane_write(&r->ophsel, oph_dly & 0x7u);
	lane_write(&r->sdlltap, dll_dly & 0x1fu);

	if (g_ddr_param && g_ddr_param->dqsbp.freq_choose_wr_t)
		ddrp_write_reg(DDRP_FREQ_CHOOSE_WR_T,
			       g_ddr_param->dqsbp.freq_choose_wr_t);

	ddrp_write_reg(DDRP_CALIB_FREQ_UPDATE, 0u);
	ddrp_write_reg(DDRP_CALIB_FREQ_UPDATE, 1u);
	ddrp_write_reg(DDRP_CALIB_FREQ_UPDATE, 0u);

	ddrp_write_reg(DDRP_TRAIN_REG_UPDATE_EN, 0u);

	ddr_platform_log("  [DQS-BP] lane=%d rank=%d  cyc=%d oph=%d dll=%d\n",
			 lane, rank, cyc_dly & 0x7u, oph_dly & 0x7u,
			 dll_dly & 0x1fu);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_apply_dqs_bypass(void)
{
	const struct ddr_dqs_bypass_config *c;
	unsigned int rank_num, lane, rank, n, nbyte;
	uint32_t open;
	int ret;

	if (!g_ddr_param || !g_ddr_param->dqsbp.use_dqs_bypass)
		return (set_err(DDR_OK), DDR_OK);	/* 没启用就什么都不做 */
	c = &g_ddr_param->dqsbp;

	/* 切到手控模式，并按配置选 gating 模式（DDR4 通常用 Read Preamble） */
	ddrp_dqs_bypass_enable(1u);
	ddrp_write_reg(DDRP_CALIB_MODE_SEL, c->calib_mode_sel ? 1u : 0u);

	rank_num = g_ddr_param->ddrp.rank_num;
	if (rank_num < 1u)
		rank_num = 1u;
	if (rank_num > 2u)
		rank_num = 2u;
	n = (rank_num > 1u) ? 2u : 1u;

	/* 数据 byte 数：8bit -> 1、16bit -> 2（本 PHY 上限就是 2） */
	nbyte = g_ddr_param->h.bus_width / 8u;
	if (nbyte < 1u)
		nbyte = 1u;
	if (nbyte > 2u)
		nbyte = 2u;

	/*
	 * 遍历 (数据 byte, rank)。X3000 只有 2 个 byte：byte0 = A_l（低）、
	 * byte1 = A_h（高），byte 号即寄存器组号（组 0/1）；rank 由
	 * rxmen0/rxmen1 后缀区分，与 byte 正交，不跨组。
	 */
	open = ddrp_read_reg(DDRP_CHANNEL_EN);
	for (lane = 0; lane < nbyte; lane++) {	/* lane = 数据 byte（0..nbyte-1） */
		if (!(open & (1u << lane)))
			continue;	/* 该 byte 未使能就不配（8bit 只有 byte0） */
		for (rank = 0; rank < n; rank++) {
			ret = ddrp_set_dqs_bypass(lane, rank,
						  c->cyc_dly[lane][rank],
						  c->oph_dly[lane][rank],
						  c->dll_dly[lane][rank]);
			if (ret != DDR_OK)
				return (set_err(ret), ret);
		}
	}
	return (set_err(DDR_OK), DDR_OK);
}

/*
 * 把 RX Vref 锁存进 PHY。
 *
 * 手册 reg_rx_vref_value_update（0x0a4[7]）：
 *   At posedge of this register, the value of reg_{a/b}_{l/h}_vref1_margsel
 *   will be latched to PHY. At negedge ... keep the system preset value.
 * 即：只写 margsel 不生效，必须造一个 0 -> 1 的上升沿；否则 PHY 一直用
 * 复位预设值。读回来的数据对不对直接取决于这个判决电平，漏掉这步会
 * 表现为"gating 窗口正常、读出的值却不对"。
 */
static void ddrp_rx_vref_latch(void)
{
	ddrp_write_reg(DDRP_RX_VREF_VALUE_UPDATE, 0u);
	ddrp_write_reg(DDRP_RX_VREF_VALUE_UPDATE, 1u);
}

int ddrp_load_param(void)
{
	struct ddr_ddrp_config *c;
	int i, vref_done = 0;

	if (!g_ddr_param)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	c = &g_ddr_param->ddrp;

	/* 写配置寄存器（值来自工具生成的协议，整寄存器写） */
	ddrp_write32(DDRP_REG_MEMCFG, c->MEMCFG_VALUE | 7);
	ddrp_write32(DDRP_REG_AL, c->AL_VALUE);
	ddrp_write32(DDRP_REG_CL, c->CL_VALUE);
	ddrp_write32(DDRP_REG_CWL, c->CWL_VALUE);
	for (i = 0; i < 2; i++) {
		if (c->VREF_VALUE[i]) {
			lane_write(&s_vref1_margsel[i], c->VREF_VALUE[i]);
			vref_done = 1;
		}
	}
	/* 一个都没填（= 保持默认）时才不动 update 位 */
	if (vref_done)
		ddrp_rx_vref_latch();
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_training_config(void)
{
	/*
	 * channel_en：9 位，每个 byte 一位。**不能写死**（0x3 只对 16bit 成立），
	 * 按颗粒位宽算：8bit -> 0x1（只有 byte0）、16bit -> 0x3（byte0~1）。
	 */
	uint32_t nbyte = 2u;		/* 默认 16bit */

	if (g_ddr_param && g_ddr_param->h.bus_width)
		nbyte = g_ddr_param->h.bus_width / 8u;
	if (nbyte < 1u)
		nbyte = 1u;
	if (nbyte > 9u)
		nbyte = 9u;		/* PHY 顶层最多 9 个 byte */

	/* 训练开始前禁止训练寄存器更新，并使能 PHY 通道。 */
	ddrp_write_reg(DDRP_TRAIN_REG_UPDATE_EN, 0u);
	ddrp_write_reg(DDRP_CHANNEL_EN, (1u << nbyte) - 1u);

	/* DDR4 的 DQ/DM 无效值按照 PHY 手册配置为 1。 */
	if (g_ddr_param && g_ddr_param->h.type == DDR4) {
		ddrp_write_reg(DDRP_DQ_INVALID_VALUE, 1u);
		ddrp_write_reg(DDRP_DM_INVALID_VALUE, 1u);
	}

	return (set_err(DDR_OK), DDR_OK);
}

void ddrp_enable_training_reg_update(void)
{
	/*
	 * DDRP_xxx 是字段宏，展开成 (offset, bith, bitl) 三个值，
	 * 而 ddrp_read32()/ddrp_write32() 只收 offset —— 这里要的是
	 * "整寄存器读改写"，用 reg32 版帮手。
	 */
	uint32_t tmp = ddrp_read_reg32(DDRP_TRAIN_REG_UPDATE_EN);

	tmp |= 0xffff0000u;
	ddrp_write_reg32(DDRP_TRAIN_REG_UPDATE_EN, tmp);
}

/* ============================== ZQ 校准 ============================== */

/*
 * 手册 Table 22（DDR4 Drive Strength and ODT Resistance，v3p4 第 38 页）：
 * 把 5 位控制位翻译成**欧姆值**。Pull-up / Pull-down 共用同一张表。
 *
 *   控制位  11111 11110 11101 11100 | 11011 11010 11001 11000
 *   阻抗      23    25    26    27  |  28    30    32    34    (ohm)
 *           10111 10110 10101 10100 | 10011 10010 10001 10000
 *            36    38    41    45  |  49    55    60    68
 *           01111 01110 01101 01100 | 01011 01010 01001 01000
 *            36    38    41    45  |  49    55    60    68
 *           00111 00110 00101 00100 | 00011 00010 00001 00000
 *            79    90    108   135  |  178   270   535   ∞(开路)
 *
 * ⚠️ bit4=1 与 bit4=0 在 36~68 这一段**重复**，是手册原表就这样
 *    （bit4 是额外并联的 leg，两种组合等效），照抄，别"修正"。
 * 数组下标 = 5 位控制位；**0 表示 ∞（开路）** —— 公式里会退化成 0，
 * 所以下面统一做了除零保护。
 *
 * ⚠️ DDR3 用 Table 23（数值略有不同），本工具跑 DDR4，只放 DDR4 这张。
 */
static const unsigned int s_drvodt_ohm[32] = {
	0u,   535u, 270u, 178u, 135u, 108u, 90u, 79u,	/* 00000..00111 */
	68u,  60u,  55u,  49u,  45u,  41u,  38u, 36u,	/* 01000..01111 */
	68u,  60u,  55u,  49u,  45u,  41u,  38u, 36u,	/* 10000..10111 */
	34u,  32u,  30u,  28u,  27u,  26u,  25u, 23u,	/* 11000..11111 */
};

/*
 * 配置 ZQ 校准用的四个 vref_sel（手册 5.2.1 ZQ Calibration，v3p4 第 54 页）。
 *
 * 公式（原文）：
 *   reg_drvpu_zqcali_vref_sel = 256*120/(120+Rup)
 *   reg_drvpd_zqcali_vref_sel = 256*Rdn/(Rup+Rdn)
 *   reg_odtpu_zqcali_vref_sel = 256*120/(120+Rupodt)
 *   reg_odtpd_zqcali_vref_sel = 256*Rdnodt/(Rupodt+Rdnodt)
 *   Rup=40 -> drvpu=192；Rdn=Rup=40 -> drvpd=128（手册举例，可当自检）
 *
 * 手册两条硬性说明：
 *   · drvpd 公式里的 Rup 必须和 drvpu 公式里的 **是同一个值**；
 *   · odtpd 公式里的 Rupodt 必须和 odtpu 公式里的 **是同一个值**。
 *   所以下面每类只取一份 Rup / Rupodt，别各算各的。
 *
 * 欧姆值怎么来：**直接读 PHY 的驱动/ODT 寄存器**（5 位控制位）再查表 ——
 * 而不是从 g_ddr_param->drvodt 取，因为 use_drvodt_config=0 时那些字段是 0，
 * 实际生效的是 PHY 复位默认值。读寄存器才能反映**真正当前**的阻抗。
 *
 * 顺序（手册 Table 35 "ZQ calibration" 那行）：
 *   设驱动强度寄存器 -> {drv,odt}_zqcali_en 置 0 -> **设这四个 vref_sel**
 *   -> reg_zqcali_en=1 等 done -> reg_zqcali_en=0
 * 所以本函数必须在 ddrp_write_reg(DDRP_ZQCALI_EN, 1) **之前**调用。
 */
static void zqcali_vref_sel_update(unsigned int *ohm)
{
	unsigned int rup, rdn, rupodt, rdnodt;
	unsigned int vpu, vpd, vopu, vopd;

	/* 5 位控制位 -> 欧姆（只取 byte0/A_l：手册公式是全局量） */
	rup	= s_drvodt_ohm[lane_read(&s_drv_pu[0]) & 0x1fu];
	rdn	= s_drvodt_ohm[lane_read(&s_drv_pd[0]) & 0x1fu];
	rupodt	= s_drvodt_ohm[lane_read(&s_odt_pu[0]) & 0x1fu];
	rdnodt	= s_drvodt_ohm[lane_read(&s_odt_pd[0]) & 0x1fu];

	/*
	 * 把"目标阻抗"回传给调用方 —— 它要和 ZQ 校准的**实际结果**（`*_2reg`）
	 * 放在一起对照打印；只打一边看不出校得准不准。
	 * 顺序固定：ohm[0..3] = {Rup, Rdn, Rupodt, Rdnodt}；不需要时传 NULL 语义的 0。
	 */
	if (ohm) {
		ohm[0] = rup;
		ohm[1] = rdn;
		ohm[2] = rupodt;
		ohm[3] = rdnodt;
	}

	/* 除零/开路保护：分母为 0 时给 0（不让它算出垃圾值） */
	vpu  = rup ? (256u * 120u) / (120u + rup) : 0u;
	vpd  = (rup + rdn) ? (256u * rdn) / (rup + rdn) : 0u;
	vopu = rupodt ? (256u * 120u) / (120u + rupodt) : 0u;
	vopd = (rupodt + rdnodt) ? (256u * rdnodt) / (rupodt + rdnodt) : 0u;

	ddrp_write_reg(DDRP_DRVPU_ZQCALI_VREF_SEL, vpu & 0xffu);
	ddrp_write_reg(DDRP_DRVPD_ZQCALI_VREF_SEL, vpd & 0xffu);
	ddrp_write_reg(DDRP_ODTPU_ZQCALI_VREF_SEL, vopu & 0xffu);
	ddrp_write_reg(DDRP_ODTPD_ZQCALI_VREF_SEL, vopd & 0xffu);

	ddr_platform_log("zqcali vref_sel: Rup=%d Rdn=%d Rupodt=%d Rdnodt=%d ohm"
			 " -> drvpu=%d drvpd=%d odtpu=%d odtpd=%d\n",
			 rup, rdn, rupodt, rdnodt, vpu, vpd, vopu, vopd);
	if (!rup || !rdn || !rupodt || !rdnodt)
		ddr_platform_log("  !! some impedance is open(0/ohm=inf) -- check"
				 " the drive/ODT control bits (Table 22)\n");
}

/*
 * 5 位阻抗控制码 -> 欧姆（手册 Table 22 那张表，与 `s_drvodt_ohm[]` 同一份）。
 * ⚠️ 下标 0 是"∞ / 开路"，本函数返回 0 —— 调用方要单独标注，别当成"0 欧姆"。
 * 用来把 ZQ 校准结果（`{drv,odt}{pd,pu}leg_zqcali_2reg`，驱动控制位是同一套
 * leg 编码）换算成欧姆，好与 vref_sel 的目标阻抗对照。
 */
static unsigned int drvodt_code2ohm(unsigned int code)
{
	return s_drvodt_ohm[code & 0x1fu];
}

/*
 * ZQ 校准用的"单项允许偏差（百分比）" —— 正常量化误差只有 ±1 档
 * （Table 22 相邻档大约差 10~20%），取 50% 既能容忍档位跳动，
 * 又能抓住"外部 RZQ 比例错"这种系统性偏差。
 */
#define DDRP_ZQCALI_OHM_TOL_PCT	50u

/*
 * 用"校准结果 vs 目标"判断**外部 RZQ 是否异常**（返回码见下）。
 *
 * 为什么需要它（现场教训）：**RZQ 接错（例如接 240Ω，而手册要求 120Ω）时
 * 校准依然会 `done=1`、`overflow` 全 0** —— 它只是"用一个错的基准收敛到了
 * 错的地方"。所以光看 done/overflow 查不出来，必须看**结果与目标的关系**：
 *   · 正常：各腿结果贴着目标，误差 ≤ ±1 档、且**方向不一致**；
 *   · RZQ 偏大 ⇒ 校准结果**四项同向偏高**（实测 240Ω 时达到目标的
 *     165%~250%）；RZQ 偏小则同向偏低。
 * 判据刻意保守：**至少 2 项有效、且所有有效项都同向超差**才判"异常"
 * （单项超差可能是噪声/单点问题，不算外部基准问题）。
 *
 * 返回：0 = 正常；1 = 整体偏大（疑似 RZQ 过大）；2 = 整体偏小（疑似 RZQ 过小）；
 *       3 = 有效数据不足（目标或结果为 0/开路），不判定。
 * `ratio_pct` 回传"实际/目标"的平均百分比（0 表示无数据）。
 */
static unsigned int zqcali_result_check(const unsigned int *ohm,
				       const unsigned int *got,
				       unsigned int *ratio_pct)
{
	unsigned int i, n = 0u, big = 0u, small = 0u, sum = 0u;

	for (i = 0u; i < 4u; i++) {
		unsigned int r;

		if (!ohm[i] || !got[i])
			continue;	/* 目标未知或开路 ⇒ 这一项不判 */
		r = (got[i] * 100u) / ohm[i];	/* 实际/目标 (%) */
		sum += r;
		n++;
		if (r > 100u + DDRP_ZQCALI_OHM_TOL_PCT)
			big++;
		else if (r + DDRP_ZQCALI_OHM_TOL_PCT < 100u)
			small++;
	}

	*ratio_pct = n ? (sum / n) : 0u;
	if (n < 2u)
		return 3u;		/* 数据太少，不判定 */
	if (big == n)
		return 1u;		/* 全部同向偏大 */
	if (small == n)
		return 2u;		/* 全部同向偏小 */
	return 0u;
}

/*
 * ZQ 校准（手册 5.2 + Table 35 "Three Paths to Control ZQ Calibration"）：
 *   ① 按目标阻抗配四个 vref_sel（比较基准）→ ② zqcali_en=1 / 等 done / 清 0
 *   → ③ 把三类使能位（cmd_drv / dq_drv / dq_odt 的 zqcali_en）置 1，让结果驱动模拟电路。
 */
int ddrp_zqcalib(uint32_t timeout_us)
{
	int byte;
	unsigned int ovf;
	unsigned int done;
	unsigned int ohm[4];	/* 目标阻抗：drvpu/drvpd/odtpu/odtpd */

	/* 函数开始：留一份"ZQ 校准前"的驱动/ODT + mux 快照（与结束时对照） */
	ddrp_dump_drvodt("zqcalib begin");

	/*
	 * ② 之前先把模块弄进"工作态"。这两位 Table 35 没列，但按字段说明不配好，
	 *    模块根本不在正常工作状态（若你确认板上本来就能跑通，删掉这两行即可）：
	 *   · reg_pd_zqcali（0x094[3]，**复位 1 = Power down**）：0 = Normal work mode；
	 *   · reg_hclk_zqcalib_sel（0x08c[29]，复位 0）：不在训练模式下会自动关掉 ZQ
	 *     模块时钟，而本函数是**训练外**手动跑 ⇒ 置 1 一直保留模块时钟。
	 */
	ddrp_write_reg(DDRP_PD_ZQCALI, 0u);
	ddrp_write_reg(DDRP_HCLK_ZQCALIB_SEL, 1u);

	/* ① 比较基准（四个 vref_sel），同时取回目标阻抗供后面与结果对照 */
	zqcali_vref_sel_update(ohm);

	/* ② 跑校准 */
	ddrp_write_reg(DDRP_ZQCALI_EN, 1u);
	ddr_platform_delay(50);
	if (wait_field(DDRP_ZQCALI_DONE, 1u, timeout_us) != DDR_OK) {
		/*
		 * ⚠️⚠️ 失败路径**必须把模块关回去**：否则 `reg_zqcali_en` 一直留在 1，
		 *    ZQ 模块卡在"校准中"，而模拟电路的驱动/ODT 就由这个**没完成**的
		 *    模块控制 ⇒ CMD/CK/DQ 驱动强度不可控 ⇒ DDR 彻底不通（连命令都发不
		 *    出去）⇒ 症状就是"调了 zqcalib 之后系统起不来"。
		 *    两个临时使能位一并恢复复位语义（power down + 不额外保留时钟）。
		 */
		ddrp_write_reg(DDRP_ZQCALI_EN, 0u);
		ddrp_write_reg(DDRP_PD_ZQCALI, 1u);
		ddrp_write_reg(DDRP_HCLK_ZQCALIB_SEL, 0u);
		/* 调用方 ddr.c 不看返回值 ⇒ 这里必须自己报出来，否则超时是"静默"的 */
		ddr_platform_log("zqcali: TIMEOUT (reg_zqcali_done not set) -"
				 " check reg_pd_zqcali / reg_hclk_zqcalib_sel\n");
		ddrp_dump_drvodt("zqcalib end (timeout)");
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	}
	/*
	 * ⭐⭐ **必须在清 `reg_zqcali_en` 之前把状态读走**（时机很关键）：
	 *   · `reg_zqcali_done` 是校准引擎的"完成"状态，**不是 sticky 位** ——
	 *     引擎一关（en=0）它就回 0，所以在清 en **之后**读 done 永远是 0。
	 *     （实测就是这样：能走到这里说明 `wait_field` 已经确认过 done=1，
	 *      但打印出来却是 done=0 ⇒ 那是**打印时机**，不是校准失败。）
	 *   · 4 个 `reg_{drv,odt}{pd,pu}_overflow` **failure flag** 同属引擎状态，
	 *     同理 —— 在清 en 之后读，这个检查就**形同虚设**。
	 *   · `{drv,odt}{pd,pu}leg_zqcali_2reg`（0x160，RO，5 位/个）= 校准出来的
	 *     阻抗控制码（会转交给 PHY IO），与四个 vref_sel 换算出的目标值对照
	 *     即可判断这次校准准不准。
	 */
	done = (unsigned)ddrp_read_reg(DDRP_ZQCALI_DONE);
	ovf = (unsigned)ddrp_read_reg(DDRP_DRVPD_OVERFLOW) |
	      (unsigned)ddrp_read_reg(DDRP_DRVPU_OVERFLOW) |
	      (unsigned)ddrp_read_reg(DDRP_ODTPD_OVERFLOW) |
	      (unsigned)ddrp_read_reg(DDRP_ODTPU_OVERFLOW);
	ddr_platform_log("zqcali: done=%d ovf(dp=%d up=%d odtpd=%d odtpu=%d)\n",
			 done,
			 ddrp_read_reg(DDRP_DRVPD_OVERFLOW),
			 ddrp_read_reg(DDRP_DRVPU_OVERFLOW),
			 ddrp_read_reg(DDRP_ODTPD_OVERFLOW),
			 ddrp_read_reg(DDRP_ODTPU_OVERFLOW));

	/*
	 * ⭐ 目标 vs 实际（一眼看出这次校得准不准）：
	 *   · target = ① 按**当前驱动/ODT 寄存器档位**换算出的欧姆（vref_sel 的依据）；
	 *   · code = 校准结果 `*_2reg`（0x160，RO）的 5 位码，用同一张驱动码表
	 *     （Table 22）换回欧姆；
	 *   · delta 在 ±1 档以内属正常量化误差；偏得多、或掉进 `0`(开路) / `31`(最弱)
	 *     这类极值，说明这次校准没收敛 ⇒ 查 ZQ 电阻（120Ω±1%）、ZQ 引脚容性负载
	 *     （手册要求 < 3pF）、VDDQ 与噪声。
	 */
	{
		static const char *nm[4] = { "drvpu", "drvpd", "odtpu", "odtpd" };
		unsigned int c[4], o2[4], i, rc, ratio;

		c[0] = (unsigned)ddrp_read_reg(DDRP_DRVLEGPU_ZQCALI_2REG);
		c[1] = (unsigned)ddrp_read_reg(DDRP_DRVLEGPD_ZQCALI_2REG);
		c[2] = (unsigned)ddrp_read_reg(DDRP_ODTLEGPU_ZQCALI_2REG);
		c[3] = (unsigned)ddrp_read_reg(DDRP_ODTLEGPD_ZQCALI_2REG);
		for (i = 0u; i < 4u; i++)
			o2[i] = drvodt_code2ohm(c[i]);

#if DDRP_DBG_ZQCALI
		ddr_platform_log("  target vs result (ohm):\n");
		for (i = 0u; i < 4u; i++)
			ddr_platform_log("   %s: target %d -> code %d = %d"
					 " (delta %d)%s\n",
					 nm[i], ohm[i], c[i], o2[i],
					 (int)o2[i] - (int)ohm[i],
					 (o2[i] == 0u) ? "  ** open(inf) **" : "");
#else
		(void)nm;	/* 关掉详细对照表时不留 unused 警告 */
#endif

		/*
		 * ⭐ 结论行：把"结果是否偏离目标"变成一个**可 grep 的判断**。
		 * 这是唯一能抓到"RZQ 接错但校准仍然 done=1/无 overflow"的信号。
		 */
		rc = zqcali_result_check(ohm, o2, &ratio);
		if (rc == 1u || rc == 2u) {
			ddr_platform_log("  ** ZQ RESULT OFF TARGET: all valid"
					 " legs %s by >%d%% (avg %d%% of"
					 " target) -> suspect external RZQ **\n",
					 (rc == 1u) ? "HIGH" : "LOW",
					 DDRP_ZQCALI_OHM_TOL_PCT, ratio);
			ddr_platform_log("  ** RZQ: manual requires 120 ohm"
					 " +/-1%% between ZQ pin and VDDQ (or GND),"
					 " ZQ pin loading < 3pF;"
					 " rough RZQ estimate = %d ohm **\n",
					 (120u * ratio) / 100u);
		} else if (rc == 3u) {
			ddr_platform_log("  (zqcali: too few valid legs to judge"
					 " RZQ -> skipped)\n");
		} else {
			ddr_platform_log("  (zqcali: result within %d%% of target"
					 " -> ok)\n", DDRP_ZQCALI_OHM_TOL_PCT);
		}
	}

	/* 状态取完了，再关引擎 */
	ddrp_write_reg(DDRP_ZQCALI_EN, 0u);

	/* 模块用完就让它回去（复位语义：power down + 不额外保留时钟） */
	ddrp_write_reg(DDRP_PD_ZQCALI, 1u);
	ddrp_write_reg(DDRP_HCLK_ZQCALIB_SEL, 0u);

	/*
	 * ⚠️⚠️ ③ 这 5 位是 **mux**，`reg_cmd_drv_zqcalib_en` 原文：
	 *    "1: Choose the ZQ calibration result as the control signal.
	 *     0: Choose the register as the control signal."
	 *    —— 置 1 之后 CMD/CK/DQ/ODT 的驱动强度**全部**改由 ZQ 结果驱动；
	 *    一旦这次校准有 overflow，等于**把坏值接进模拟电路** ⇒ 连命令都发不出去。
	 *    所以：**有 overflow 就不切**，退回 Table 35 第 1 条路径
	 *    （I/O Drive Strength Register，即 ddrp_apply_drvodt() 写的寄存器值）。
	 */
	if (ovf) {
		ddr_platform_log("zqcali: OVERFLOW -> NOT using ZQ result,"
				 " keep I/O drive strength registers\n");
		ddrp_dump_drvodt("zqcalib end (overflow, regs kept)");
		return (set_err(DDR_OK), DDR_OK);
	}

	/* ③ 让校准结果生效：DQ 只有 A_l/A_h 两组（byte0/byte1）+ CMD/CK 那一位 */
	for (byte = 0; byte < 2; byte++) {
		lane_write(&s_odt_zqcali_en[byte], 1u);
		lane_write(&s_drv_zqcali_en[byte], 1u);
	}
	ddrp_write_reg(DDRP_CMD_DRV_ZQCALIB_EN, 1u);

	/* 函数结束：与 begin 那份对照，能直接看出 mux 是否切换、结果是否生效 */
	ddrp_dump_drvodt("zqcalib end (regs applied)");

	return (set_err(DDR_OK), DDR_OK);
}

/* ============================== Write Leveling ============================== */

/*
 * 打印 Write Leveling 结果（4 个 lane 一行一条）。
 *
 * 数据在 0x280 家族：
 *   tdqs_invdelaysel0 [23:16]  RANK0 的 WL 结果（高 8 位）
 *   tdqs_invdelaysel1 [ 7: 0]  RANK1 的 WL 结果（低 8 位）
 * RANK1 没用到时低 8 位保持复位值 0x80，不代表训练值。
 *
 * 只在 WL 流程（ddrp_write_leveling）里调用 —— WL 没跑时这几个寄存器
 * 全是复位值，提前打出来反而误导。
 */
static void ddrp_dump_wl_result(unsigned int rank)
{
#if DDRP_DBG_TRAIN
	unsigned int i;

	printf("---- Write Leveling result (rank%d, 0x280 family, "
	       "high 8b=RANK0 / low 8b=RANK1)----\n", rank);
	for (i = 0; i < 2u; i++) {
		const struct ddrp_train_result *t = &s_train_result[i];

		printf("  %s: rank0=0x%x rank1=0x%x\n", t->name,
				 lane_read(&t->wl_rank0),
				 lane_read(&t->wl_rank1));
	}
#else
	(void)rank;
#endif
}

int ddrp_write_leveling(uint32_t timeout_us)
{
	unsigned int rank_num = g_ddr_param ? g_ddr_param->ddrp.rank_num : 1;
	unsigned int rank;
	uint32_t want = ddrp_read_reg(DDRP_CHANNEL_EN) & 0x1ffu;
	int ret;

	/*
	 * **每个 CS/RANK 各跑一轮**。
	 * 手册 reg_wlcs_sel 的 Note：使能 WL 期间不能设 2'b00，必须显式选
	 * 10=RANK0 / 01=RANK1；双 rank 就得跑两次，最后再切回 00（自动切换）。
	 * 原来只跑一次 RANK0，接双 rank 颗粒时 RANK1 根本没训练。
	 */
	if (rank_num < 1u)
		rank_num = 1u;
	if (rank_num > 2u)
		rank_num = 2u;
	if (want == 0u)
		want = 0x1u;


	for (rank = 0; rank < rank_num; rank++) {
		ddrp_write_reg(DDRP_WL_ENABLE, 0u);	/* 先清状态机 */
		ddrp_write_reg(DDRP_WLCS_SEL, rank ? 1u : 2u);
		ddrp_write_reg(DDRP_WL_ENABLE, 1u);
		ret = wait_field(DDRP_WL_END, 1u, timeout_us);
		if (ret == DDR_OK)
			ret = wait_field(DDRP_WL_DONE_BYTE, want, timeout_us);

		ddrp_write_reg(DDRP_WL_ENABLE, 0u);

		if (ret != DDR_OK) {
			ddrp_write_reg(DDRP_WLCS_SEL, rank_num > 1u ? 0u : 2u);
			/* 超时也要看结果：哪个 byte 没完成一眼可见 */
			ddrp_dump_wl_result(rank);
			return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
		}

	}
	ddrp_dump_wl_result(rank_num - 1u);	
	ddrp_write_reg(DDRP_WL_ENABLE, 0u);
	ddrp_write_reg(DDRP_WLCS_SEL, 0);
	return (set_err(DDR_OK), DDR_OK);
}

/*
 * 打印 Rx-DQS calib result（4 个 lane 一行一条）。
 *
 * 数据在 0x284 家族，**三段合起来才是完整延迟**：
 *   cycsel [18:16]  1x     延迟
 *   ophsel [10: 8]  0.5UI  延迟
 *   dllsel [ 4: 0]  1/64UI 延迟
 *
 * 只在 Rx-DQS 校准流程（ddrp_rx_dqs_calib）里调用。
 */
void ddrp_dump_rx_dqs_result(void)
{
#if DDRP_DBG_TRAIN
	unsigned int i;

	printf("---- Rx-DQS calib result"
			 " (0x284 family, cycsel/ophsel/dllsel combined)----\n");
	for (i = 0; i < 2u; i++) {
		const struct ddrp_train_result *t = &s_train_result[i];

		printf("  %s: cycsel=%d (1x)  ophsel=%d (0.5UI)  "
				 "dllsel=%d (1/64UI)\n", t->name,
				 lane_read(&t->cycsel), lane_read(&t->ophsel),
				 lane_read(&t->dllsel));
	}
#endif
}

/*
 * 打印 Rx-DQS calib result。
 *
 * 结果都在 0x174（RO）：
 *   calib_end[10]        所有使能通道都完成
 *   calib_error[9]       校准出错标志（超时 / 读命令数超过 reg_max_rdvalue）
 *   calib_done_byte[8:0] 每个 byte 的完成标志
 *
 * 注意：未使能的通道（CHANNEL_EN 对应位为 0）本来就不会置 done，
 * 所以逐个 byte 只对被使能且没完成的通道报"未完成"，避免误报。
 */
static void ddrp_log_rx_dqs_result(unsigned int rank)
{
#if DDRP_DBG_TRAIN
	uint32_t end = ddrp_read_reg(DDRP_CALIB_END);
	uint32_t err = ddrp_read_reg(DDRP_CALIB_ERROR);
	uint32_t done = ddrp_read_reg(DDRP_CALIB_DONE_BYTE);
	uint32_t open = ddrp_read_reg(DDRP_CHANNEL_EN);
	unsigned int i;

	printf("  [Rx-DQS] rank=%d calib_end=%d error=%d "
			 "done_byte=0x%x channel_en=0x%x\n",
			 rank, end, err, done, open);

	for (i = 0; i < 9u; i++) {
		if ((open & (1u << i)) && !(done & (1u << i)))
			printf("    byte%d NOT done (enabled but done=0)\n", i);
	}
	if (err)
		printf("    calib_error=1: check reg_max_rdvalue / "
				 "reg_calib_timeout, or DQS gating found no window\n");
#else
	(void)rank;
#endif
}


/* ============================== Rx-DQS 校准 ============================== */
int ddrp_rx_dqs_calib(uint32_t timeout_us)
{
	unsigned int rank_num = g_ddr_param ? g_ddr_param->ddrp.rank_num : 1;
	int ret;

	/*
	 * "哪些 byte 该完成"**不能写死**：颗粒位宽决定 CHANNEL_EN
	 * （8bit 只使能 byte0、16bit 使能 byte0~1，B 通道再往后排）。
	 * 写死 3（=byte0+byte1）接 8bit 颗粒时会一直等 byte1 直到超时。
	 */
	uint32_t want = ddrp_read_reg(DDRP_CHANNEL_EN) & 0x1ffu;
	unsigned int rank;

	if (rank_num < 1u)
		rank_num = 1u;
	if (rank_num > 2u)
		rank_num = 2u;
	if (want == 0u)
		want = 0x1u;

	/* Keep the PHY issuing refreshes while the training state machine runs. */
	ddrp_write_reg(DDRP_PHY_REFRESH_EN, 1u);

	/*
	 * DQS gating 模式选择（手册 §6.2.2 Step 2 —— **这一步属于 Rx DQS gating，
	 * 不属于 read training**）：
	 *   1 = Read Preamble Training mode（手册注明 "Only for DDR4"）
	 *   0 = Normal Read mode（复位值；此时手册要求把 reg_a_l/h_weakpub_reg
	 *       设成 2'b01 把高阻态下的 Read DQS 拉起来）
	 * DDR4 走读前导训练模式，与颗粒侧 MR4 A10（Read Preamble Training Mode，
	 * 见 ddrp_training_config）成对。
	 * 实测两种模式 gating 都能过（calib_end=1 / error=0 / done_byte=0x3），
	 * 所以这里不是当前故障的重点。
	 */
	if (g_ddr_param && g_ddr_param->h.type == DDR4)
		ddrp_write_reg(DDRP_CALIB_MODE_SEL, 1u);

	/*
	 * **每个 CS/RANK 各跑一轮**：手册 reg_calcs_sel 的 Note 说使能
	 * Rx-DQS 校准期间不能设 2'b00，必须显式选 10=RANK0 / 01=RANK1。
	 */
	for (rank = 0; rank < rank_num; rank++) {

		ddrp_write_reg(DDRP_START_CALIB, 0u);	/* 先清状态机 */
		ddrp_write_reg(DDRP_CALCS_SEL, rank ? 1u : 2u);
		ddrp_write_reg(DDRP_START_CALIB, 1u);

		/* CALIB_END is asserted once all enabled bytes have completed. */
		ret = wait_field(DDRP_CALIB_END, 1u, timeout_us);
		if (ret != DDR_OK)
			goto timeout;

		/* Keep the source flow's byte-complete check (bytes 0 and 1). */
		ret = wait_field(DDRP_CALIB_DONE_BYTE, want, timeout_us);
		if (ret != DDR_OK)
			goto timeout;

		/* 校准结束，打印结果（平台没实现 log 时是空操作） */
		ddrp_log_rx_dqs_result(rank);
		ddrp_write_reg(DDRP_START_CALIB, 0u);
		ddrp_write_reg(DDRP_PHY_REFRESH_EN, 0u);
		ddrp_write_reg(DDRP_CALCS_SEL, (rank_num > 1) ? 0u : 2u);


	}
	ddrp_dump_rx_dqs_result();

	return (set_err(DDR_OK), DDR_OK);

timeout:
	ddrp_write_reg(DDRP_START_CALIB, 0u);
	ddrp_write_reg(DDRP_PHY_REFRESH_EN, 0u);
	ddrp_write_reg(DDRP_CALCS_SEL, (rank_num > 1) ? 0u : 2u);

	ddrp_log_rx_dqs_result(rank);
	ddrp_dump_rx_dqs_result();
	return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
}

/* ============================== Read Training ============================== */
/*
 * 依据手册 **6.3 Read Training** + Table 46（寄存器）+ Table 47（结果）
 * + 6.3.2 Figure 24（Auto 流程）。
 *
 * 目的：为每个 DQ 找出 Rx delay line 的通过窗口，并把 Rx DQS 对齐到 DQ
 *       数据眼中心 —— 即训练 **DQ 与 DQS 的延迟**（per-bit de-skew，2-UI）。
 *
 * Auto（MPR/MPC）流程（Figure 24）：
 *   1) 开 PHY auto-refresh（training 期间控制器会插入 refresh）
 *   2) reg_rdtrain_cs_sel 选 rank（2'b10 = RANK0、2'b01 = RANK1）
 *   3) reg_dq_rd_train_en = 1 启动
 *   4) （控制器发 auto-refresh）
 *   5) 等 train_true_done = 1'b1
 *   6) 读 reg_train_error_for_rd_byte 判成败（高 = 错）
 *   7) reg_dq_rd_train_en = 0 退出
 *   8) 从 Table 47 的结果寄存器读窗口
 *
 * 结果**由 PHY 自动应用**到 Rx 采样相位，软件不必再写
 * （想软件指定相位就走 bypass：ddrp_set_rx_dq_delay + ddrp_rx_dq_update）。
 */

/* 结果寄存器（手册 Table 47）。索引 [byte][dq]：byte 0/1 = A_l/A_h */
static const struct ddrp_lane_reg s_rd_min_dq[2][8] = {
	{ { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ0 }, { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ1 },
	  { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ2 }, { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ3 },
	  { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ4 }, { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ5 },
	  { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ6 }, { DDRP_A_L_TRAIN_MIN_FOR_RD_DQ7 } },
	{ { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ0 }, { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ1 },
	  { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ2 }, { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ3 },
	  { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ4 }, { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ5 },
	  { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ6 }, { DDRP_A_H_TRAIN_MIN_FOR_RD_DQ7 } },
};

static const struct ddrp_lane_reg s_rd_max_dq[2][8] = {
	{ { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ0 }, { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ1 },
	  { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ2 }, { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ3 },
	  { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ4 }, { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ5 },
	  { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ6 }, { DDRP_A_L_TRAIN_MAX_FOR_RD_DQ7 } },
	{ { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ0 }, { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ1 },
	  { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ2 }, { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ3 },
	  { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ4 }, { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ5 },
	  { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ6 }, { DDRP_A_H_TRAIN_MAX_FOR_RD_DQ7 } },
};

/* DQS/DQSB 的窗口与最佳点（每 byte 一份） */
static const struct ddrp_lane_reg s_rd_min_dqs[2] = {
	{ DDRP_A_L_TRAIN_MIN_FOR_RD_DQS }, { DDRP_A_H_TRAIN_MIN_FOR_RD_DQS },
};
static const struct ddrp_lane_reg s_rd_max_dqs[2] = {
	{ DDRP_A_L_TRAIN_MAX_FOR_RD_DQS }, { DDRP_A_H_TRAIN_MAX_FOR_RD_DQS },
};
static const struct ddrp_lane_reg s_rd_dqs_base[2] = {
	{ DDRP_A_L_TRAIN_RESULT_FOR_RD_BASE_DQS },
	{ DDRP_A_H_TRAIN_RESULT_FOR_RD_BASE_DQS },
};
/* 1 = 该 byte 找不到通过窗口（窗口无效，训练结果不可信） */
static const struct ddrp_lane_reg s_rd_no_window[2] = {
	{ DDRP_A_L_CHANGE_RD_DQS_DEFAULT },
	{ DDRP_A_H_CHANGE_RD_DQS_DEFAULT },
};

/*
 * 窗口是否撞到延迟线边界（手册："the flag that the DQS Rx delay line has
 * reached the {min,max} value when doing the read training of byteN"）。
 *
 * ⚠️ 这两个标志用来区分两种**看起来一样**的失败：
 *   · left 置位 -> 扫描撞到最小延迟，结果里的 min 会是 0；
 *   · right 置位 -> 撞到最大延迟，max 会是 0x7f(127)。
 * 现场遇到过 "min=0 max=124 width=124" 这种"看着有窗口"的打印 ——
 * 其实左边界在延迟线之外，是**被截断的假窗口**，不能用。
 */
static const struct ddrp_lane_reg s_rd_left_ovf[2] = {
	{ DDRP_A_L_LEFT_BOUNDARY_OVERFLOW_FOR_RD },
	{ DDRP_A_H_LEFT_BOUNDARY_OVERFLOW_FOR_RD },
};
static const struct ddrp_lane_reg s_rd_right_ovf[2] = {
	{ DDRP_A_L_RIGHT_BOUNDARY_OVERFLOW_FOR_RD },
	{ DDRP_A_H_RIGHT_BOUNDARY_OVERFLOW_FOR_RD },
};

/* 上一次训练的 error 位（reg_train_error_for_rd_byte，0x1c4[26:18]） */
static uint32_t s_rd_train_err;

/*
 * 按 rank 缓存的训练快照。
 * ⚠️ PHY 的 Table 47 结果寄存器**只有一份**（存的是"刚训完的那个 rank"），
 *    双 rank 时后一次会把前一次覆盖掉 —— ddrp_read_train_get() 只能拿到
 *    最后一次的。所以每训完一个 rank 就立刻抓一份快照，这样：
 *      · 两个 rank 的 per-bit skew 可以逐个取回、放到一起对比；
 *      · 某个 rank 训失败时，它"训到哪儿"的窗口也留得下来，便于诊断。
 * valid 表示该 rank 训过（不代表训成功，成败看 err）。
 */
struct rd_train_cache_ent {
	unsigned int valid;		/* 1 = 这个 rank 有快照 */
	unsigned int err;		/* 训练时的 error 位（0 = 无错） */
	struct ddrp_rd_train_result byte[DDRP_TRAIN_MAX_BYTE];
};
static struct rd_train_cache_ent s_rd_train_cache[DDRP_TRAIN_MAX_RANK];

/* X3000 的 byte 数：bus_width/8，上限 2（本 PHY 数据通路最大 16bit） */
static unsigned int rd_train_nbyte(void)
{
	unsigned int n = 2u;

	if (g_ddr_param && g_ddr_param->h.bus_width)
		n = g_ddr_param->h.bus_width / 8u;
	if (n < 1u)
		n = 1u;
	if (n > 2u)
		n = 2u;
	return n;
}

/* ==================== 读训练结果 → 打印 + bypass 回写 ==================== */

/*
 * DQS 的结果（每 byte 一份，手册 Table 47）：
 *   min/max = 通过区间 —— **只有 reg_dqs_rd_train_en=1（DQS-DQ Eye 训练）时才更新**；
 *             该位为 0 时它们保持复位 127/0，**不代表** no_window；
 *   base    = PHY 算出的最佳点（RO，复位 0）。
 * ⚠️ 这三张表（`s_rd_min_dqs` / `s_rd_max_dqs` / `s_rd_dqs_base`）在文件前面
 *    "Read Training" 段已经定义过，这里**不要重复定义**。
 */

/*
 * 读训练收尾：**先退出自动训练（dq_rd_train_en = 0）**，再把结果打印出来，
 * 并用 **bypass** 把值写进 DQ/DQS 的 Rx 延迟线。
 *
 * 为什么走 bypass 回写（手册 Table 27 的第一条路径 "Register (Bypass Read
 * Training)"）：Table 27 里让读训练结果生效只有两条路 ——
 *   · `reg_rd_train_perdef_en = 1` —— **本 IP 不支持**（字段原文
 *     "1: Not supported in this design"）；
 *   · 让 `reg_dq_rd_train_en` **一直保持 1** —— 结果会生效，但 PHY 会一直挂在
 *     训练模式（旧代码就是这么干的）。
 * 所以这里走最干净的一条：退出自动训练 → 把值写进
 * `reg_*_cs*_{dq0..7,dqs,dqsb}_invdelayselrx` → 用 `rd_train_freq_update` 的
 * **上升沿**刷进 PHY（`rx_dq_commit()` 即此流程，`ddrp_rx_dq_update()` 就是它）。
 *
 * 写什么 / 什么不写：
 *   · DQS 与 DQSB（必须同值）→ PHY 给的 **best point**；
 *   · 每根 DQ → 该 DQ 通过区间的**中点**；
 *   · ⚠️ **不可信的一律不写、保持原值**：可信 = 该 byte 的
 *     `no_window / left_ovf / right_ovf` 全为 0，且该 DQ 满足 `min < max <= 127`。
 *     被延迟线边界截断的"假窗口"（min 贴 0 或 max 贴 127）拿去回写相位必然错。
 *
 * ⚠️ 必须在**训练刚结束、结果寄存器还没被下一个 rank 覆盖**时调用。
 * 返回 DDR_OK / DDR_ERR_PARAM。
 */
int ddrp_read_train_apply(unsigned int rank)
{
	unsigned int nbyte = rd_train_nbyte();
	unsigned int lane, d;
	unsigned int nwrite = 0u, nkeep = 0u;

	if (rank > 1u || nbyte == 0u)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);

	/* 退出自动训练：此后生效的是 register 里的相位值（Table 27 第一条路径） */
	ddrp_write_reg(DDRP_DQ_RD_TRAIN_EN, 0u);
	ddrp_write_reg(DDRP_RD_TRAIN_PERDEF_EN, 0u);

	ddr_platform_log("---- read train apply rank%d (bypass) ----\n", rank);

	for (lane = 0; lane < nbyte; lane++) {
		unsigned int no_win = lane_read(&s_rd_no_window[lane]);
		unsigned int l_ovf = lane_read(&s_rd_left_ovf[lane]);
		unsigned int r_ovf = lane_read(&s_rd_right_ovf[lane]);
		unsigned int dqs_mn = lane_read(&s_rd_min_dqs[lane]);
		unsigned int dqs_mx = lane_read(&s_rd_max_dqs[lane]);
		unsigned int dqs_best = lane_read(&s_rd_dqs_base[lane]);
		/*
		 * ⚠️ 写入判据分两档（**别用 rd_win_usable()**，它是给"能不能当可信
		 *    结果报告"用的，比这里严）：
		 *   · `no_window`（change_rd_dqs_default = 1）= 真的没找到通过窗口
		 *     ⇒ **不写**、保持原值（这时 base/min/max 都是兜底值）；
		 *   · `left_ovf`/`right_ovf`（撞延迟线边界）= 窗口被端点截断 ⇒ 只**提示**，
		 *     不阻止写入。现场就是这样：DQS 基准偏在延迟线高端、DQ 的 min 全贴 0
		 *     （left_ovf=1），但通过区间的**中点依然是合理的采样相位**；
		 *     若按"贴边就不写"，等于训练完什么都不写、相位退回旧值 = 白训。
		 */
		unsigned int dqs_ok = (!no_win);

		/*
		 * DQS 的 min/max **只有 reg_dqs_rd_train_en=1（DQS-DQ Eye 训练）
		 * 时才会更新**；该位为 0 时它们保持复位 127/0，打印出来只为对照，
		 * **不代表** no_window（后者专指 change_rd_dqs_default）。
		 */
		ddr_platform_log("  byte%d dqs: min=%d max=%d best=%d"
				 " dqs_scan_en=%d no_window=%d"
				 " left_ovf=%d right_ovf=%d\n",
				 lane, dqs_mn, dqs_mx, dqs_best,
				 ddrp_read_reg(DDRP_DQS_RD_TRAIN_EN) & 0x1u,
				 no_win, l_ovf, r_ovf);
		if (dqs_ok) {
			ddrp_set_rx_dqs_delay(lane, rank, dqs_best);
			if (l_ovf || r_ovf)
				ddr_platform_log("    -> write dqs=%d (dqsb same)"
						 "  [window clipped]\n", dqs_best);
			else
				ddr_platform_log("    -> write dqs=%d (dqsb same)\n",
						 dqs_best);
			nwrite++;
		} else {
			ddr_platform_log("    -> no_window: keep old dqs\n");
			nkeep++;
		}

		ddr_platform_log("    dq mid:");
		for (d = 0; d < 8u; d++) {
			unsigned int mn = lane_read(&s_rd_min_dq[lane][d]);
			unsigned int mx = lane_read(&s_rd_max_dq[lane][d]);

			/*
			 * 只要有**真实通过区间**（min < max <= 127）就用它的中点；
			 * `min=127 / max=0` 是"该 DQ 一个通过点都没有"的复位值 ⇒ 不写。
			 */
			if (mx > mn && mx <= 127u) {
				unsigned int mid = mn + (mx - mn) / 2u;

				ddrp_set_rx_dq_delay(lane, rank, d, mid);
				ddr_platform_log(" %d", mid);
				nwrite++;
			} else {
				ddr_platform_log(" x");
				nkeep++;
			}
		}
		ddr_platform_log("   (x = no pass window, keep old)\n");

		/*
		 * ⭐ 边界溢出提示（逐 DQ）。
		 *
		 * `left/right_boundary_overflow_for_rd` 是 **per-byte** 的（整个 byte
		 * 共用一组），所以它影响的是这个 byte **全部** DQ 的相位可信度，
		 * 这里逐根提示一遍，免得只在这一行上面那个汇总行里被忽略。
		 *
		 * 含义（手册原话）：扫描**撞到延迟线 {min,max} 端时仍在 pass**
		 * ⇒ 窗口的**真实边沿在延迟线之外**，所以我们看到的 `min`（=0）或
		 * `max`（=127）是**被截断的假边沿**，上面按可见区间取的中点
		 * **不保证是真实窗口中心**（低频余量大时通常仍可用）。
		 * 取值逻辑**不因此改变**（仍取 `mn + (mx-mn)/2`，它对任意窗宽都安全）。
		 */
		if (l_ovf || r_ovf) {
			unsigned int dd;

			for (dd = 0u; dd < 8u; dd++) {
				if (l_ovf)
					ddr_platform_log("  byte%d dq%d: left_ovf"
							 " (window truncated)\n",
							 lane, dd);
				if (r_ovf)
					ddr_platform_log("  byte%d dq%d: right_ovf"
							 " (window truncated)\n",
							 lane, dd);
			}
		}
	}

	ddrp_rx_dq_update(rank);
	ddr_platform_log("  applied: %d values written, %d kept\n",
			 nwrite, nkeep);
	return (set_err(DDR_OK), DDR_OK);
}

/*
 * 把"当前 PHY 里的训练结果"抓一份到 s_rd_train_cache[rank]。
 * 必须在训练刚结束时立刻调用 —— 结果寄存器马上会被下一次训练覆盖。
 */
static void rd_train_snapshot(unsigned int rank, unsigned int err)
{
	unsigned int n;

	if (rank >= DDRP_TRAIN_MAX_RANK)
		return;

	n = ddrp_read_train_get(s_rd_train_cache[rank].byte,
				DDRP_TRAIN_MAX_BYTE);
	s_rd_train_cache[rank].err = err;
	s_rd_train_cache[rank].valid = n ? 1u : 0u;
}

int ddrp_read_train(unsigned int rank, uint32_t timeout_us)
{
	uint32_t want = ddrp_read_reg(DDRP_CHANNEL_EN) & 0x1ffu;
	uint32_t err;
	int ret;

	if (rank > 1u)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	if (!timeout_us)
		timeout_us = 1000000u;	/* 默认 1s：训练要发 refresh，比较久 */

	if (want == 0u)
		want = 0x1u;

	/*
	 * 0) 训练前置条件：**放在函数内部**，不依赖调用方（ddr.c）记得配。
	 *
	 * 为什么必须在这里再配一次：
	 *   · 检查图案（reg_*_rdtrain_check_wrap + en）—— 以前只有
	 *     ddrp_apply_pinmap() 会写，调用方一旦没调它，图案停在复位 0，
	 *     每根 DQ 都拿 pattern0 比 ⇒ **全表无窗口**；
	 *   · 训练逻辑时钟保持（Table 27 的两条 register 路径都要求
	 *     reg_train_reg_update_en = 1；读训练本身不强制，但开着无害）；
	 * 注：DDR4 训练期间进 MPR / 读前导（MR3/MR4）由 PHY 按训练流程自己
	 *     载入、训练结束自己退出，本驱动不再写 reg_ddr4_mr3/mr4。
	 */
	rdtrain_check_pattern_apply();
	ddrp_write_reg(DDRP_TRAIN_REG_UPDATE_EN, 1u);

	/*
	 * 1) PHY auto-refresh：训练期间 PHY 需要自己插 refresh，
	 *    否则数据在涨、训练窗口会漂。TREFI/TRFC 用复位默认即可
	 *    （0x0b8 复位值 0x960/0x8c），这里只开使能。
	 */
	ddrp_write_reg(DDRP_PHY_REFRESH_EN, 1u);

	/* 2) 选 rank。注意 0 是非法值（不选任何 rank），必须显式给 10/01 */
	ddrp_write_reg(DDRP_RDTRAIN_CS_SEL, rank ? 1u : 2u);

	/* 清训练计数器 + 上一次的 error，避免读到脏状态 */
	ddrp_write_reg(DDRP_RTRAIN_CNT_CLEAR, 1u);
	ddrp_write_reg(DDRP_RTRAIN_CNT_CLEAR, 0u);
	s_rd_train_err = 0u;

	/*
	 * 2b) 扫描范围 —— ⚠️ read training 的 range **有两组寄存器，两组都要设**：
	 *
	 *  (A) 全局组（0x0a4，手册 6.3.1 Table 46 "Read Training Registers"）：
	 *   · reg_rd_train_dq_scan_max（[30:24]，复位 **0x3f**）：DQ 扫描上限，
	 *     "the read training will stop the scan when the delay line reaches the max value"。
	 *     DQ 的 Rx 延迟线是 7 位（0~0x7f，`train_{min,max}_for_rd_dq` 复位值就是 127/0）
	 *     ⇒ 默认 0x3f 只扫**下半程**，最优相位落在上半段时表现为"没窗口/窗口贴边"。
	 *   · reg_rd_train_dqs_scan_max（[22:16]，复位 0x3f）+ reg_rd_train_dqs_range_bypass
	 *     （[6]，复位 0）：bypass=1 才认寄存器里的 range；=0 用内部默认 range，
	 *     而这个"默认 range"两处文档**互相矛盾**（Excel 表 [0~6'h3f]、PDF p81 [0~7'h7f]）。
	 *
	 *  (B) per-byte 组（0x258 / 0x358）—— **这组以前完全没设过（一直是复位值）**：
	 *   · reg_a_l/h_rd_train_dqs_range_min（[6:0]，复位 **0x0**）
	 *   · reg_a_l/h_rd_train_dqs_range_max（[14:8]，复位 **0x3f**）
	 *   · reg_a_l/h_rd_train_dqs_default（[30:24]，复位 **0x1f**）= **扫描起点**
	 *     （"the read training will **start based on this value**"）—— 这里**保持不动**，
	 *     改它会改变扫描行为与收敛过程，要试单独试。
	 *   手册正文只写了 (A)，(B) 只出现在寄存器表里、两组到底谁生效没有权威说明
	 *   ⇒ **两组按同一范围设**，不管 mux 选哪个都是我们要的 0~0x7f。
	 *   ⭐ 线索：之前实测 byte1 的 `train_result_for_rd_base_dqs` 恒为 31 = **0x1f**，
	 *      正好等于 (B) 的 `rd_train_dqs_default` 复位值 ⇒ byte1 的 DQS 相位压根没被动过。
	 *
	 *  ⚠️ 另外：DQS 扫描本身要 **reg_dqs_rd_train_en（0x0a4[1]，复位 0）= 1** 才做，
	 *     该位为 0 时以上 DQS 的 range 寄存器都不起作用（手册标 "Not suggested"，
	 *     本文件默认不开）。DQ 那部分不受影响。
	 */
	ddrp_write_reg(DDRP_RD_TRAIN_DQ_SCAN_MAX, 0x7fu);
	ddrp_write_reg(DDRP_RD_TRAIN_DQS_RANGE_BYPASS, 1u);
	ddrp_write_reg(DDRP_RD_TRAIN_DQS_SCAN_MAX, 0x7fu);
	/* (B) per-byte：X3000 只有 byte0(A_l) / byte1(A_h) 两组，直接按宏写清楚 */
	ddrp_write_reg(DDRP_A_L_RD_TRAIN_DQS_RANGE_MIN, 0x00u);
	ddrp_write_reg(DDRP_A_L_RD_TRAIN_DQS_RANGE_MAX, 0x7fu);
	ddrp_write_reg(DDRP_A_H_RD_TRAIN_DQS_RANGE_MIN, 0x00u);
	ddrp_write_reg(DDRP_A_H_RD_TRAIN_DQS_RANGE_MAX, 0x7fu);

	/*
	 * 3) 启动。DQS_RD_TRAIN_EN 保持 0 = 只做 DQ per-bit skew 训练；
	 *    手册注明"同时做 DQS-DQ Eye 训练"是 Not suggested，所以不开。
	 */
	ddrp_write_reg(DDRP_DQS_RD_TRAIN_EN, 0u);
	ddrp_write_reg(DDRP_DQ_RD_TRAIN_EN, 1u);

	/* 5) 等 train_true_done */
	ret = wait_field(DDRP_TRAIN_TRUE_DONE, 1u, timeout_us);

	/*
	 * 5b) 训练收尾：**退出自动训练 + 打印结果 + 用 bypass 把最优相位写进
	 *     DQ/DQS 的 Rx 延迟线**（函数内部第一件事就是写 dq_rd_train_en = 0）。
	 *     ⚠️ 必须在这里调用 —— 结果寄存器只有一份，训下一个 rank 就覆盖了。
	 */
	if (ddrp_read_train_apply(rank) != DDR_OK)
		ddr_platform_log("read train rank%d: apply failed\n", rank);

	/* 6) 判成败：error 是每 byte 一位 */
	err = ddrp_read_reg(DDRP_TRAIN_ERROR_FOR_RD_BYTE) & 0x1ffu;
	s_rd_train_err = err;
	/*
	 * 立刻按 rank 抓快照 —— 结果寄存器只有一份，接着训下一个 rank 就没了。
	 * 失败时**也要抓**：留下"训到哪儿"的窗口，比什么都没有好诊断。
	 */
	rd_train_snapshot(rank, err);

	/*
	 * ⭐ 把"训练结果能不能被 PHY 采用"的三个开关打出来。
	 *
	 * 依据手册 **Table 30（Two Path to Control Rx Data Per-bit De-skew）**：
	 *   "Using the read training result" 的条件是
	 *        reg_dq_rd_train_en = 1   **或**   reg_rd_train_perdef_en = 1
	 *   而另一条路径（用寄存器直接写相位）要求
	 *        reg_dq_rd_train_en = 0 且 reg_rd_train_perdef_en = 0
	 *        + reg_train_reg_update_en = 1 + reg_rd_train_freq_update 上升沿
	 *
	 * 手册 Figure 19 又要求训练结束时 `reg_dq_rd_train_en = 0`（退出训练）。
	 * 两者合起来看：**退出训练后，如果 perdef_en 也是 0，那两条路径的
	 * 条件都不满足** —— 训练结果是否仍然起作用，就变成必须实测确认的事。
	 * 以前这里不打这几个开关，出了问题只能猜；现在直接摊出来。
	 *
	 * 判读：en=0 且 perdef_en=0 时，若读数据不对，就在这里下手
	 *       （把 perdef_en 置 1，或让 dq_rd_train_en 保持 1）。
	 */
	ddr_platform_log("read train rank%d: switches after exit: "
			 "dq_rd_train_en=%d rd_train_perdef_en=%d "
			 "train_reg_update_en=%d\n", rank,
			 ddrp_read_reg(DDRP_DQ_RD_TRAIN_EN) & 0x1u,
			 ddrp_read_reg(DDRP_RD_TRAIN_PERDEF_EN) & 0x1u,
			 ddrp_read_reg(DDRP_TRAIN_REG_UPDATE_EN) & 0x1u);

	/* 结果打印 + bypass 回写已经在 5b) 的 ddrp_read_train_apply() 里做完 */

	/*
	 * ⚠️ 这里**不再内部 dump 结果** —— 以前三处 return 前各调了一次
	 * ddrp_dump_read_train_result()，而调用方（ddr.c 里训练完又会调
	 * ddrp_dump_read_train_all_ranks / ddrp_dump_read_train_result）
	 * 再 dump 一遍，于是日志里同一份结果**出现两次**。
	 * 现在只报状态，要不要打结果由调用方决定。
	 */
	if (ret != DDR_OK) {
		ddr_platform_log("read train rank%d: TIMEOUT (train_true_done not"
				 " set), err=0x%x\n", rank, err);
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	}
	if (err & want) {
		ddr_platform_log("read train rank%d: BYTE ERROR err=0x%x"
				 " (expect 0x%x)\n", rank, err, want);
		return (set_err(DDR_ERR_FAIL), DDR_ERR_FAIL);
	}

	ddr_platform_log("read train rank%d: done, err=0x%x\n", rank, err);
	return (set_err(DDR_OK), DDR_OK);
}

unsigned int ddrp_read_train_get(struct ddrp_rd_train_result *res,
				 unsigned int nres)
{
	unsigned int n = rd_train_nbyte();
	unsigned int b, d;

	if (!res || !nres)
		return 0u;
	if (n > nres)
		n = nres;

	for (b = 0; b < n; b++) {
		for (d = 0; d < 8u; d++) {
			res[b].min_dq[d] = lane_read(&s_rd_min_dq[b][d]);
			res[b].max_dq[d] = lane_read(&s_rd_max_dq[b][d]);
		}
		res[b].min_dqs = lane_read(&s_rd_min_dqs[b]);
		res[b].max_dqs = lane_read(&s_rd_max_dqs[b]);
		res[b].dqs = lane_read(&s_rd_dqs_base[b]);
		res[b].no_window = lane_read(&s_rd_no_window[b]) ? 1u : 0u;
		res[b].left_ovf = lane_read(&s_rd_left_ovf[b]) ? 1u : 0u;
		res[b].right_ovf = lane_read(&s_rd_right_ovf[b]) ? 1u : 0u;
	}
	return n;
}

/*
 * 判定一个训练窗口是否**有效**。
 *
 * ⚠️ 不能只看 width：PHY 在"这个 DQ 找不到任何通过点"时，会把
 *    min 写成延迟线最大值(127)、max 写成 0 —— 于是 min > max，
 *    (max - min) 在无符号下还会回绕成一个很大的数。
 *    所以判据是 **min < max 且 max 落在 7 位延迟线范围内(<=127)**。
 *    （实测无窗口时打印出来正是 "min=127 max=0"。）
 */
static unsigned int win_ok(unsigned int mn, unsigned int mx)
{
	return (mx > mn) && (mx <= 127u);
}

/*
 * 窗口是否**真正可用** —— 三个条件缺一不可：
 *   ① PHY 没报 no_window（reg_*_change_rd_dqs_default = 0）
 *   ② 没撞延迟线边界（left/right_boundary_overflow_for_rd = 0）
 *   ③ min < max 且 max 落在 7 位范围内
 *
 * ⚠️ 为什么要全部纳入：现场出现过 **"mid=0 width=124"** 这种自相矛盾的
 *    打印 —— 因为 no_window 置位时 dq_mid() 返回 0，而 dq_width() 只看
 *    min/max 就返回了 124。更要命的是 "min=0 max=124" 其实是
 *    **左边界溢出**（扫描撞到延迟线最小值），窗口是**被截断的假窗口**，
 *    这种值拿去回写相位必然错。现在全部走这一个判据。
 */
static unsigned int rd_win_usable(const struct ddrp_rd_train_result *e,
				  unsigned int d)
{
	return !e->no_window && !e->left_ovf && !e->right_ovf &&
		win_ok(e->min_dq[d], e->max_dq[d]);
}

void ddrp_dump_read_train_result(void)
{
#if DDRP_DBG_TRAIN
	struct ddrp_rd_train_result r[2];
	unsigned int n = ddrp_read_train_get(r, 2u);
	unsigned int b, d;
	unsigned int nok = 0u, nbad = 0u;

	/*
	 * NOTE: target u-boot printf only reliably supports %d / %x.
	 * %u and width modifiers like %03x are NOT recognised AND do not
	 * consume their argument, shifting every following argument
	 * (symptom: %s prints "(null)", the format string prints literally).
	 * Non-ASCII garbles too -> keep logs ASCII-only, %d/%x only.
	 * 也别用 %-3d / %3d 这类宽度修饰（同样不认）。
	 */
	ddr_platform_log("---- Read training result (Table 47, Rx delay line) ----\n");
	if (!n) {
		ddr_platform_log("  (no usable byte - check bus_width/CHANNEL_EN)\n");
		return;
	}

	for (b = 0; b < n; b++) {
		if (b == 0)
			ddr_platform_log("  byte0 (A_l/low)");
		else
			ddr_platform_log("  byte1 (A_h/high)");
		ddr_platform_log(": DQS min=%d max=%d best=%d",
				 r[b].min_dqs, r[b].max_dqs, r[b].dqs);
		if (!win_ok(r[b].min_dqs, r[b].max_dqs))
			ddr_platform_log("   ** DQS WINDOW INVALID **");
		ddr_platform_log("\n");

		/*
		 * 先报"边界溢出"和 PHY 自己的 no_window 标志。
		 * ⚠️ 这两个是判读后面的 min/max 的前提：
		 *    left_ovf 置位时 min 一定是 0（扫描撞到延迟线最小端），
		 *    看着像"窗口从 0 开始"，其实是**被截断的假窗口**。
		 */
		ddr_platform_log("    flags: phy_no_window=%d left_ovf=%d"
				 " right_ovf=%d\n", r[b].no_window,
				 r[b].left_ovf, r[b].right_ovf);
		if (r[b].left_ovf && r[b].right_ovf)
			ddr_platform_log("    ** WHOLE DELAY LINE PASSES **"
					 " (both boundaries overflowed)"
					 " -> pattern/phase likely wrong\n");
		else if (r[b].left_ovf)
			ddr_platform_log("    ** LEFT BOUNDARY OVERFLOW **"
					 " (min hit 0; window is clipped,"
					 " 'min' is NOT the real edge)\n");
		else if (r[b].right_ovf)
			ddr_platform_log("    ** RIGHT BOUNDARY OVERFLOW **"
					 " (max hit 127; window is clipped)\n");

		/*
		 * 逐 DQ 打 min/max/width。判读：
		 *   usable           -> 有窗口，width 越大裕量越好
		 *   min > max（127..0）-> **该 DQ 没有任何通过点**
		 *   有溢出标志       -> min/max 被边界截断，不可信
		 */
		for (d = 0; d < 8u; d++) {
			unsigned int mn = r[b].min_dq[d];
			unsigned int mx = r[b].max_dq[d];

			if (rd_win_usable(&r[b], d)) {
				ddr_platform_log("    DQ%d min=%d max=%d width=%d%s\n",
						 d, mn, mx, mx - mn,
						 (mx - mn) < 16u ?
						 "  (narrow, margin low)" : "");
				nok++;
			} else if (win_ok(mn, mx)) {
				/* min/max 有值但 PHY 说不可用 -> 值不可信 */
				ddr_platform_log("    DQ%d min=%d max=%d  "
						 "** UNUSABLE **"
						 " (window clipped or"
						 " no_window set)\n", d, mn, mx);
				nbad++;
			} else {
				ddr_platform_log("    DQ%d min=%d max=%d  "
						 "** NO WINDOW **\n", d, mn, mx);
				nbad++;
			}
		}
	}

	ddr_platform_log("  summary: %d DQ with window, %d DQ without\n",
			 nok, nbad);
	/*
	 * ⚠️ 一定要区分这两种"NO WINDOW" —— 现象一样，原因完全不同：
	 *   (a) 训练**压根没生效**：PHY 失败时不回写结果寄存器，于是打印出来
	 *       就是**复位值**。结果寄存器的复位值（手册寄存器表）：
	 *         读: reg_*_train_min_for_rd_dq* = 0x7f(127)
	 *             reg_*_train_max_for_rd_dq* = 0x0
	 *       所以看到 "min=127 max=0" 就说明**一个字都没被写过**。
	 *   (b) 训练跑了但窗口确实窄/没有。
	 * 这里直接把 (a) 标出来，免得围着"窗口"绕。
	 *
	 * NOTE: u-boot 的 printf 只稳支持 %d/%x，且**非 ASCII 会乱码**，
	 *       所以本文件所有日志必须纯英文（以前写过中文提示，实际全乱码）。
	 */
	if (nok == 0u && nbad != 0u)
		ddr_platform_log("  NOTE: min/max == RESET values (min=127 max=0)"
				 " -> PHY never wrote any result,\n"
				 "        i.e. training did NOT take effect at all"
				 " (this is NOT a narrow-window case)\n");

	if (nbad) {
		ddr_platform_log("  NO WINDOW checklist (preconditions first):\n"
				 "    1) rdtrain_check_wrap / rd_train_check_value_en:\n"
				 "       if wrap stays at reset(0), every DQ is compared"
				 " against pattern0,\n"
				 "       but SDRAM returns DQk -> Pattern(k mod 4)"
				 " (manual Table 47) => never matches.\n"
				 "       Direct wiring needs wrap0 = wrap1 = 0xE4.\n"
				 "    2) Is SDRAM init done? DDR4 read-training uses MPR"
				 " commands,\n"
				 "       DRAM must be up (manual 3.4: Step 8 before Step 9).\n"
				 "    3) Is Rx DQS gating valid? (see the [Rx-DQS] print)\n"
				 "    4) only then look at DQ trace / Vref / drive strength.\n"
				 "  rule: ALL bytes & DQs dead = precondition problem;\n"
				 "        a SINGLE DQ dead = that one trace.\n");
	}
#endif
}

/* ==================== 多 rank：per-bit skew 训练与对比 ==================== */
void ddrp_read_train_cache_clear(void)
{
	unsigned int r, b, d;

	for (r = 0; r < DDRP_TRAIN_MAX_RANK; r++) {
		struct rd_train_cache_ent *c = &s_rd_train_cache[r];

		c->valid = 0u;
		c->err = 0u;
		for (b = 0; b < DDRP_TRAIN_MAX_BYTE; b++) {
			struct ddrp_rd_train_result *e = &c->byte[b];

			for (d = 0; d < 8u; d++) {
				e->min_dq[d] = 0u;
				e->max_dq[d] = 0u;
			}
			e->min_dqs = 0u;
			e->max_dqs = 0u;
			e->dqs = 0u;
			e->no_window = 0u;
		}
	}
}

int ddrp_read_train_all_ranks(unsigned int nrank, uint32_t timeout_us)
{
	unsigned int r;
	int ret;
	int first_err = DDR_OK;

	if (nrank < 1u || nrank > DDRP_TRAIN_MAX_RANK)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);

	/*
	 * 逐个 rank 训。**某个 rank 失败也继续训下一个** —— 目的就是把两边的
	 * 快照都拿到：若只有 rank1 失败，基本可断定是 rank1 自己的走线 / ODT
	 * 问题，而不是公共通道（DQ/DQS 走线、PLL）的问题。
	 */
	for (r = 0; r < nrank; r++) {
		ret = ddrp_read_train(r, timeout_us);
		if (ret != DDR_OK && first_err == DDR_OK)
			first_err = ret;
	}
	return (set_err(first_err), first_err);
}

unsigned int ddrp_read_train_get_rank(unsigned int rank,
				      struct ddrp_rd_train_result *res,
				      unsigned int nres)
{
	unsigned int n, b;

	if (rank >= DDRP_TRAIN_MAX_RANK || !res || !nres)
		return 0u;
	if (!s_rd_train_cache[rank].valid)
		return 0u;

	n = rd_train_nbyte();
	if (n > nres)
		n = nres;
	if (n > DDRP_TRAIN_MAX_BYTE)
		n = DDRP_TRAIN_MAX_BYTE;

	for (b = 0; b < n; b++)
		res[b] = s_rd_train_cache[rank].byte[b];
	return n;
}

/*
 * 窗口中点 —— PHY 只报 min/max，per-bit 的"最佳点"就是窗口中心。
 * 窗口不可用时返回 0（调用方会标注原因，别把 0 当成真实中点）。
 */
static unsigned int dq_mid(const struct ddrp_rd_train_result *e, unsigned int d)
{
	unsigned int mn = e->min_dq[d], mx = e->max_dq[d];

	if (!rd_win_usable(e, d))
		return 0u;
	return mn + (mx - mn) / 2u;
}

/*
 * 窗口宽度 —— 判据必须和 dq_mid() 一致：
 * 无窗口时 PHY 给的是 min=127 / max=0，直接做 (max - min) 会在
 * unsigned 下**回绕成一个巨大的数**（0-127 = 4294967169），
 * 打印出来像是"窗口特别宽"，正好把结论看反。
 */
static unsigned int dq_width(const struct ddrp_rd_train_result *e, unsigned int d)
{
	unsigned int mn = e->min_dq[d], mx = e->max_dq[d];

	return rd_win_usable(e, d) ? (mx - mn) : 0u;
}

void ddrp_dump_read_train_all_ranks(void)
{
#if DDRP_DBG_TRAIN
	struct ddrp_rd_train_result r[DDRP_TRAIN_MAX_RANK][DDRP_TRAIN_MAX_BYTE];
	unsigned int got[DDRP_TRAIN_MAX_RANK];
	unsigned int nbyte = rd_train_nbyte();
	unsigned int i, b, d, nrank = 0u;

	ddr_platform_log("---- Read training per-rank compare (per-bit skew, Rx delay line) ----\n");

	for (i = 0; i < DDRP_TRAIN_MAX_RANK; i++) {
		got[i] = ddrp_read_train_get_rank(i, r[i], DDRP_TRAIN_MAX_BYTE);
		if (got[i])
			nrank++;
	}
	if (!nrank) {
		ddr_platform_log("  (cache empty - call ddrp_read_train_all_ranks first)\n");
		return;
	}

	for (b = 0; b < nbyte; b++) {
		unsigned int worst_d = 0u, worst_v = 0u;

		if (b == 0)
			ddr_platform_log("  byte0 (A_l/low)\n");
		else
			ddr_platform_log("  byte1 (A_h/high)\n");
		/* DQS 单独列：它的 best 是 PHY 直接给的，不是窗口中点 */
		for (i = 0; i < DDRP_TRAIN_MAX_RANK; i++) {
			if (!got[i])
				continue;
			ddr_platform_log("    rank%d DQS [%d..%d] best=%d\n", i,
					 r[i][b].min_dqs, r[i][b].max_dqs,
					 r[i][b].dqs);
			if (r[i][b].no_window)
				ddr_platform_log("      rank%d: no pass window\n", i);
			/*
			 * 边界溢出标志：能区分"窗口窄"和"窗口被延迟线边界截断"。
			 * left 置位 -> min 会是 0（左边界在延迟线之外）；
			 * right 置位 -> max 会是 127。任一置位都说明 min/max 不可信。
			 */
			if (r[i][b].left_ovf || r[i][b].right_ovf)
				ddr_platform_log("      rank%d: BOUNDARY OVERFLOW"
						 " (left=%d right=%d) -> min/max are"
						 " clipped, not real edges\n", i,
						 r[i][b].left_ovf,
						 r[i][b].right_ovf);
			if (s_rd_train_cache[i].err & (1u << b))
				ddr_platform_log("      rank%d: training error, byte%d\n",
						 i, b);
		}

		if (!got[0] || !got[1]) {
			/* 只有一个 rank 有快照：列出它自己的逐 DQ 窗口就够了 */
			unsigned int k = got[0] ? 0u : 1u;

			/*
			 * ⚠️ 这里**不能只打 mid/width**：窗口不可用（no_window 置位 / 撞到
			 *    延迟线边界）时 `dq_mid()`/`dq_width()` 会**一起返回 0**，打出来就是
			 *    一排水 "mid=0 width=0"，看着像"压根没读到数据"，其实是"有数据但被
			 *    判成不可用"（现场踩过：left_ovf=1 时 8 根 DQ 全打 mid=0 width=0，
			 *    而真实 min/max 是 0..119~126）。
			 *    所以这里和 ddrp_dump_read_train_result() 保持一致，改成三态打
			 *    **原始 min/max**：可用 -> min/max/width；有值但不可用 -> 标 UNUSABLE
			 *    并把溢出/no_window 原因带上；没值 -> NO WINDOW。
			 */
			for (d = 0; d < 8u; d++) {
				unsigned int mn = r[k][b].min_dq[d];
				unsigned int mx = r[k][b].max_dq[d];

				if (rd_win_usable(&r[k][b], d)) {
					ddr_platform_log("    DQ%d min=%d max=%d width=%d\n",
							 d, mn, mx, mx - mn);
					if ((mx - mn) < 16u)
						ddr_platform_log("      (narrow, margin low)\n");
				} else if (win_ok(mn, mx)) {
					ddr_platform_log("    DQ%d min=%d max=%d"
							 " ** UNUSABLE ** (left_ovf=%d"
							 " right_ovf=%d no_window=%d)\n",
							 d, mn, mx, r[k][b].left_ovf,
							 r[k][b].right_ovf, r[k][b].no_window);
				} else {
					ddr_platform_log("    DQ%d min=%d max=%d"
							 " ** NO WINDOW **\n", d, mn, mx);
				}
			}
			continue;
		}

		if (r[0][b].left_ovf || r[0][b].right_ovf ||
		    r[1][b].left_ovf || r[1][b].right_ovf)
			ddr_platform_log("    NOTE: mid/width=0 means"
					 " \"window unusable\" (clipped"
					 " /overflow) - read min/max in diag\n");
		ddr_platform_log("    DQ  rank0(mid/width)  rank1(mid/width)  diff\n");
		for (d = 0; d < 8u; d++) {
			unsigned int m0 = dq_mid(&r[0][b], d);
			unsigned int m1 = dq_mid(&r[1][b], d);
			unsigned int w0 = dq_width(&r[0][b], d);
			unsigned int w1 = dq_width(&r[1][b], d);
			unsigned int diff = (m1 > m0) ? (m1 - m0) : (m0 - m1);

			if (diff > worst_v) {
				worst_v = diff;
				worst_d = d;
			}
			/*
			 * 只用 %d / %x：目标 uboot printf 不认 %u / 宽度修饰
			 * （%3u、%-3u 会原样打印且不消费参数），也不用 %s。
			 */
			ddr_platform_log("    DQ%d  %d / %d  %d / %d  %d\n",
					 d, m0, w0, m1, w1, diff);
			if (w0 == 0u || w1 == 0u)
				ddr_platform_log("      no window on DQ%d "
						 "(w0=%d w1=%d)\n", d, w0, w1);
		}
		ddr_platform_log("    max cross-rank diff = %d (DQ%d)\n",
				 worst_v, worst_d);
		if (worst_v >= 8u)
			ddr_platform_log("      !! large diff: check trace length / "
					 "loading of both ranks,\n"
					 "         and rank1 ODT / drive strength\n");
	}
#endif
}

/* ==================== Write Training（手册 5.4 Auto Write Training） ==================== */
/*
 * 为什么需要它：读训练（5.3）校的是 PHY **Rx** 方向的 per-bit 延迟；
 * **Tx（发/写）方向**只有写均衡（5.2）定了 DQS 的基准相位，**DQ 之间的
 * per-bit skew 没人校** —— 这就是这个训练要干的。手册 5.4 原文：
 *   "When the write training is enabled, the PHY will adjust the per-bit
 *    phase tuning to change the delay of the TX DQ to find the optimal
 *    position. (The DQS will keep to the phase found by the write-leveling
 *    when it's enabled.)"
 * 调节范围 per-bit phase tuning = 6'h0 ~ 6'h3f。
 *
 * 手册 Figure 20 的流程（本函数按序实现）：
 *   ① check data：默认让 PHY 自己生成随机数据（reg_..._random_gen=1），
 *      省掉配 10*BL8 个 dq{0..7}_train_check_data_value{0..9}
 *   ② 选 rank   reg_wrtrain_cs_sel  2'b10=RANK0 / 2'b01=RANK1
 *   ③ 选 DQS 相位 reg_wr_train_dqs_default_bypass=0 -> 用 write-leveling 结果（手册推荐）
 *   ④ auto 模式 reg_dq_wr_train_auto = 1
 *   ⑤ 使能      reg_dq_wr_train_en   = 1
 *   ⑥ 等完成    train_all_step_done = 1（再核 wr_train_done_byte 覆盖 CHANNEL_EN）
 *   ⑦ 退出      reg_dq_wr_train_en   = 0
 *   ⑧ 读结果    reg_{a,b}_{l,h}_train_{min,max}_for_dq{0..7}
 *
 * **前提（顺序不能反）**：
 *   · SDRAM 必须已初始化完成 —— 手册 5.4：DDR3/DDR4/LPDDR3 是"用**普通读写
 *     命令**"完成写训练，DRAM 没起来就没有读写通路；
 *   · **写均衡必须先跑** —— 手册 5.4："It is suggested to complete the
 *     write-leveling before auto write training, so that the write training
 *     can use the write-leveling results as references."
 *     顺序：RxDQS gating -> WL -> Read training -> **Write training**
 *
 * Tx Vref 训练（step2）是 **LPDDR4/4X 专属**，本项目跑 DDR4，故
 * reg_train_vref_en 保持 0 —— 此时手册明说"write training only performs Step 1"。
 */

/* 每个 rank 的写训练结果快照（结果寄存器 A_l/A_h 各一份，双 rank 会互相覆盖） */
struct wt_result_ent {
	unsigned int done_byte;
	unsigned int err_byte;
	unsigned int step_err;		/* [2:0] = step3,step2,step1 */
	unsigned int min_dq[2][8];	/* [A_l/A_h][DQ0..7] */
	unsigned int max_dq[2][8];
	unsigned int valid;
};

static struct wt_result_ent s_wt[DDRP_TRAIN_MAX_RANK];
static unsigned int s_wt_nrank;

static const struct ddrp_lane_reg s_wt_min_dq[2][8] = {
	{ { DDRP_A_L_TRAIN_MIN_FOR_DQ0 }, { DDRP_A_L_TRAIN_MIN_FOR_DQ1 },
	  { DDRP_A_L_TRAIN_MIN_FOR_DQ2 }, { DDRP_A_L_TRAIN_MIN_FOR_DQ3 },
	  { DDRP_A_L_TRAIN_MIN_FOR_DQ4 }, { DDRP_A_L_TRAIN_MIN_FOR_DQ5 },
	  { DDRP_A_L_TRAIN_MIN_FOR_DQ6 }, { DDRP_A_L_TRAIN_MIN_FOR_DQ7 } },
	{ { DDRP_A_H_TRAIN_MIN_FOR_DQ0 }, { DDRP_A_H_TRAIN_MIN_FOR_DQ1 },
	  { DDRP_A_H_TRAIN_MIN_FOR_DQ2 }, { DDRP_A_H_TRAIN_MIN_FOR_DQ3 },
	  { DDRP_A_H_TRAIN_MIN_FOR_DQ4 }, { DDRP_A_H_TRAIN_MIN_FOR_DQ5 },
	  { DDRP_A_H_TRAIN_MIN_FOR_DQ6 }, { DDRP_A_H_TRAIN_MIN_FOR_DQ7 } },
};

static const struct ddrp_lane_reg s_wt_max_dq[2][8] = {
	{ { DDRP_A_L_TRAIN_MAX_FOR_DQ0 }, { DDRP_A_L_TRAIN_MAX_FOR_DQ1 },
	  { DDRP_A_L_TRAIN_MAX_FOR_DQ2 }, { DDRP_A_L_TRAIN_MAX_FOR_DQ3 },
	  { DDRP_A_L_TRAIN_MAX_FOR_DQ4 }, { DDRP_A_L_TRAIN_MAX_FOR_DQ5 },
	  { DDRP_A_L_TRAIN_MAX_FOR_DQ6 }, { DDRP_A_L_TRAIN_MAX_FOR_DQ7 } },
	{ { DDRP_A_H_TRAIN_MAX_FOR_DQ0 }, { DDRP_A_H_TRAIN_MAX_FOR_DQ1 },
	  { DDRP_A_H_TRAIN_MAX_FOR_DQ2 }, { DDRP_A_H_TRAIN_MAX_FOR_DQ3 },
	  { DDRP_A_H_TRAIN_MAX_FOR_DQ4 }, { DDRP_A_H_TRAIN_MAX_FOR_DQ5 },
	  { DDRP_A_H_TRAIN_MAX_FOR_DQ6 }, { DDRP_A_H_TRAIN_MAX_FOR_DQ7 } },
};

/* 抓当前结果到 s_wt[rank] —— 必须在训下一个 rank **之前**调 */
static void wt_snapshot(unsigned int rank)
{
	struct wt_result_ent *c;
	unsigned int b, d;

	if (rank >= DDRP_TRAIN_MAX_RANK)
		return;
	c = &s_wt[rank];
	c->done_byte = ddrp_read_reg(DDRP_WR_TRAIN_DONE_BYTE) & 0x1ffu;
	c->err_byte = ddrp_read_reg(DDRP_WR_TRAIN_ERROR_BYTE) & 0x1ffu;
	c->step_err = (ddrp_read_reg(DDRP_TRAIN_STEP1_ERROR) & 0x1u)
		| ((ddrp_read_reg(DDRP_TRAIN_STEP2_ERROR) & 0x1u) << 1)
		| ((ddrp_read_reg(DDRP_TRAIN_STEP3_ERROR) & 0x1u) << 2);
	for (b = 0; b < 2u; b++) {
		for (d = 0; d < 8u; d++) {
			c->min_dq[b][d] = lane_read(&s_wt_min_dq[b][d]);
			c->max_dq[b][d] = lane_read(&s_wt_max_dq[b][d]);
		}
	}
	c->valid = 1u;
	if (rank + 1u > s_wt_nrank)
		s_wt_nrank = rank + 1u;
}

int ddrp_write_train(unsigned int nrank, uint32_t timeout_us)
{
	unsigned int rank_num;
	uint32_t want = ddrp_read_reg(DDRP_CHANNEL_EN) & 0x1ffu;
	uint32_t chunk;
	unsigned int rank;
	int ret;

	if (nrank == 0u)
		nrank = g_ddr_param ? g_ddr_param->ddrp.rank_num : 1u;
	rank_num = (nrank > DDRP_TRAIN_MAX_RANK) ? DDRP_TRAIN_MAX_RANK : nrank;
	if (rank_num == 0u)
		rank_num = 1u;
	if (want == 0u)
		want = 0x1u;

	s_wt_nrank = 0u;
	for (rank = 0; rank < DDRP_TRAIN_MAX_RANK; rank++)
		s_wt[rank].valid = 0u;

	for (rank = 0; rank < rank_num; rank++) {
		/* ① 清 FSM，再从头配 */
		ddrp_write_reg(DDRP_DQ_WR_TRAIN_EN, 0u);
		ddrp_write_reg(DDRP_WR_TRAIN_RST, 1u);
		ddrp_write_reg(DDRP_WR_TRAIN_RST, 0u);

		/* ② 选 rank（使能期间不能设 2'b00，必须显式选） */
		ddrp_write_reg(DDRP_WRTRAIN_CS_SEL, rank ? 1u : 2u);

		/* ③ DQS 相位用 write-leveling 结果（手册推荐，0 = 用 WL 结果） */
		ddrp_write_reg(DDRP_WR_TRAIN_DQS_DEFAULT_BYPASS, 0u);
		ddrp_write_reg(DDRP_WR_TRAIN_DQS_RANGE_BYPASS, 0u);

		/* ① check data 由 PHY 随机生成（跳过 80 个 value 寄存器的配置） */
		ddrp_write_reg(DDRP_WRTRAIN_CHECK_DATA_VALUE_RANDOM_GEN, 1u);

		/* Tx Vref 训练是 LPDDR4 专属；DDR4 下明确关掉 -> 只跑 Step1 */
		ddrp_write_reg(DDRP_TRAIN_VREF_EN, 0u);

		/* ④ auto 模式 + ⑤ 使能 */
		ddrp_write_reg(DDRP_DQ_WR_TRAIN_AUTO, 1u);
		ddrp_write_reg(DDRP_DQ_WR_TRAIN_EN, 1u);

		/* ⑥ 等完成：先等 all_step_done，再核每 byte 的 done 位 */
		ret = wait_field(DDRP_TRAIN_ALL_STEP_DONE, 1u, timeout_us);
		if (ret == DDR_OK)
			ret = wait_field(DDRP_WR_TRAIN_DONE_BYTE, want, timeout_us);

		/* ⑦ 无论成败都退出 */
		ddrp_write_reg(DDRP_DQ_WR_TRAIN_EN, 0u);

		/* 抓快照：结果寄存器 A_l/A_h 各一份，训下一个 rank 就没了 */
		wt_snapshot(rank);

		chunk = ddrp_read_reg(DDRP_WR_TRAIN_ERROR_BYTE) & want;
		if (ret != DDR_OK) {
			ddr_platform_log("write train rank%d: TIMEOUT (all_step_done"
					 " not set), done_byte=0x%x err_byte=0x%x\n",
					 rank, s_wt[rank].done_byte,
					 s_wt[rank].err_byte);
			return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
		}
		if (chunk != 0u) {
			ddr_platform_log("write train rank%d: BYTE ERROR err_byte=0x%x"
					 " (expect 0x%x)\n", rank, chunk, want);
			return (set_err(DDR_ERR_FAIL), DDR_ERR_FAIL);
		}
	}
	return (set_err(DDR_OK), DDR_OK);
}

/*
 * 打印写训练结果。判读和读训练一样：
 *   min < max（且 <= 0x3f）-> 有效窗口，宽度越大 Tx 裕量越好
 *   min > max             -> 该 DQ 没有任何通过点（PHY 找不到窗口时的写法）
 * 寄存器是 9 位宽，但手册说 per-bit phase tuning 只有 6 位（6'h0~6'h3f），
 * 所以 > 0x3f 视为无效。
 */
static unsigned int wt_win_ok(unsigned int mn, unsigned int mx)
{
	return (mx > mn) && (mx <= 0x3fu);
}

void ddrp_dump_write_train_result(void)
{
#if DDRP_DBG_TRAIN
	struct wt_result_ent *c;
	unsigned int r, b, d, nok, nbad;

	if (s_wt_nrank == 0u) {
		ddr_platform_log("  (write train result empty - run"
				 " ddrp_write_train() first)\n");
		return;
	}
	ddr_platform_log("---- Write training result (Table 54, Tx delay line) ----\n");
	for (r = 0; r < s_wt_nrank; r++) {
		c = &s_wt[r];
		if (!c->valid)
			continue;
		nok = 0u;
		nbad = 0u;
		ddr_platform_log("  rank%d: done_byte=0x%x err_byte=0x%x"
				 " step_err=0x%x\n", r, c->done_byte,
				 c->err_byte, c->step_err);
		for (b = 0; b < 2u; b++) {
			if (b == 0)
				ddr_platform_log("    byte0 (A_l/low)\n");
			else
				ddr_platform_log("    byte1 (A_h/high)\n");
			for (d = 0; d < 8u; d++) {
				unsigned int mn = c->min_dq[b][d];
				unsigned int mx = c->max_dq[b][d];

				if (wt_win_ok(mn, mx)) {
					ddr_platform_log("      DQ%d min=%d max=%d"
							 " width=%d%s\n", d, mn, mx,
							 mx - mn,
							 (mx - mn) < 8u ?
							 "  (narrow)" : "");
					nok++;
				} else {
					ddr_platform_log("      DQ%d min=%d max=%d"
							 "  ** NO WINDOW **\n",
							 d, mn, mx);
					nbad++;
				}
			}
		}
		ddr_platform_log("    summary: %d DQ with window, %d without\n",
				 nok, nbad);
		if (nok == 0u && nbad != 0u)
			ddr_platform_log("    NOTE: values equal RESET"
					 " (min_for_dq=0x1ff) -> PHY never wrote results,\n"
					 "          training did NOT take effect.\n");
		if (nbad) {
			ddr_platform_log("    NO WINDOW checklist"
					 " (preconditions first):\n"
					 "      1) Was write leveling done? DQS base phase"
					 " comes from it\n"
					 "         (reg_wr_train_dqs_default_bypass=0).\n"
					 "      2) Is SDRAM init done? DDR3/4/LPDDR3 write"
					 " training uses\n"
					 "         normal write/read commands, DRAM must be up.\n"
					 "      3) single DQ dead -> that trace/solder joint.\n");
		}
	}
#endif
}

/* ============================== 驱动/ODT tuning ============================== */

/*
 * 欧姆 -> 5 位控制位（Table 22 反查）。返回 -1 = 不是标准档位。
 *
 * ⚠️ Table 22 里 36/38/41/45/49/55/60/68 各出现**两次**（bit4=1 与 bit4=0，
 *    bit4 是并联 leg，两种组合等效）。这里取**下标较小的那个**（bit4=0 段），
 *    保证同一欧姆值每次落同一码值，行为确定。
 * 下标 0（∞/开路）不参与匹配。
 */
static int drvodt_ohm2code(unsigned int ohm)
{
	unsigned int i;

	if (!ohm)
		return -1;
	for (i = 1u; i < 32u; i++) {
		if (s_drvodt_ohm[i] == ohm)
			return (int)i;
	}
	return -1;
}

/*
 * 非标准档位时取最接近的：**优先"不小于目标"**（阻值更大 = 驱动更弱 = 更保守），
 * 没有更大的再退到最接近的小值。返回控制位，-1 = 无可用档位。
 */
static int drvodt_ohm2code_near(unsigned int ohm)
{
	unsigned int i, diff = 0xffffffffu;
	int best = -1;

	if (!ohm)
		return -1;
	for (i = 1u; i < 32u; i++) {
		unsigned int v = s_drvodt_ohm[i];

		if (v >= ohm && (v - ohm) < diff) {
			diff = v - ohm;
			best = (int)i;
		}
	}
	if (best >= 0)
		return best;
	diff = 0xffffffffu;
	for (i = 1u; i < 32u; i++) {
		unsigned int v = s_drvodt_ohm[i];

		if (v < ohm && (ohm - v) < diff) {
			diff = ohm - v;
			best = (int)i;
		}
	}
	return best;
}

/*
 * 把"欧姆"写进一个 5 位阻抗寄存器。ohm == 0 -> 跳过（保持 PHY 复位默认）。
 * 非标准档位自动取最接近值，并把实际用了多少打出来（别让参数和实际悄悄不一致）。
 */
static void drvodt_write_ohm(const struct ddrp_lane_reg *r, unsigned int ohm,
			     const char *what, int idx)
{
	int code;

	if (!ohm)
		return;
	code = drvodt_ohm2code(ohm);
	if (code < 0) {
		code = drvodt_ohm2code_near(ohm);
		if (code < 0)
			return;
		ddr_platform_log("  drvodt %s%d: %d ohm is not a Table22 step"
				 " -> using %d ohm\n", what, idx, ohm,
				 s_drvodt_ohm[code]);
	}
	lane_write(r, (unsigned int)code);
}

/*
 * 写一个阻抗寄存器：**只用控制位（code）**。
 *
 * ⚠️ 按用户要求，**砍掉"欧姆换算"兜底路径**：
 *    原来是 `code != 0` 写控制位、否则 `ohm != 0` 现场查表换算；
 *    现在只认控制位 —— `code == 0` 直接**不写**（保持 PHY 复位默认值，
 *    **不是写 0**，写 0 会把驱动/ODT 关成最弱档，语义完全不同）。
 *
 * 也就是说：**阻抗完全由 ddr_param 里的控制位决定**，
 * 生成器算好什么就写什么，启动路径上不再有查表换算。
 *
 * `drvodt_write_ohm()` 的**实现保留**（用户后面调试要用），但当前无调用方；
 * 下面用 `(void)` 取一次地址，避免 -Wunused-function 报警
 * （不用 __attribute__((unused))，那个在 MIPS 交叉工具链上不一定都认）。
 *
 * 参数签名（ohm/what/idx）**保持不变**：`ddrp_apply_drvodt()` 里有 8 处调用
 * （其中 4 处在 `for (i = 0; i < 2; i++)` 里，运行时共 12 次），
 * 留着以后要装回欧姆路径时不必再改一遍 —— 现在一并 (void) 掉。
 */
static void drvodt_write(const struct ddrp_lane_reg *r, unsigned int code,
			 unsigned int ohm, const char *what, int idx)
{
	(void)ohm;
	(void)what;
	(void)idx;
	(void)drvodt_write_ohm;		/* 保留备用实现，见上 */

	if (!code)
		return;
	lane_write(r, code & 0x1fu);
}

int ddrp_apply_drvodt(void)
{
	struct ddr_drvodt_config *d;
	int i;

	if (!g_ddr_param || !g_ddr_param->drvodt.use_drvodt_config)
		return (set_err(DDR_OK), DDR_OK);
	d = &g_ddr_param->drvodt;

	/*
	 * 三类寄存器互相独立，分别处理（见 ddr_param.h 的长注释）：
	 *   DQ  per-byte（本 PHY 只有 A_l/A_h 两个 byte，数组开 [4] 仅兼容）
	 *   CMD 全局一份（除 CK）
	 *   CK  **独立**一组（0x0c8[4:0]/[12:8]，别写错成 CMD 的）
	 * ODT 只有 DQ 有。
	 * 每项都是"控制位优先、欧姆兜底"，见 drvodt_write()。
	 */
	for (i = 0; i < 2; i++) {
		drvodt_write(&s_drv_pu[i], d->drv_pu[i], d->dq_drv_pu_ohm[i],
			     "dq_drv_pu", i);
		drvodt_write(&s_drv_pd[i], d->drv_pd[i], d->dq_drv_pd_ohm[i],
			     "dq_drv_pd", i);
		drvodt_write(&s_odt_pu[i], d->odt_pu[i], d->dq_odt_pu_ohm[i],
			     "dq_odt_pu", i);
		drvodt_write(&s_odt_pd[i], d->odt_pd[i], d->dq_odt_pd_ohm[i],
			     "dq_odt_pd", i);
	}

	drvodt_write(&s_cmd_drv_pu, d->cmd_drv_pu, d->cmd_drv_pu_ohm,
		     "cmd_drv_pu", 0);
	drvodt_write(&s_cmd_drv_pd, d->cmd_drv_pd, d->cmd_drv_pd_ohm,
		     "cmd_drv_pd", 0);
	drvodt_write(&s_ck_drv_pu, d->ck_drv_pu, d->ck_drv_pu_ohm,
		     "ck_drv_pu", 0);
	drvodt_write(&s_ck_drv_pd, d->ck_drv_pd, d->ck_drv_pd_ohm,
		     "ck_drv_pd", 0);

	return (set_err(DDR_OK), DDR_OK);
}

/*
 * 打印"驱动/ODT 阻抗 + ZQ 状态"相关寄存器（training 前后对照用）。
 *
 * 为什么专门加这一个：这些阻抗控制位是**软件写进去的静态配置**（手册 4.2.3 /
 * 4.3.6），**训练不会改它们的值**；但**是否生效**取决于 ZQ 校准的 mux ——
 * 一旦 `reg_cmd_drv_zqcalib_en` / `reg_a_l/h_dq_{drv,odt}_zqcali_en` = 1，
 * 手册原话是 "the register control is useless"，模拟电路改由 `*_2reg` 驱动。
 * 所以"training 前 vs 后"要对齐看三件事：
 *   ① 寄存器里现在是什么值（本函数打印控制位 + 换算欧姆）；
 *   ② mux 现在选谁（`zq mux` 那一行）；
 *   ③ ZQ 引擎状态与结果（`zq mod` / `2reg` 那一行）。
 *
 * tag 由调用方给（例如 "before training" / "after zqcalib"），便于日志对照。
 */
void ddrp_dump_drvodt(const char *tag)
{
#if DDRP_DBG_DRVODT
	unsigned int i;

	ddr_platform_log("---- drv/odt regs [%s] ----\n", tag ? tag : "?");

	/* CMD(除 CK) 与 CK 的驱动：0x0c8，各 5 位控制位（复位 0xe = 38 ohm） */
	{
		unsigned int c[4];

		c[0] = lane_read(&s_cmd_drv_pu);
		c[1] = lane_read(&s_cmd_drv_pd);
		c[2] = lane_read(&s_ck_drv_pu);
		c[3] = lane_read(&s_ck_drv_pd);
		ddr_platform_log("  cmd: pu %d(%d ohm) pd %d(%d ohm)"
				 " | ck: pu %d(%d ohm) pd %d(%d ohm)"
				 "   [code(ohm)]\n",
				 c[0], drvodt_code2ohm(c[0]),
				 c[1], drvodt_code2ohm(c[1]),
				 c[2], drvodt_code2ohm(c[2]),
				 c[3], drvodt_code2ohm(c[3]));
	}

	/* DQ 驱动 / ODT（per byte：0 = A_l、1 = A_h） */
	for (i = 0; i < 2u; i++) {
		unsigned int a = lane_read(&s_drv_pu[i]);
		unsigned int b = lane_read(&s_drv_pd[i]);
		unsigned int c = lane_read(&s_odt_pu[i]);
		unsigned int d = lane_read(&s_odt_pd[i]);

		ddr_platform_log("  byte%d dq: drvpu %d(%d) drvpd %d(%d)"
				 " odtpu %d(%d) odtpd %d(%d)\n",
				 i, a, drvodt_code2ohm(a), b, drvodt_code2ohm(b),
				 c, drvodt_code2ohm(c), d, drvodt_code2ohm(d));
	}

	/* ZQ 的 mux：=1 表示"阻抗寄存器控制无用"，实际由 ZQ 结果驱动 */
	ddr_platform_log("  zq mux: cmd_drv=%d b0_drv=%d b1_drv=%d"
			 " b0_odt=%d b1_odt=%d   (=1 -> regs useless)\n",
			 ddrp_read_reg(DDRP_CMD_DRV_ZQCALIB_EN),
			 lane_read(&s_drv_zqcali_en[0]),
			 lane_read(&s_drv_zqcali_en[1]),
			 lane_read(&s_odt_zqcali_en[0]),
			 lane_read(&s_odt_zqcali_en[1]));

	/* ZQ 引擎状态 + 校准结果（mux=1 时就是它们在驱动模拟电路） */
	ddr_platform_log("  zq mod: en=%d pd=%d hclk=%d | 2reg(dp=%d up=%d"
			 " odtpd=%d odtpu=%d)\n",
			 ddrp_read_reg(DDRP_ZQCALI_EN),
			 ddrp_read_reg(DDRP_PD_ZQCALI),
			 ddrp_read_reg(DDRP_HCLK_ZQCALIB_SEL),
			 ddrp_read_reg(DDRP_DRVLEGPD_ZQCALI_2REG),
			 ddrp_read_reg(DDRP_DRVLEGPU_ZQCALI_2REG),
			 ddrp_read_reg(DDRP_ODTLEGPD_ZQCALI_2REG),
			 ddrp_read_reg(DDRP_ODTLEGPU_ZQCALI_2REG));
#else
	(void)tag;		/* 关掉时不留 unused 参数警告 */
#endif
}

/* ============================== 模拟参数 ============================== */
int ddrp_set_byte_vref(uint8_t byte, uint16_t vref_margsel)
{
	if (byte > 1)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	lane_write(&s_vref1_margsel[byte], vref_margsel);
	ddrp_rx_vref_latch();	/* 不造上升沿的话 PHY 仍是预设 Vref */
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_set_byte_drv_strength(uint8_t byte, uint8_t pu, uint8_t pd)
{
	if (byte > 1)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	if (pu) lane_write(&s_drv_pu[byte], pu);
	if (pd) lane_write(&s_drv_pd[byte], pd);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_set_byte_odt(uint8_t byte, uint8_t pu, uint8_t pd)
{
	if (byte > 1)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	if (pu) lane_write(&s_odt_pu[byte], pu);
	if (pd) lane_write(&s_odt_pd[byte], pd);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrp_set_byte_pin_map(uint8_t byte, const uint8_t dq_map[8],
			  uint8_t dm_map, uint8_t cat_wrap)
{
	int i;

	if (byte > 1)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	if (dq_map) {
		for (i = 0; i < 8; i++)
			lane_write(&s_dq_wrap[byte][i], dq_map[i]);
	}
	if (dm_map)
		lane_write(&s_dm_wrap[byte], dm_map);
	if (cat_wrap)
		lane_write(&s_cat_wrap[byte], cat_wrap);
	return (set_err(DDR_OK), DDR_OK);
}

/*
 * 配"读训练检查图案"：reg_{a,b}_{l,h}_rdtrain_check_wrap0/1 + 使能。
 *
 * ⚠️ 单独抽成函数，是因为它是**读训练的前置条件**，不是"引脚映射的附赠品"：
 *    以前只有 ddrp_apply_pinmap() 会写它，调用方（ddr.c）一旦忘调，
 *    这些寄存器就停在**复位 0**（reg_a_l_rdtrain_check_wrap0 reset 0x0）
 *    —— 每根 DQ 都拿 pattern0 去比，直接全表无窗口。
 *    ddrp_apply_pinmap() 与 ddrp_read_train() 都会调它（幂等，重复写无害）。
 *
 * 语义（手册 Table 45/46）：每 2 位管一根 PHY 侧 DQ，值是**图案编号**
 *   2'b00/01/10/11 -> pattern0/1/2/3
 * 要填的是"PHY 第 n 根 DQ 对应 SDRAM 的第几根线，再取 %4"：
 *   · 直连（use_pinmap = 0）：PHY DQn 就是 SDRAM DQn -> 编号 = n % 4
 *     -> wrap0 = [11][10][01][00] = 0xE4、wrap1 同理 0xE4
 *   · 有映射（use_pinmap = 1）：编号 = dq_bit_wrap[lane][n] % 4
 * 直连时**不能**读 m->dq_bit_wrap —— use_pinmap=0 时那些字段全是 0。
 * 另：reg_*_rdtrain_check_wrap 原文写 "Valid only when
 *     reg_rd_train_check_value_en is set to 1'b1" ⇒ 必须同时把 en 置 1。
 */
static void rdtrain_check_pattern_apply(void)
{
	struct ddr_pinmap_config *m;
	unsigned int nbyte = 2u, use_map;
	int i, j;

	if (!g_ddr_param)
		return;
	m = &g_ddr_param->pinmap;
	use_map = m->use_pinmap;

	/*
	 * 按位宽决定要配几个 byte：8bit -> 1（只有 byte0）、16bit -> 2。
	 * 本 PHY 数据通路**最大 16bit**，所以上限就是 2，不会再多了。
	 * （lane 表里虽然有 A_l/A_h/B_l/B_h 四项，但那是 PHY 顶层的
	 *  A/B 两组/通道，X3000 实际只用得到前两个，别按 4 去遍历。）
	 */
	if (g_ddr_param->h.bus_width)
		nbyte = g_ddr_param->h.bus_width / 8u;
	if (nbyte < 1u)
		nbyte = 1u;
	if (nbyte > 2u)
		nbyte = 2u;

	for (i = 0; i < (int)nbyte; i++) {
		unsigned int wr0 = 0u, wr1 = 0u;

		for (j = 0; j < 8; j++) {
			unsigned int src = use_map ?
				(m->dq_bit_wrap[i][j] & 0x7u) : (unsigned int)j;
			unsigned int code = src % 4u;

			if (j < 4)
				wr0 |= code << (j * 2);
			else
				wr1 |= code << ((j - 4) * 2);
		}
		lane_write(&s_rdtrain_wrap[i][0], wr0);
		lane_write(&s_rdtrain_wrap[i][1], wr1);
	}
	ddrp_write_reg(DDRP_RD_TRAIN_CHECK_VALUE_EN, 1u);
}

/* 应用协议中的 PHY 引脚映射（ddr_param.pinmap，工具按 PCB 走线生成）：
 *   CMD pad -> DQ byte -> byte 内 DQ/DM/CAT
 * 全部寄存器引用 ddrp_regs.h 宏（见上方查找表）。
 * 注：读训练检查图案**不在这里配** —— 它只在读训练时需要，
 *     由 ddrp_read_train() 自己调用 rdtrain_check_pattern_apply()。 */
int ddrp_apply_pinmap(void)
{
	struct ddr_pinmap_config *m;
	unsigned int nbyte = 2u;	/* 默认按 16bit = 2 个 byte */
	unsigned int use_map;
	int i, j;

	if (!g_ddr_param)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	m = &g_ddr_param->pinmap;
	use_map = m->use_pinmap;

	if (g_ddr_param->h.bus_width)
		nbyte = g_ddr_param->h.bus_width / 8u;
	if (nbyte < 1u)
		nbyte = 1u;
	if (nbyte > 2u)
		nbyte = 2u;

	/* ===== 引脚重映射：直连时跳过 ===== */
	if (!use_map)
		return (set_err(DDR_OK), DDR_OK);

	/* 0. CMD 重映射配套：标记 CK/CKE 实际所在 pad（手册 4.2.1） */
	ddrp_write_reg(DDRP_CKE_CK_CMD_PAD_T, m->cke_ck_cmd_pad);

	/* 1. CMD pad 映射（5-bit/个；CK/CKB/CSB/ODT/CKE 保持默认） */
	for (i = 0; i < 31; i++)
		lane_write(&s_cmd_wrap[i], m->cmd_wrap[i]);

	/* 2. DQ byte 映射（4-bit/个） */
	for (i = 0; i < 9; i++)
		lane_write(&s_byte_wrap[i], m->byte_wrap[i]);

	/* 3. byte 内 DQ/DM/CAT 位映射（只配实际用到的 byte） */
	for (i = 0; i < (int)nbyte; i++) {
		for (j = 0; j < 8; j++)
			lane_write(&s_dq_wrap[i][j], m->dq_bit_wrap[i][j]);
		lane_write(&s_dm_wrap[i], m->dm_bit_wrap[i]);
		lane_write(&s_cat_wrap[i], m->cat_wrap[i]);
	}
	return (set_err(DDR_OK), DDR_OK);
}

/* ============================== 状态 / 调试 ============================== */
int ddrp_last_error(void)
{
	return s_last_err;
}

void ddrp_dump_regs(void)
{
	/* TODO: 集成时按平台打印（如 printk("PHY 0x000=0x%x\n", ddrp_read32(0x000))） */
}

/* ==================== DQS gating 软件遍历扫描 ====================
 *
 * 用途：不依赖 PHY 自动校准，由软件把 gating 延迟逐点试出来。
 *
 * 判据：往一段 DDR 写图案再读回来逐字比对。地址走 **KSEG1(0xa0000000)**
 *       直接 volatile 访问 —— 非 cache，读到的是 DRAM 真值，
 *       不会被 cache 命中掩盖掉"其实没读对"的情况。
 *
 * **前提**：DQ 采样相位得先大致调好（见 ddrp_set_rx_dq_delay），
 *         否则任何 gating 值都读不对，扫出来整片全失败，看不出窗口。
 *         典型顺序：先粗调 DQ/DQS 延迟 -> 再扫 gating -> 回填最佳值。
 *
 * 扫描空间：cycsel（1x，0..7）× dllsel（delay line，0..31）。
 * 结果取通过区间的**中点**（离两侧边界最远，最稳）。
 */
#define DDR_SCAN_KSEG1		0xa0000000u

/* struct ddrp_dqs_scan_result 的定义在 ddrp.h（调用者要用） */

/* 写图案 -> 读回 -> 逐字比对。返回 0 = 全对，非 0 = 有错 */
static int ddr_dqs_scan_test(unsigned int pa, unsigned int nword)
{
	static const uint32_t pat[8] = {
		0x5a5a5a5au, 0xa5a5a5a5u, 0x12345678u, 0x9abcdef0u,
		0x0f0f0f0fu, 0xf0f0f0f0u, 0x00ff00ffu, 0xff00ff00u,
	};
	unsigned int i;

	for (i = 0; i < nword; i++)
		*(volatile uint32_t *)(DDR_SCAN_KSEG1 + pa + i * 4u) =
			pat[i & 7u];

	/* 给写一点落地时间，避免写缓冲还没到 DRAM 就回读 */
	ddr_platform_delay(1);

	for (i = 0; i < nword; i++) {
		if (*(volatile uint32_t *)(DDR_SCAN_KSEG1 + pa + i * 4u)
		    != pat[i & 7u])
			return -1;
	}
	return 0;
}

/*
 * 扫 gating。参数：
 *   rank  0/1 = CS0/CS1（只扫指定 rank，双 rank 由调用者各扫一次）
 *   pa    DDR 里可用的物理地址（**要在已初始化、不碰其他数据的区域**）
 *   nword 每次试写多少个 word；0 用默认 32
 *   res   输出数组，每个使能 lane 一份
 *   nres  res 的容量
 * 返回 DDR_OK = 每个 lane 都找到了窗口；DDR_ERR_TIMEOUT = 有 lane 全失败。
 */
int ddrp_rx_dqs_scan(unsigned int rank, unsigned int pa, unsigned int nword,
		     struct ddrp_dqs_scan_result *res, unsigned int nres)
{
	unsigned int lane, cyc, dll, nbyte;
	int all_ok = 1;

	if (rank > 1u || !res || nres == 0u)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	if (nword == 0u)
		nword = 32u;

	nbyte = 2u;
	if (g_ddr_param && g_ddr_param->h.bus_width)
		nbyte = g_ddr_param->h.bus_width / 8u;
	if (nbyte < 1u)
		nbyte = 1u;
	if (nbyte > 2u)
		nbyte = 2u;	/* 本 PHY 数据通路最大 16bit = 2 byte */
	if (nbyte > nres)
		nbyte = nres;

	/* 切手控模式，否则写进 bypass 寄存器的值不生效 */
	ddrp_dqs_bypass_enable(1u);

	ddr_platform_log("---- DQS gating scan rank=%d (addr 0x%x, %d words)----\n",
			 rank, pa, nword);

	for (lane = 0; lane < nbyte; lane++) {
		unsigned int npass = 0;
		unsigned int mn_c = 8u, mx_c = 0u, mn_d = 32u, mx_d = 0u;

		res[lane].npass = 0;
		res[lane].min_cyc = res[lane].max_cyc = 0;
		res[lane].min_dll = res[lane].max_dll = 0;
		res[lane].best_cyc = res[lane].best_dll = 0;

		for (cyc = 0; cyc < 8u; cyc++) {
			for (dll = 0; dll < 32u; dll++) {
				ddrp_set_dqs_bypass(lane, rank, cyc, 0u, dll);
				if (ddr_dqs_scan_test(pa, nword) != 0)
					continue;
				npass++;
				if (cyc < mn_c)
					mn_c = cyc;
				if (cyc > mx_c)
					mx_c = cyc;
				if (dll < mn_d)
					mn_d = dll;
				if (dll > mx_d)
					mx_d = dll;
			}
		}

		res[lane].npass = npass;
		if (npass != 0u) {
			res[lane].min_cyc = mn_c;
			res[lane].max_cyc = mx_c;
			res[lane].min_dll = mn_d;
			res[lane].max_dll = mx_d;
			res[lane].best_cyc = (mn_c + mx_c) / 2u;
			res[lane].best_dll = (mn_d + mx_d) / 2u;
			ddr_platform_log("  lane%d: pass=%d/256  "
					 "cyc[%d..%d] dll[%d..%d]  "
					 "-> suggest cyc=%d dll=%d\n",
					 lane, npass, mn_c, mx_c, mn_d, mx_d,
					 res[lane].best_cyc,
					 res[lane].best_dll);
		} else {
			all_ok = 0;
			/* 日志必须纯英文：u-boot printf 非 ASCII 会乱码 */
			ddr_platform_log("  lane%d: **ALL FAILED** (0/256) --"
					 " set the DQ sample phase first"
					 " (ddrp_set_rx_dq_delay), or try another"
					 " readable/writable address\n", lane);
		}
	}

	if (!all_ok)
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	return (set_err(DDR_OK), DDR_OK);
}
