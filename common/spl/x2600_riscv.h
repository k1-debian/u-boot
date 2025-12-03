#ifndef __X2600_RISCV_H__
#define __X2600_RISCV_H__

typedef int (*load_p)(unsigned int src_addr, unsigned int count, unsigned int dst_addr);
void spl_load_riscv(load_p func_p, unsigned int offset, unsigned long args_addr);
void riscv_reset(void);

#endif
