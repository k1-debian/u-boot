/*
 * x3000_clk_values.h —— X3000 PLL/CCU 的「目标值」（可写的 RAM 镜像）
 *
 * 为什么要有这个头文件（2026-09-19）：
 *   PLL/CCU 的目标值原来是 generated/pll_reg_values.h 里的**编译期宏**，
 *   pllsetting.c 直接内联使用。这样 debug_stub 想在线改时钟就无从下手
 *   （宏改不了），只能自己重写一遍 PLL 锁定时序 —— 没必要。
 *
 *   把值搬进一个**非 const 的 RAM 结构**后：
 *     · pll_init() 的时序代码**一行都不用改**（仍然它自己写寄存器、等锁）；
 *     · debug_stub 只要在 pll_init() 之前改这几个字，就等于换了时钟配置；
 *     · 初值仍然来自 generated/pll_reg_values.h，**默认行为逐位不变**。
 *
 * 本文件是叶子头文件（不依赖任何 u-boot 头），便于被工具解析：
 *   ds_x3000/tools/ext_param_fields.py 从它生成"clk 域"的字索引表。
 */
#ifndef __X3000_CLK_VALUES_H__
#define __X3000_CLK_VALUES_H__

/* 一颗 PLL 的配置，对应 CPAPCR/CPMPCR/CPEPCR 的位段 */
struct x3000_pll {
	unsigned int en;	/* [0]     使能（0 = 该 PLL 不启用） */
	unsigned int m;		/* [25:20] 倍频 M */
	unsigned int n;		/* [19:14] 分频 N */
	unsigned int od0;	/* [10:8]  OD0 */
	unsigned int od1;	/* [13:11] OD1 */
};

struct x3000_clk_values {
	struct x3000_pll apll;	/* 应用 PLL（CPU/SCLK_A 源） */
	struct x3000_pll mpll;	/* 主 PLL（DDR/AHB/外设源） */
	struct x3000_pll epll;	/* 扩展 PLL */
	unsigned int cpccr;	/* CPCCR 目标值：源选择 + CDIV/L2DIV/LEPDIV */
	unsigned int cpccr1;	/* CPCCR1 目标值：AHB0/1/2 的源与分频 + PDIV */
};

extern struct x3000_clk_values g_clk_values;

#endif /* __X3000_CLK_VALUES_H__ */
