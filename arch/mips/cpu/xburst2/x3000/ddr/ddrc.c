/*
 * ddrc.c - DDRC（DW UMCTL2 派生）驱动实现
 *
 * 流程：ddrc_load_param()（协议 -> 配置寄存器）-> ddrc_start_init()
 *     -> ddrc_mr_write()（颗粒 MR）-> ddrc_zq_config()
 *
 * 配置直接取自 g_ddr_param->ddrc（工具生成），无驱动侧副本。
 *
 * TODO(待确认)：
 *   - SWCTL.sw_done 触发位语义（标准 UMCTL2 同位置为 sw_init）；
 *   - ctrlupd 触发位（DFIUPD0 中未见标准 dfi_ctrlupd_req 位）；
 *   - mr_rank/mr_data 变量位域按 rank 数/实例化宽度。
 */
#include <common.h>
#include "ddrc.h"

static int s_last_err = DDR_OK;

static void set_err(int e)
{
	s_last_err = e;
}

/* 使能 DDRC 各端口。 */
void ddrc_enable_ports(void)
{
	unsigned int i;

	for (i = 0; i <= 8; i++)
		ddrc_write_bits(DDRC_PCTRL(i), DDRC_PCTRL_PORT_EN, 1u);
}

static int wait_bit(uint32_t reg, uint8_t bit, uint32_t expect, uint32_t timeout_us)
{
	uint32_t waited = 0;
	const uint32_t step = 100;
	while (waited < timeout_us) {
		if (((ddrc_readl(reg) >> bit) & 1u) == expect)
			return DDR_OK;
		udelay(1000);
		waited += step;
	}
	return DDR_ERR_TIMEOUT;
}

/* 触发 DDRC DFI 初始化并等待 DFI_INIT_COMPLETE。 */
int ddrc_dfi_init(uint32_t timeout_us)
{
	uint32_t waited = 0;
	const uint32_t step_us = 100;

	/* Set DFI_INIT_START while preserving the other DFIMISC fields. */
	ddrc_write_bits(DDRC_DFIMISC, DDRC_DFIMISC_DFI_INIT_START, 1u);

	while (waited < timeout_us) {
		if (ddrc_read_bits(DDRC_DFISTAT,
		                   DDRC_DFISTAT_DFI_INIT_COMPLETE) &&
		    ddrc_read_bits(DDRC_STAT, DDRC_STAT_OPERATING_MODE) == 1u)
			return (set_err(DDR_OK), DDR_OK);
		udelay(step_us);
		waited += step_us;
	}

	/* Set DFI_INIT_START while preserving the other DFIMISC fields. */
	ddrc_write_bits(DDRC_DFIMISC, DDRC_DFIMISC_DFI_INIT_START, 0u);
	return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
}

/* ============================ 配置载入 ============================ */
int ddrc_load_param(void)
{
	struct ddr_ddrc_config *c;
	int i;

	if (!g_ddr_param)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	c = &g_ddr_param->ddrc;

	/* 使能静态和动态寄存器编程模式，否则以下寄存器更新无效. */
	/* reset SWCTLSTATIC */
	ddrc_writel(DDRC_SWCTLSTATIC, 0x1u);
	/* reset SWCTL */
	ddrc_writel(DDRC_SWCTL, 0x0u);


	/* 写全部配置寄存器（值来自工具生成的协议） */
	ddrc_writel(DDRC_MSTR,     c->MSTR_VALUE);
	for (i = 0; i < 18; i++)
		ddrc_writel(DDRC_DRAMTMG0 + i * 0x4, c->DRAMTMG_VALUE[i]);
	ddrc_writel(DDRC_RFSHCTL0, c->RFSHCTL0_VALUE);
	ddrc_writel(DDRC_RFSHCTL1, c->RFSHCTL1_VALUE);
	ddrc_writel(DDRC_RFSHCTL2, c->RFSHCTL2_VALUE);
	ddrc_writel(DDRC_RFSHCTL3, c->RFSHCTL3_VALUE);
	ddrc_writel(DDRC_RFSHCTL4, c->RFSHCTL4_VALUE);
	ddrc_writel(DDRC_RFSHTMG,  c->RFSHTMG_VALUE);
	ddrc_writel(DDRC_RFSHTMG1, c->RFSHTMG1_VALUE);
	ddrc_writel(DDRC_DFITMG0,  c->DFITMG0_VALUE);
	ddrc_writel(DDRC_DFITMG1,  c->DFITMG1_VALUE);
	/*
	 * DM/DBI 控制（0x0180）：[0]DM_EN [1]WR_DBI_EN [2]RD_DBI_EN。
	 * 值由工具按 MR5 生成，这里整寄存器写。
	 */
	ddrc_writel(DDRC_DBICTL,   c->DBICTL_VALUE);
	/* DFI 低功耗控制（0x0198/0x019c）：默认全 0 = 不使能 */
	ddrc_writel(DDRC_DFILPCFG0, c->DFILPCFG0_VALUE);
	ddrc_writel(DDRC_DFILPCFG1, c->DFILPCFG1_VALUE);
	ddrc_writel(DDRC_DFIUPD0,  c->DFIUPD0_VALUE);
	ddrc_writel(DDRC_DFIUPD1,  c->DFIUPD1_VALUE);
	ddrc_writel(DDRC_ODTCFG,   c->ODTCFG_VALUE);
	ddrc_writel(DDRC_ODTMAP,   c->ODTMAP_VALUE);
	ddrc_writel(DDRC_PWRCTL,   c->PWRCTL_VALUE);
	ddrc_writel(DDRC_PWRTMG,   c->PWRTMG_VALUE);
	ddrc_writel(DDRC_ZQCTL0,   c->ZQCTL0_VALUE);
	ddrc_writel(DDRC_ZQCTL1,   c->ZQCTL1_VALUE);
	for (i = 0; i < 12; i++)
		ddrc_writel(DDRC_ADDRMAP0 + i * 0x4, c->ADDRMAP_VALUE[i]);

	/* SDRAM 自动初始化用 MR 值（DDR4 必需）：uMCTL2 在初始化序列写入 INIT3/4/6/7
	 * 配置的 MR0-6，驱动不用 MRCTRL 逐个写。DDR3 无 DDR4 的 MR4-6，仅写 INIT3/4。 */
	/*INIT 1 DRAM RSTN_CYCLE*/
	ddrc_writel(DDRC_INIT1, 0);

	/*INIT0 config*/
	ddrc_write_bits(DDRC_INIT0, DDRC_INIT0_POST_CKE_X1024, 0x3u);
	ddrc_write_bits(DDRC_INIT0, DDRC_INIT0_PRE_CKE_X1024, 0x3u);

	if (g_ddr_param->h.type == DDR4) {
		ddrc_writel(DDRC_INIT3, c->INIT3_VALUE);	/* MR0+MR1 */
		ddrc_writel(DDRC_INIT4, c->INIT4_VALUE);	/* MR2+MR3 */
		ddrc_writel(DDRC_INIT5, c->INIT5_VALUE);	/* ZQ 校准时间 */
		ddrc_writel(DDRC_INIT6, c->INIT6_VALUE);	/* MR4+MR5 */
		ddrc_writel(DDRC_INIT7, c->INIT7_VALUE);	/* MR6 */
	} else if (g_ddr_param->h.type == DDR3) {
		ddrc_writel(DDRC_INIT3, c->INIT3_VALUE);	/* MR0+MR1 */
		ddrc_writel(DDRC_INIT4, c->INIT4_VALUE);	/* MR2+MR3 */
	}

	return (set_err(DDR_OK), DDR_OK);
}

/* ============================ 初始化触发 ============================ */
int ddrc_start_init(uint32_t timeout_us)
{
	/* TODO(待确认): SWCTL[0] 本文档为 sw_done(R/W)；标准 UMCTL2 同位置为
	 * sw_init（写 1 触发初始化）。实现按"写 1 触发、SWSTAT 完成"处理。 */
	ddrc_write_bits(DDRC_SWCTL, 0, 0, 1u);
	if (wait_bit(DDRC_SWSTAT, 0, 1u, timeout_us) != DDR_OK)
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	return (set_err(DDR_OK), DDR_OK);
}

/* ============================ MR 访问 ============================ */
int ddrc_mr_write(uint8_t rank, uint8_t mr, uint16_t data, uint32_t timeout_us)
{
	ddrc_write_bits(DDRC_MRCTRL0, DDRC_MRCTRL0_MR_ADDR, mr);
	/* TODO(待确认): mr_rank 为变量位域，单 rank 按 [0:0]，多 rank 需确认 */
	ddrc_write_bits(DDRC_MRCTRL0, 0, 0, rank);
	/* TODO: mr_data 变量位域，按实例化宽度调整（当前按 16bit） */
	ddrc_write_bits(DDRC_MRCTRL1, 15, 0, data);
	ddrc_write_bits(DDRC_MRCTRL0, DDRC_MRCTRL0_MR_WR, 1u);
	if (wait_bit(DDRC_MRSTAT, 0, 0u, timeout_us) != DDR_OK)
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	ddrc_write_bits(DDRC_MRCTRL0, DDRC_MRCTRL0_MR_WR, 0u);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrc_mr_read(uint8_t rank, uint8_t mr, uint16_t *data, uint32_t timeout_us)
{
	if (!data)
		return (set_err(DDR_ERR_PARAM), DDR_ERR_PARAM);
	/* TODO(待确认): MR 读完成判定（mr_rd_busy?）待确认 */
	ddrc_write_bits(DDRC_MRCTRL0, DDRC_MRCTRL0_MR_ADDR, mr);
	ddrc_write_bits(DDRC_MRCTRL0, 0, 0, rank);
	ddrc_write_bits(DDRC_MRCTRL0, DDRC_MRCTRL0_MR_WR, 0u);
	if (wait_bit(DDRC_MRSTAT, 0, 0u, timeout_us) != DDR_OK)
		return (set_err(DDR_ERR_TIMEOUT), DDR_ERR_TIMEOUT);
	*data = (uint16_t)ddrc_read_bits(DDRC_MRCTRL1, 15, 0);  /* TODO: mr_data 变量位域 */
	return (set_err(DDR_OK), DDR_OK);
}

/* ============================ ZQ / ctrlupd ============================ */
int ddrc_zq_config(void)
{
	struct ddr_ddrc_config *c = &g_ddr_param->ddrc;

	if (c->ZQCTL0_VALUE) ddrc_writel(DDRC_ZQCTL0, c->ZQCTL0_VALUE);
	if (c->ZQCTL1_VALUE) ddrc_writel(DDRC_ZQCTL1, c->ZQCTL1_VALUE);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrc_ctrlupd_request(uint32_t timeout_us)
{
	/* TODO(待确认): ctrlupd 触发位待确认（DFIUPD0 中未见 dfi_ctrlupd_req）；
	 * 标准流程中 ctrlupd 由 MC 发起。 */
	(void)timeout_us;
	return (set_err(DDR_OK), DDR_OK);
}

/* ============================ 状态 / 调试 ============================ */
int ddrc_get_status(uint32_t *sw_done_ack, uint32_t *mr_busy)
{
	if (sw_done_ack)
		*sw_done_ack = ddrc_read_bits(DDRC_SWSTAT, 0, 0);
	if (mr_busy)
		*mr_busy = ddrc_read_bits(DDRC_MRSTAT, 0, 0);
	return (set_err(DDR_OK), DDR_OK);
}

int ddrc_last_error(void)
{
	return s_last_err;
}

void ddrc_dump_regs(void)
{
	/* TODO: 集成时按平台打印（如 printk("MSTR=0x%x\n", ddrc_readl(DDRC_MSTR))） */
}
