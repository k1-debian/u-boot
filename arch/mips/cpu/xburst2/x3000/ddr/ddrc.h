/*
 * ddrc.h - DDRC（DDR Controller，DW UMCTL2 派生）驱动接口
 *
 * 职责：DDRC 配置写入 / 初始化触发 / MR 访问 / 状态查询。
 * 配置直接取自 ddr_param.h 的 struct ddr_param::ddrc（工具生成），
 * 由 ddrc_load_param() 写入寄存器，驱动内部不再复制配置结构体。
 */
#ifndef __DDRC_H__
#define __DDRC_H__

#include "ddr.h"

/* 从 g_ddr_param->ddrc 载入配置并写全部 DDRC 配置寄存器 */
int ddrc_load_param(void);

/* 使能 DDRC 各端口。 */
void ddrc_enable_ports(void);

/* 触发 DDRC DFI 初始化并等待初始化完成（超时 us）。 */
int ddrc_dfi_init(uint32_t timeout_us);

/* 触发 DDRC 初始化并轮询 SWSTAT.sw_done_ack（超时 us） */
int ddrc_start_init(uint32_t timeout_us);

/* MR 写 / 读（经 MRCTRL0/1 + MRSTAT 轮询） */
int ddrc_mr_write(uint8_t rank, uint8_t mr, uint16_t data, uint32_t timeout_us);
int ddrc_mr_read(uint8_t rank, uint8_t mr, uint16_t *data, uint32_t timeout_us);

/* ZQ 校准时序配置（触发由 MC/DFI 负责） */
int ddrc_zq_config(void);

/* ctrlupd 请求（触发位待确认） */
int ddrc_ctrlupd_request(uint32_t timeout_us);

/* 状态 / 错误 / 调试 */
int  ddrc_get_status(uint32_t *sw_done_ack, uint32_t *mr_busy);
int  ddrc_last_error(void);
void ddrc_dump_regs(void);

#endif /* __DDRC_H__ */
