#ifndef __X3000_EFUSE_H__
#define __X3000_EFUSE_H__

#include <asm/arch/base.h>

#define EFUSE_CTRL		0x0
#define EFUSE_CFG		0x4
#define EFUSE_STATE		0x8
#define EFUSE_DATA(n)		(0xc + (n) * 4)

#define EFUSE_CFG_RD_ADJ	24
#define EFUSE_CFG_RD_STROBE	16

#define EFUSE_CTRL_ADDR		21
#define EFUSE_CTRL_LEN		16
#define EFUSE_CTRL_RDEN		(1 << 0)

#define EFUSE_STA_RD_DONE	(1 << 0)
#define EFUSE_STA_WR_DONE	(1 << 1)

#define TRIM1_OFFSET_ADDR	0x61

#endif /* __X3000_EFUSE_H__ */
