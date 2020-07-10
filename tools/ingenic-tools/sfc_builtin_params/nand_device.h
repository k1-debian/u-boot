#ifndef  __NAND_DEVICE_H
#define  __NAND_DEVICE_H

#include <asm/arch/spinand.h>

typedef struct nand_partition_builtin_params {
	uint32_t magic_num;
	int32_t partition_num;
	struct jz_sfcnand_partition partition[];
}nand_partition_builtin_params_t;


#endif
