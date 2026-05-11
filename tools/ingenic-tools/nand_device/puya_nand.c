#include <stdio.h>
#include "nand_common.h"


static unsigned char puya_errstat_1[] = {0x2};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x2A, 2048, 2, 4, 2, 1, puya_errstat_1, 0),
};

static struct nand_desc puya_nand = {

	.id_manufactory = 0x85,
	.device_counts = ARRAY_SIZE(device),
	.device = device,
};

int puya_nand_register_func(void) {
	return nand_register(&puya_nand);
}
