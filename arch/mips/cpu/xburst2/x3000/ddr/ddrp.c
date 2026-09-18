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
	ddrp_write32(DDRP_REG_MEMCFG, c->MEMCFG_VALUE | 7); // 此处必须释放reset。
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
int ddrp_zqcalib(uint32_t timeout_us)
{
	int byte;

	/* 使能各 byte：校准结果写入模拟电路（reg_*_dq_{odt,drv}_zqcali_en） */
	for (byte = 0; byte < 4; byte++) {
		lane_write(&s_odt_zqcali_en[byte], 1u);
		lane_write(&s_drv_zqcali_en[byte], 1u);
	}
	ddrp_write_reg(DDRP_ZQCALI_EN, 1u);
	udelay(1000);
	if (wait_field(DDRP_ZQCALI_DONE, 1u, timeout_us) != DDR_OK)
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	ddrp_write_reg(DDRP_ZQCALI_EN, 0u);
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
	unsigned int i;

	printf("---- Write Leveling result (rank%d, 0x280 family, "
	       "high 8b=RANK0 / low 8b=RANK1)----\n", rank);
	for (i = 0; i < 2u; i++) {
		const struct ddrp_train_result *t = &s_train_result[i];

		printf("  %s: rank0=0x%x rank1=0x%x\n", t->name,
				 lane_read(&t->wl_rank0),
				 lane_read(&t->wl_rank1));
	}
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
	 * **每个 CS/RANK 各跑一轮**：手册 reg_calcs_sel 的 Note 说使能
	 * Rx-DQS 校准期间不能设 2'b00，必须显式选 10=RANK0 / 01=RANK1。
	 */
	for (rank = 0; rank < rank_num; rank++) {

		ddrp_write_reg(DDRP_START_CALIB, 0u);	/* 先清状态机 */
		ddrp_write_reg(DDRP_CALCS_SEL, rank ? 1u : 2u);
		/* DDR4 requires automatic Rx-DQS calibration mode. */
		if (g_ddr_param && g_ddr_param->h.type == DDR4)
			ddrp_write_reg(DDRP_CALIB_MODE_SEL, 1u);
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

/* 上一次训练的 error 位（reg_train_error_for_rd_byte，0x1c4[26:18]） */
static uint32_t s_rd_train_err;

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
	 * 3) 启动。DQS_RD_TRAIN_EN 保持 0 = 只做 DQ per-bit skew 训练；
	 *    手册注明"同时做 DQS-DQ Eye 训练"是 Not suggested，所以不开。
	 */
	ddrp_write_reg(DDRP_DQS_RD_TRAIN_EN, 0u);
	ddrp_write_reg(DDRP_DQ_RD_TRAIN_EN, 1u);

	/* 5) 等 train_true_done */
	ret = wait_field(DDRP_TRAIN_TRUE_DONE, 1u, timeout_us);

	/* 7) 无论成败都退出，别把状态机留在训练模式 */
	ddrp_write_reg(DDRP_DQ_RD_TRAIN_EN, 0u);

	/* 6) 判成败：error 是每 byte 一位 */
	err = ddrp_read_reg(DDRP_TRAIN_ERROR_FOR_RD_BYTE) & 0x1ffu;
	s_rd_train_err = err;

	if (ret != DDR_OK) {
		ddr_platform_log("read train rank%u: 超时（train_true_done 未置位）"
				 " err=0x%03x\n", rank, err);
		ddrp_dump_read_train_result();
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	}
	if (err & want) {
		ddr_platform_log("read train rank%u: 有 byte 报错 err=0x%03x"
				 "（期望 0x%03x）\n", rank, err, want);
		ddrp_dump_read_train_result();
		return (set_err(DDR_ERR_FAIL), DDR_ERR_FAIL);
	}

	ddr_platform_log("read train rank%u: done, err=0x%03x\n", rank, err);
	ddrp_dump_read_train_result();
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
	}
	return n;
}

void ddrp_dump_read_train_result(void)
{
	struct ddrp_rd_train_result r[2];
	unsigned int n = ddrp_read_train_get(r, 2u);
	unsigned int b, d;

	ddr_platform_log("---- Read training 结果（Table 47；Rx delay line）----\n");
	if (!n) {
		ddr_platform_log("  （没有可用的 byte —— 检查 bus_width/CHANNEL_EN）\n");
		return;
	}
	for (b = 0; b < n; b++) {
		ddr_platform_log("  byte%u (%s): dqs [%u..%u] best=%u%s\n", b,
				 b ? "A_h/high" : "A_l/low",
				 r[b].min_dqs, r[b].max_dqs, r[b].dqs,
				 r[b].no_window ? "  **找不到通过窗口**" : "");
		/*
		 * 窗口宽度 = max - min。两个都要看：
		 *   完全没有（no_window 置位 / min==max）-> 该 DQ 没建立窗口；
		 *   非常窄 -> 有窗口但裕量不足，换图案/换数据就可能错。
		 */
		for (d = 0; d < 8u; d++) {
			unsigned int mn = r[b].min_dq[d];
			unsigned int mx = r[b].max_dq[d];
			unsigned int w = (mx > mn) ? (mx - mn) : 0u;

			ddr_platform_log("    DQ%u: [%u..%u] 宽=%u%s%s\n", d, mn, mx, w,
					 w == 0u ? "  **无窗口**" : "",
					 (w > 0u && w < 16u) ? "  (偏窄)" : "");
		}
	}
}

/* ============================== 驱动/ODT tuning ============================== */
int ddrp_apply_drvodt(void)
{
	struct ddr_drvodt_config *d;
	int i;

	if (!g_ddr_param || !g_ddr_param->drvodt.use_drvodt_config)
		return (set_err(DDR_OK), DDR_OK);
	d = &g_ddr_param->drvodt;
	for (i = 0; i < 2; i++) {
		if (d->drv_pu[i])
			lane_write(&s_drv_pu[i], d->drv_pu[i]);
		if (d->drv_pd[i])
			lane_write(&s_drv_pd[i], d->drv_pd[i]);
		if (d->odt_pu[i])
			lane_write(&s_odt_pu[i], d->odt_pu[i]);
		if (d->odt_pd[i])
			lane_write(&s_odt_pd[i], d->odt_pd[i]);
	}
	return (set_err(DDR_OK), DDR_OK);
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

/* 应用协议中的 PHY 引脚映射（ddr_param.pinmap，工具按 PCB 走线生成）：
 *   CMD pad -> DQ byte -> byte 内 DQ/DM/CAT -> 读训练检查图案（配套）
 * 全部寄存器引用 ddrp_regs.h 宏（见上方查找表）。 */
int ddrp_apply_pinmap(void)
{
	struct ddr_pinmap_config *m;
	int i, j;

	if (!g_ddr_param || !g_ddr_param->pinmap.use_pinmap)
		return (set_err(DDR_OK), DDR_OK);
	m = &g_ddr_param->pinmap;

	/* 0. CMD 重映射配套：标记 CK/CKE 实际所在 pad（手册 4.2.1） */
	ddrp_write_reg(DDRP_CKE_CK_CMD_PAD_T, m->cke_ck_cmd_pad);

	/* 1. CMD pad 映射（5-bit/个；CK/CKB/CSB/ODT/CKE 保持默认） */
	for (i = 0; i < 31; i++)
		lane_write(&s_cmd_wrap[i], m->cmd_wrap[i]);

	/* 2. DQ byte 映射（4-bit/个） */
	for (i = 0; i < 9; i++)
		lane_write(&s_byte_wrap[i], m->byte_wrap[i]);

	/* 3. byte 内 DQ/DM/CAT 位映射 */
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 8; j++)
			lane_write(&s_dq_wrap[i][j], m->dq_bit_wrap[i][j]);
		lane_write(&s_dm_wrap[i], m->dm_bit_wrap[i]);
		lane_write(&s_cat_wrap[i], m->cat_wrap[i]);
	}

	/* 4. 读训练检查图案（DQ 顺序改变后必须同步，手册 4.3.1 注；
	 *    且须置位 reg_rd_train_check_value_en 才生效） */
	for (i = 0; i < 2; i++) {
		lane_write(&s_rdtrain_wrap[i][0], m->rdtrain_check_wrap[i][0]);
		lane_write(&s_rdtrain_wrap[i][1], m->rdtrain_check_wrap[i][1]);
	}
	ddrp_write_reg(DDRP_RD_TRAIN_CHECK_VALUE_EN, m->rdtrain_check_value_en ? 1u : 0u);
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
			ddr_platform_log("  lane%d: **ALL FAILED** (0/256) -- "
					 "先确认 DQ 采样相位"
					 "（ddrp_set_rx_dq_delay），"
					 "或换一段可读写地址\n", lane);
		}
	}

	if (!all_ok)
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	return (set_err(DDR_OK), DDR_OK);
}
