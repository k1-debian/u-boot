/*
 * ddr.h - DDR 公共头文件
 *
 * 统一错误码 / 平台适配接口 / 总入口声明。
 * 寄存器定义见 ddrc_regs.h（DDRC）与 ddrp_regs.h（PHY），
 * 参数协议见 ddr_param.h（工具生成）。
 */
#ifndef __DDR_H__
#define __DDR_H__

#include "ddr_param.h"
#include "ddrc_regs.h"
#include "ddrp_regs.h"

/* ============================ 统一错误码 ============================ */
#define DDR_OK              0
#define DDR_ERR_PARAM      (-1)
#define DDR_ERR_TIMEOUT    (-2)
#define DDR_ERR_BUSY       (-3)
#define DDR_ERR_NOT_INIT   (-4)
#define DDR_ERR_FAIL       (-5)	/* 硬件报了失败（如训练 error 位） */

/* ============================ 平台适配接口 ============================ */
/*
 * 由平台层（SoC/BSP）实现，DDR 驱动不直接接触时钟/供电/引脚：
 *   ddr_platform_init    : SoC 级 DDR 前置（时钟源/供电/引脚/efuse）
 *   ddr_platform_set_clk : 设置 DDR 时钟频率（hz）
 *   ddr_platform_delay   : 微秒延时
 *   ddr_platform_log     : 调试打印
 * 未实现时可在链接层提供空实现（延时空实现会导致轮询死等，务必提供）。
 *
 * 在 u-boot 里直接映射到原生接口，避免各处出现未定义符号：
 *   udelay() / printf() 都是 u-boot 自带的。
 */
#include <common.h>

#define ddr_platform_delay(us)	udelay(us)
#define ddr_platform_log(...)	printf(__VA_ARGS__)

/* ============================ 总入口 ============================ */
/* DDR 初始化总入口（控制器 + PHY + 颗粒），由启动流程调用 */
void sdram_init(void);

#endif /* __DDR_H__ */
