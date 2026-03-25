#include <stdio.h>
#include "nand_common.h"


static unsigned char mk_errstat_1[]= {0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x0A, 2048, 2, 4, 2, 1, mk_errstat_1, 0),
	DEVICE_STRUCT(0x0B, 2048, 2, 4, 2, 1, mk_errstat_1, 0),
};

static struct nand_desc mk_nand = {

	.id_manufactory = 0xF2,
	.device_counts = ARRAY_SIZE(device),
	.device = device,
};

int mk_nand_register_func(void) {
	return nand_register(&mk_nand);
}
