#ifndef __BOOTARGS_MEM_H__
#define __BOOTARGS_MEM_H__

const char *bootargs_mem_select(void);
int bootargs_mem_append(char *buf, unsigned int buf_size, const char *base);

#endif
