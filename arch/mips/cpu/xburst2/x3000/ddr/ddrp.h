/*
 * ddrp.h - DDRP（Innosilicon DDR4 3.3L PHY）驱动接口
 *
 * 职责：PHY 复位/配置 / ZQ 校准 / Write Leveling / Rx-DQS 校准 /
 *      模拟参数（Vref/驱动强度/ODT/pin map）。
 * 配置直接取自 ddr_param.h 的 struct ddr_param::ddrp（工具生成），
 * 由 ddrp_load_param() 写入寄存器，驱动内部不再复制配置结构体。
 */
#ifndef __DDRP_H__
#define __DDRP_H__

#include "ddr.h"

/* 从 g_ddr_param->ddrp 载入配置并写 PHY 配置寄存器（类型/通道/频率点/Vref） */
int ddrp_load_param(void);

/* 配置 PHY 训练所需的通道、寄存器更新和 DQ/DM 参数。 */
int ddrp_training_config(void);

/* 开启 PHY 训练相关寄存器更新。 */
void ddrp_enable_training_reg_update(void);

/* PHY 软复位（低有效；assert=1 复位，0 释放） */
int ddrp_reset(void);

/* ZQ 校准（超时 us，轮询 zqcali_done） */
int ddrp_zqcalib(uint32_t timeout_us);

/* Write Leveling（超时 us，轮询 wl_done_byte） */
int ddrp_write_leveling(uint32_t timeout_us);

/* Rx-DQS 校准（超时 us，轮询 calib_done_byte） */
int ddrp_rx_dqs_calib(uint32_t timeout_us);

/* ==================== Rx-DQS Gating 手动（bypass） ====================
 * 依据手册 **6.2.3 Bypass Rx DQS Gating Training** + Table 45。
 * 初期用软件遍历 gating 窗口时：把控制权从 PHY 自动校准切到软件，
 * 由软件直接给 gating 延迟，配合 DFI 读写颗粒逐点扫，找最佳窗口。
 * 注意：只调 gating 还不够 —— DQ 采样相位是另一套寄存器
 * （reg_*_cs*_dq*_invdelayselrx，见手册 Table 26/27），
 * 两个维度都要遍历，否则读回数据无法判断。
 */

/* 开关 bypass 模式：1 = 用软件手填值（reg_calib_bypass=1），0 = 用自动校准结果 */
int ddrp_dqs_bypass_enable(unsigned int enable);

/*
 * 设置一个 lane/rank 的 gating 延迟并立即生效 —— **遍历扫描反复调它**。
 *   lane: 0..3 = A_l/A_h/B_l/B_h     rank: 0..1 = RANK0/RANK1
 *   cyc_dly: 1x 延迟(3 位)
 *   oph_dly: 0.5UI 延迟(3 位)
 *   dll_dly: 4UI/256 延迟(5 位，即 delay line)
 * 内部走手册 6.2.3 的 Step1~6（开时钟门 -> 写三个值 -> 选频点
 * -> calib_freq_update 0/1/0 触发更新 -> 收时钟门）。
 */
int ddrp_set_dqs_bypass(unsigned int lane, unsigned int rank,
			unsigned int cyc_dly, unsigned int oph_dly,
			unsigned int dll_dly);

/* 按 g_ddr_param->dqsbp 一次设好所有使能 lane/rank 并生效 */
int ddrp_apply_dqs_bypass(void);

/* ==================== DQS gating 软件遍历扫描 ====================
 * 把 gating 延迟逐点试出来，判据是"往 DDR 写图案再读回来比对"，
 * 地址走 KSEG1（非 cache）确保读的是 DRAM 真值。
 * 扫 cycsel(0..7) × dllsel(0..31)，取通过窗口中点作为建议值。
 * 前提：DQ 采样相位先大致调好，否则整片读错看不出窗口。
 */

/* 一个 (lane,rank) 的扫描结果 */
struct ddrp_dqs_scan_result {
	unsigned int npass;		/* 通过的点数（满分 8*32=256） */
	unsigned int min_cyc, max_cyc;	/* 通过窗口在 cycsel 上的范围 */
	unsigned int min_dll, max_dll;	/* 通过窗口在 dllsel 上的范围 */
	unsigned int best_cyc, best_dll;/* 窗口中点 = 建议采用值 */
};

/*
 * rank  0/1 = CS0/CS1（双 rank 由调用者各扫一次）
 * pa    DDR 里可用的**物理地址**（要在已初始化、且不会踩到其他数据的区域）
 * nword 每次试写多少个 word；0 = 默认 32
 * res   输出数组，每个使能 lane 一份；nres 是它的容量
 * 返回 DDR_OK = 每个 lane 都找到窗口；DDR_ERR_TIMEOUT = 有 lane 全失败。
 */
int ddrp_rx_dqs_scan(unsigned int rank, unsigned int pa, unsigned int nword,
		     struct ddrp_dqs_scan_result *res, unsigned int nres);

/* ==================== Rx 采样相位（DQ per-bit）====================
 * lane: 0..1 = 数据 byte（0 = 低 byte = A_l，1 = 高 byte = A_h）
 * rank: 0..1 = RANK0/RANK1（CS0/CS1），对应寄存器后缀 _cs0_ / _cs1_
 * 注意 ddrp_set_rx_dqs_delay() 会**同时写 DQS 与 DQSB**（反相端必须同值）。
 */
int ddrp_set_rx_dq_delay(unsigned int lane, unsigned int rank,
			 unsigned int dq, unsigned int dly);
int ddrp_set_rx_dqs_delay(unsigned int lane, unsigned int rank,
			  unsigned int dly);
/* 改完一批值后调一次把值刷进 PHY；rank = 0/1 = CS0/CS1 */
void ddrp_rx_dq_update(unsigned int rank);

/*
 * 一次设好某个 rank 下两个 byte 的 RX DQ/DQS/DQSB 延迟并刷值（初始化用）。
 * CS0/CS1 由 rank 区分，不再单开 *_cs0 / *_cs1 两个函数。
 *   l_dq / h_dq    低 byte(A_l) / 高 byte(A_h) 的 DQ 延迟（7 位）
 *   l_dqs / h_dqs  低 / 高 byte 的 DQS 延迟（DQSB 自动取同值）
 */
void ddrp_set_rx_delay_rank(unsigned int rank,
			    unsigned int l_dq, unsigned int h_dq,
			    unsigned int l_dqs, unsigned int h_dqs);

/* ==================== Read Training（Rx DQ/DQS 采样相位训练）====================
 * 依据手册 **6.3 Read Training** + Table 46（寄存器）/ Table 47（结果）
 * + 6.3.2 Figure 24（Auto 流程）。
 *
 * 目的：为每个 DQ 找出 Rx delay line 的**通过窗口**，并把 Rx DQS 精确对齐到
 *       DQ 数据眼的中心 —— 即训练出 **DQ 与 DQS 的延迟**。
 *       per-bit de-skew 调整范围 2-UI（手册 4.3.2）。
 *
 * 两种模式（手册 6.3）：
 *   Auto（本模块）：硬件自己用 MPR/MPC 命令扫，结果落在 Table 47 的结果
 *                   寄存器里，**并自动应用**到 Rx 采样相位，软件不必再写。
 *   Bypass        ：结果由软件给的 bypass 寄存器决定，即
 *                   ddrp_set_rx_dq_delay() / ddrp_set_rx_dqs_delay()
 *                   + ddrp_rx_dq_update()（手册 4.3.2 Table 27）。
 *
 * 与 ddrp_rx_dqs_scan() 的分工：那个是**软件**逐点扫 gating（cyc/dll）窗口；
 * 这个让 **PHY 硬件**训练 DQ/DQS 采样相位。两者配合：先本函数把采样相位
 * 训好，再用 ddrp_rx_dqs_scan() 扫 gating 窗口。
 */

/* 一个 byte 的 read training 结果（手册 Table 47） */
struct ddrp_rd_train_result {
	unsigned int min_dq[8];	/* [dq] Rx delay line 通过窗口下限（7 位） */
	unsigned int max_dq[8];	/* [dq] 通过窗口上限 */
	unsigned int min_dqs;	/* DQS/DQSB 的窗口下限 */
	unsigned int max_dqs;	/* DQS/DQSB 的窗口上限 */
	unsigned int dqs;	/* DQS/DQSB 训练后的最佳点（reg_*_train_result_for_rd_base_dqs） */
	unsigned int no_window;	/* 1 = 该 byte 找不到通过窗口
				 *     （reg_*_change_rd_dqs_default） */
};

/*
 * 跑一轮 auto read training（MPR/MPC 模式，手册 6.3.2 Figure 24）。
 *
 *   rank       0 = CS0 / 1 = CS1（写 reg_rdtrain_cs_sel，双 rank 需各跑一次）
 *   timeout_us 等待 train_true_done 的超时
 *
 * 前置：PHY 已复位、PLL 已锁、DDRC 已完成初始化（能正常读写），并且
 *       ddrp_apply_pinmap() 已配好 —— RDTRAIN 检查图案依赖 DQ 重映射关系。
 * 内部会开 PHY auto-refresh（training 期间需要）、训练完再关掉。
 *
 * 返回 DDR_OK；DDR_ERR_TIMEOUT（没等到 done）；
 *      DDR_ERR_FAIL（有 byte 报 reg_train_error_for_rd_byte）。
 */
int ddrp_read_train(unsigned int rank, uint32_t timeout_us);

/*
 * 读回上一次训练结果。byte 索引 0/1 = A_l/A_h（X3000 只有 2 个 byte）。
 * res 至少要有 2 项，nres 是容量。返回实际填了几个。
 */
unsigned int ddrp_read_train_get(struct ddrp_rd_train_result *res,
				 unsigned int nres);

/* 打印结果：每个 DQ 的 [min..max] 窗口 + 宽度 + DQS 点。
 * 窗口太窄（< 1/4）说明采样裕量不足，别只看"有没有窗口"。 */
void ddrp_dump_read_train_result(void);

/* 应用协议中的驱动强度/ODT tuning（ddr_param.drvodt） */
int ddrp_apply_drvodt(void);

/* 应用协议中的 PHY 引脚映射（ddr_param.pinmap，工具按 PCB 走线生成）：
 * 须在校准/训练前调用（CMD/DQ 重映射 + 读训练检查图案同步） */
int ddrp_apply_pinmap(void);

/* 模拟参数（byte 0..3 = A_l/A_h/B_l/B_h） */
int ddrp_set_byte_vref(uint8_t byte, uint16_t vref_margsel);
int ddrp_set_byte_drv_strength(uint8_t byte, uint8_t pu, uint8_t pd);
int ddrp_set_byte_odt(uint8_t byte, uint8_t pu, uint8_t pd);
int ddrp_set_byte_pin_map(uint8_t byte, const uint8_t dq_map[8],
			  uint8_t dm_map, uint8_t cat_wrap);

/* 状态 / 错误 / 调试 */
int  ddrp_last_error(void);
void ddrp_dump_regs(void);

#endif /* __DDRP_H__ */
