#ifndef __SPL_READ_RESERVED_H__
#define __SPL_READ_RESERVED_H__

#include "spl_ota_utils.h"

struct reserved_info {
    unsigned int len;
    unsigned int crc;
    unsigned char data[0];
} ;

int spl_read_nandsize(struct ota_ops *ota_ops,int *nand_size);
int spinand_read_reserve(struct ota_ops* ota_ops, unsigned int addr, char *buf, int len);
#endif /* !SPL_READ_RESERVED_H */
