/*
 * ddr.c - DDR 总初始化入口
 *
 * 编排 DDRC（ddrc.c）+ DDRP（ddrp.c）+ 颗粒（MR 经 DDRC 写入），
 * 参数全部来自 ddr_param.h（工具生成）。
 *
 * 流程（参考 u-boot ddr_innophy_xx.c 的 sdram_init 编排）：
 *   平台初始化 -> PHY 复位/配置 -> DDRC 配置 -> PHY 校准(ZQ/WL/Rx-DQS)
 *   -> 颗粒 MR -> DDRC 触发初始化 -> 驱动/ODT tuning
 *
 * TODO(待确认)：
 *   1) PHY 校准与 DDRC 触发初始化的先后顺序；
 *   2) DFI 侧 read/write/CA training 的调度（需 MC 配合）；
 *   3) 平台接口 ddr_platform_* 的 X3000 实现。
 */
#include <common.h>
#include <asm/io.h>
#include <ddr/ddr_params.h>

#include <asm/arch/cpm.h>
#include <asm/arch/clk.h>
#include "ddr.h"
#include "ddrc.h"
#include "ddrp.h"
#include "ddr_reg_values.h"

#define CCU_DDRC_CTRL		(CCU_BASE + 0x1200)
#define CCU_DDRC_CTRL_ACCESSIBLE_DDR_READ	(1u << 0)


struct ddr_param *g_ddr_param = &ddr_reg_values;

/* Assert the PHY APB register reset, presetn (active low). */
static void presetn_reset(void)
{
	unsigned int tmp;

	tmp = cpm_readl(CPM_SR);
	tmp |= CPM_DDRP_RESET;
	cpm_writel(tmp, CPM_SR_SET);
}

/* Release the PHY APB register reset, presetn. */
static void presetn_release_reset(void)
{
	unsigned int tmp = cpm_readl(CPM_SR);

	tmp |= CPM_DDRP_RELEASE_RESET;
	cpm_writel(tmp, CPM_SR_CLR);
}

/* Assert the PHY digital-core reset, system_rstn (active low). */
static void system_rstn_reset(void)
{
	unsigned int tmp = cpm_readl(CPM_SR);

	tmp |= CPM_DDRC_RESET;
	cpm_writel(tmp, CPM_SR_SET);
}

/* Release the PHY digital-core reset, system_rstn. */
static void system_rstn_release_reset(void)
{
	unsigned int tmp = cpm_readl(CPM_SR);

	tmp |= CPM_DDRC_RELEASE_RESET;
	cpm_writel(tmp, CPM_SR_CLR);
}

/* 允许 CCU 模块访问 DDR 读通道。 */
static void ddr_enable_read_access(void)
{
	unsigned int ccu_ctrl = readl(CCU_DDRC_CTRL);

	ccu_ctrl |= CCU_DDRC_CTRL_ACCESSIBLE_DDR_READ;
	writel(ccu_ctrl, CCU_DDRC_CTRL);
}

static void ddr_init_rx_dq_delay(void)
{
	if (g_ddr_param && g_ddr_param->h.type == DDR4)
		ddrp_set_rx_delay_rank(0u, 20u, 20u, 127u, 127u);
	else
		ddrp_set_rx_delay_rank(0u, 14u, 14u, 8u, 8u);
}

/* 配置 DDRC 和 PHY 的训练前参数。 */
#if 0
/* 颗粒 MR 写入序列（值来自协议 dram 段） */
static void ddr_mr_write_sequence(uint32_t timeout_us)
{
	struct ddr_param *p = g_ddr_param;

	ddrc_mr_write(0, 0,  (uint16_t)p->dram.MR0_VALUE,  timeout_us);
	ddrc_mr_write(0, 1,  (uint16_t)p->dram.MR1_VALUE,  timeout_us);
	ddrc_mr_write(0, 2,  (uint16_t)p->dram.MR2_VALUE,  timeout_us);
	ddrc_mr_write(0, 3,  (uint16_t)p->dram.MR3_VALUE,  timeout_us);
	ddrc_mr_write(0, 10, (uint16_t)p->dram.MR10_VALUE, timeout_us);
	ddrc_mr_write(0, 11, (uint16_t)p->dram.MR11_VALUE, timeout_us);
	ddrc_mr_write(0, 63, (uint16_t)p->dram.MR63_VALUE, timeout_us);
	/* TODO(待确认): MR 间时序间隔（tMRD/tMOD）由 MC 侧流程保证 */
}
#endif


/* 各寄存器块基址（物理地址）与 dump 长度（字节） */
#define DDR_DUMP_CPM_SIZE       0x264u          /* CPCCR..LEP_SFT_INT_EN */
#define DDR_DUMP_DDRP_SIZE      0x800u
#define DDR_DUMP_DDRC_SIZE      0x800u

static void dump_region(const char *name, unsigned int phys, unsigned int size)
{
        unsigned int off;

        printf("#REGION %s 0x%x 0x%x\n", name, phys, size);
        for (off = 0; off < size; off += 4u)
                printf("%x %x\n", phys + off,
                       *(volatile unsigned int *)(phys + off));
}

void ddr_dump_region(int which)
{
        if (which <= 0 || which > 2)
                dump_region("CPM", CPM_BASE, DDR_DUMP_CPM_SIZE);
        if (which == 1 || which < 0 || which > 2)
                dump_region("DDRP", DDR_PHY_BASE, DDR_DUMP_DDRP_SIZE);
        if (which == 2 || which < 0 || which > 2)
                dump_region("DDRC", DDRC_BASE, DDR_DUMP_DDRC_SIZE);
}

void ddr_dump_all(void)
{
        printf("# X3000 DDR DUMP v1 (parseable by ddr_creater -B)\n");
        ddr_dump_region(-1);
        printf("# END\n");
}

/* ============================ 总入口 ============================ */
void sdram_init(void)
{
	unsigned int timeout = 100000u;

	/* CCU module 允许访问 DDR 的读通道*/
	ddr_enable_read_access();

	clk_set_rate(DDR, g_ddr_param->h.freq / 2); // set dfi1x_clk freq 1:2

        dump_region("CPM", CPM_BASE, DDR_DUMP_CPM_SIZE);

	/* Step 1: 提供稳定的 pclk 和 dfi_clk1x_in，完成 SoC 级前置。 */
	/* Step 2: 同时拉低 PHY 的 presetn 和 system_rstn，并保持至少 100ns。 */
	presetn_reset();
	system_rstn_reset();
	udelay(1);

	/* Step 3: 等待至少 5 个 pclk 后释放 presetn。 */
	presetn_release_reset();
	udelay(1);

	/* Step 4: PHY 内部 soft reset。 */
	ddrp_reset();
        
	/* Step 4（续）：配置 DDRC/PHY 静态寄存器。 */
	ddrc_load_param();
	ddrp_load_param();

	/* Step 4（续）：配置 PHY 引脚映射（板级 PCB 走线；须在 ZQ/训练前生效，否则
	 *    读训练检查图案与 DQ 实际顺序不符） */
	//ddrp_apply_pinmap();

	/* 禁用页面关闭策略，清除 SCHED.pageclose。 */
	ddrc_write_bits(DDRC_SCHED, DDRC_SCHED_PAGECLOSE, 0u);

	/* Step 4（续）：配置 PHY 训练前的通道和训练参数。 */
	ddrp_training_config();


	/* Step 5: 释放 PHY 数字核心的 system_rstn。 */
	system_rstn_release_reset();

	/* Step 6: 拉高 dfi_init_start，等待 dfi_init_complete。 */
	ddrc_dfi_init(timeout);
	/* Step 7: 拉低 dfi_init_start，结束 PHY 初始化（当前待补充）。 */

	ddrp_enable_training_reg_update();

	/* Step 9：执行 PHY 训练（当前代码位置早于 SDRAM 初始化）。 */
	//ddrp_write_leveling(timeout);
	ddrp_rx_dqs_calib(timeout);
	ddr_init_rx_dq_delay();

	ddrc_enable_ports();


	/* Step 8：初始化 SDRAM（当前代码位置晚于 PHY 训练）。 */
	/* 颗粒 MR 写入经 DDRC MRCTRL。 */
	/*ddr_mr_write_sequence(100000u);*/
	//ddrc_start_init(timeout);

	/* Step 9（续）：完成 PHY ZQ 校准。 */
	//ddrp_zqcalib(timeout);
	/* Step 10：进入正常读写前完成 PHY 驱动/ODT tuning。 */
	//ddrp_apply_drvodt();

}

phys_size_t initdram(int board_type)
{
        return 0x10000000;
}
