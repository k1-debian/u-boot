
#ifndef __SPL_MCU_RTOS_H__
#define __SPL_MCU_RTOS_H__

#include <common.h>
#include <asm/io.h>

#define LEP_TAG  (('L'<<24)|('E'<<16)|('P'<<8)|('T'<<0))
#define IO_ADDR(x)                ((volatile unsigned long *)(void *)(x))

#define x2600_CCU_CFCR           IO_ADDR(CCU_BASE + 0x0fe0)
#define x2600_CCU_CCSR           IO_ADDR(RISCV_BASE + 0x0000)
#define x2600_CCU_CRER           IO_ADDR(RISCV_BASE + 0x0004)
#define x2600_CCU_FROM_HOST      IO_ADDR(RISCV_BASE + 0x0008)
#define x2600_CCU_TO_HOST        IO_ADDR(RISCV_BASE + 0x000C)
#define x2600_CCU_TIME_L         IO_ADDR(RISCV_BASE + 0x0010)
#define x2600_CCU_TIME_H         IO_ADDR(RISCV_BASE + 0x0014)
#define x2600_CCU_TIME_CMP_L     IO_ADDR(RISCV_BASE + 0x0018)
#define x2600_CCU_TIME_CMP_H     IO_ADDR(RISCV_BASE + 0x001C)
#define x2600_CCU_INTC_MASK_L    IO_ADDR(RISCV_BASE + 0x0020)
#define x2600_CCU_INTC_MASK_H    IO_ADDR(RISCV_BASE + 0x0024)
#define x2600_CCU_INTC_PEND_L    IO_ADDR(RISCV_BASE + 0x0028)
#define x2600_CCU_INTC_PEND_H    IO_ADDR(RISCV_BASE + 0x002c)

#define x2600_CCU_PMA_ADR_0      IO_ADDR(RISCV_BASE + 0x0040)
#define x2600_CCU_PMA_ADR_1      IO_ADDR(RISCV_BASE + 0x0044)
#define x2600_CCU_PMA_ADR_2      IO_ADDR(RISCV_BASE + 0x0048)
#define x2600_CCU_PMA_ADR_3      IO_ADDR(RISCV_BASE + 0x004c)
#define x2600_CCU_PMA_ADR_4      IO_ADDR(RISCV_BASE + 0x0050)
#define x2600_CCU_PMA_ADR_5      IO_ADDR(RISCV_BASE + 0x0054)
#define x2600_CCU_PMA_ADR_6      IO_ADDR(RISCV_BASE + 0x0058)
#define x2600_CCU_PMA_ADR_7      IO_ADDR(RISCV_BASE + 0x005c)

#define x2600_CCU_PMA_CFG_0      IO_ADDR(RISCV_BASE + 0x0060)
#define x2600_CCU_PMA_CFG_1      IO_ADDR(RISCV_BASE + 0x0064)
#define x2600_CCU_PMA_CFG_2      IO_ADDR(RISCV_BASE + 0x0068)
#define x2600_CCU_PMA_CFG_3      IO_ADDR(RISCV_BASE + 0x006c)
#define x2600_CCU_PMA_CFG_4      IO_ADDR(RISCV_BASE + 0x0070)
#define x2600_CCU_PMA_CFG_5      IO_ADDR(RISCV_BASE + 0x0074)
#define x2600_CCU_PMA_CFG_6      IO_ADDR(RISCV_BASE + 0x0078)
#define x2600_CCU_PMA_CFG_7      IO_ADDR(RISCV_BASE + 0x007c)

#define CFCR_LEP_Reset     31, 31

#define CCSR_Timer_en      5, 5
#define CCSR_Reset         4, 4
#define CCSR_Sleep         3, 3
#define CCSR_Bus_idle      2, 2
#define CCSR_Bus_mask      1, 1
#define CCSR_IE            0, 0

#define PMA_CFG_C        5, 5    // cacheable: 过cache
#define PMA_CFG_W        2, 2    // can execute: 可以执行
#define PMA_CFG_R        1, 1    // can read: 可读
#define PMA_CFG_X        0, 0    // can execute: 可以执行
#define PMA_CFG_A        3, 4    // 地址范围类型: 0:off 1:TOR(从上一个到这个) 2:NA4(从这个地址开始的4字节)
                                 // 3:NAPOT(从0位开始数有多少个bit连续是1,假设n个,那么地址范围是2的n+3次方字节)
                                 // 3:NAPOT(数到的n个1全替换成0,就变成了起始地址)
                                 // 注意所有的地址都要乘以4, NAPOT和NA4的范围不乘以4

#define MEM_MAIN 1
#define MEM_IO 2

#define ADDR_OFF   0
#define ADDR_TOR   1
#define ADDR_NA4   2
#define ADDR_NAPOT 3


struct lep_header {
    unsigned long code;
    unsigned long tag;
    unsigned long img_start;
    unsigned long entry;
    unsigned long img_end;
    unsigned long version;

    unsigned long ring_mem_for_host_write;
    unsigned long ring_mem_for_host_read;
    unsigned long uncache_addr;
    unsigned long uncache_size;
};


static inline unsigned long bit_field_max(int start, int end)
{
    return (1ul << (end - start + 1)) - 1;
}


static inline unsigned long bit_field_mask(int start, int end)
{
    return bit_field_max(start, end) << start;
}


static inline int check_bit_field(int start, int end, unsigned long val)
{
    return bit_field_max(start, end) >= val;
}


static inline unsigned long bit_field_val(int start, int end, unsigned long val)
{
    return val << start;
}

static inline unsigned long set_bit_field(unsigned long reg, int start, int end, unsigned long val)
{
    return (reg & ~bit_field_mask(start, end)) | (val << start);
}


static inline unsigned long get_bit_field(unsigned long reg, int start, int end)
{
    return (reg >> start) & bit_field_max(start, end);
}

static inline void set_bit_field_v(volatile unsigned long *reg, int start, int end, unsigned long val)
{
    unsigned long mask = bit_field_mask(start, end);
    *reg = (*reg & ~mask) | ((val << start) & mask);
}

static inline unsigned long get_bit_field_v(volatile unsigned long *reg, int start, int end)
{
    return (*reg & bit_field_mask(start, end)) >> start;
}

static inline unsigned long pma_addr_napot(unsigned long start, unsigned long end)
{
    unsigned long size = end - start;

/*for debug*/
/*
    int n = 31 - __builtin_clz(size);
    unsigned long sz = 1 << n;
    if (sz != size) {
        printf("pma: napot size fix %lx to %lx\n", size, sz);
        size = sz;
    }

    if (start % sz) {
        unsigned long old = start;
        start = start - start % sz;
        printf("pma: napot start fix %lx to %lx\n", old, start);
    }
*/

    start = start >> 2;
    size = size - 1;
    size = size >> 3;
    unsigned long result = start | size;

    return result;
}


static inline unsigned long pma_addr_tor(unsigned long end)
{
    return end >> 2;
}

static inline void lep_stop(void)
{
    set_bit_field_v(x2600_CCU_CCSR, CCSR_Bus_mask, 0);
    set_bit_field_v(x2600_CCU_CFCR, CFCR_LEP_Reset, 1);
}

static inline void lep_start(struct lep_header *header)
{
    *x2600_CCU_INTC_MASK_L = 0;
    *x2600_CCU_INTC_MASK_H = 0;
    *x2600_CCU_FROM_HOST   = 0;
    *x2600_CCU_TO_HOST     = 0;
    *x2600_CCU_CRER = header->entry;

    unsigned long ccsr = *x2600_CCU_CCSR;
    ccsr = set_bit_field(ccsr, CCSR_Timer_en, 0);
    ccsr = set_bit_field(ccsr, CCSR_Bus_mask, 1);
    ccsr = set_bit_field(ccsr, CCSR_IE, 1);
    *x2600_CCU_CCSR = ccsr;


    unsigned long cfg0 = 0;
    if (header->uncache_size) {
        cfg0 = set_bit_field(cfg0, PMA_CFG_R, 1);
        cfg0 = set_bit_field(cfg0, PMA_CFG_W, 1);
        cfg0 = set_bit_field(cfg0, PMA_CFG_X, 1);
        cfg0 = set_bit_field(cfg0, PMA_CFG_C, 0);
        cfg0 = set_bit_field(cfg0, PMA_CFG_A, ADDR_NAPOT);
    }

    unsigned long uncache_start = header->uncache_addr;
    unsigned long uncache_end = header->uncache_addr + header->uncache_size;
    *x2600_CCU_PMA_CFG_0 = cfg0;
    *x2600_CCU_PMA_ADR_0 = pma_addr_napot(uncache_start, uncache_end);

    unsigned long cfg1 = 0;
    cfg1 = set_bit_field(cfg1, PMA_CFG_R, 1);
    cfg1 = set_bit_field(cfg1, PMA_CFG_W, 1);
    cfg1 = set_bit_field(cfg1, PMA_CFG_X, 1);
    cfg1 = set_bit_field(cfg1, PMA_CFG_C, 1);
    cfg1 = set_bit_field(cfg1, PMA_CFG_A, ADDR_NAPOT);
    *x2600_CCU_PMA_CFG_1 = cfg1;
    *x2600_CCU_PMA_ADR_1 = pma_addr_napot(0, 256*1024*1024);

    unsigned long cfg2 = 0;
    cfg2 = set_bit_field(cfg2, PMA_CFG_R, 1);
    cfg2 = set_bit_field(cfg2, PMA_CFG_W, 1);
    cfg2 = set_bit_field(cfg2, PMA_CFG_X, 1);
    cfg2 = set_bit_field(cfg2, PMA_CFG_C, 0);
    cfg2 = set_bit_field(cfg2, PMA_CFG_A, ADDR_NAPOT);
    *x2600_CCU_PMA_CFG_2 = cfg2;
    *x2600_CCU_PMA_ADR_2 = pma_addr_napot(256*1024*1024, 512*1024*1024);

    unsigned long cfg3 = 0;
    cfg3 = set_bit_field(cfg3, PMA_CFG_R, 1);
    cfg3 = set_bit_field(cfg3, PMA_CFG_W, 1);
    cfg3 = set_bit_field(cfg3, PMA_CFG_X, 1);
    cfg3 = set_bit_field(cfg3, PMA_CFG_C, 0);
    cfg3 = set_bit_field(cfg3, PMA_CFG_A, ADDR_TOR);
    *x2600_CCU_PMA_CFG_3 = cfg3;
    *x2600_CCU_PMA_ADR_3 = pma_addr_tor(2048*1024*1024ul);

    cfg0 = 0;
    *x2600_CCU_PMA_CFG_4 = cfg0;
    *x2600_CCU_PMA_CFG_5 = cfg0;
    *x2600_CCU_PMA_CFG_6 = cfg0;
    *x2600_CCU_PMA_CFG_7 = cfg0;

    set_bit_field_v(x2600_CCU_CFCR, CFCR_LEP_Reset, 0);
}

#endif





