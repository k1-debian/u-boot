/*
 * debug_stub_param_layout.h —— **自动生成，不要手工修改**
 * 生成者：ds_x3000/tools/ext_param_fields.py
 *
 * SPL 侧只需要知道：每个参数域有多少个 32 位字、多大、以及字段表指纹。
 * 字段名只有上位机用 —— SPL 不存名字表，省掉几百字节 rodata。
 */
#ifndef __DEBUG_STUB_PARAM_LAYOUT_H__
#define __DEBUG_STUB_PARAM_LAYOUT_H__

/* 域 ddr：struct ddr_param（arch/mips/cpu/xburst2/x3000/ddr/ddr_param.h）*/
#define DDR_PARAM_WORDS      228
#define DDR_PARAM_SIZE       912
#define DDR_PARAM_LAYOUT_TAG 0x4497

/* 域 clk：struct x3000_clk_values（arch/mips/cpu/xburst2/x3000/x3000_clk_values.h）*/
#define CLK_PARAM_WORDS      17
#define CLK_PARAM_SIZE       68
#define CLK_PARAM_LAYOUT_TAG 0x4b0d

#endif /* __DEBUG_STUB_PARAM_LAYOUT_H__ */
