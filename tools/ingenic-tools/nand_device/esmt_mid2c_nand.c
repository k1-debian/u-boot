#include <stdio.h>
#include "nand_common.h"


static unsigned char esmt_2c_errstat_2[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x34, 4096, 2, 4, 3, 1, esmt_2c_errstat_2, 1),
};

static struct nand_desc esmt_2c_nand = {

	.id_manufactory = 0x2C,
	.device_counts  = ARRAY_SIZE(device),
	.device = device,
};

int esmt_mid2c_nand_register_func(void) {
	return nand_register(&esmt_2c_nand);
}
