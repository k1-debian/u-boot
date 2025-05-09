#include <stdio.h>
#include "nand_common.h"


static unsigned char cochipgo_errstat_2[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x91, 2048, 2, 4, 3, 1, cochipgo_errstat_2, 0),
	DEVICE_STRUCT(0x81, 2048, 2, 4, 3, 1, cochipgo_errstat_2, 0),
};

static struct nand_desc cochipgo_nand = {

	.id_manufactory = 0xD8,
	.device_counts  = ARRAY_SIZE(device),
	.device = device,
};

int cochipgo_nand_register_func(void) {
	return nand_register(&cochipgo_nand);
}
