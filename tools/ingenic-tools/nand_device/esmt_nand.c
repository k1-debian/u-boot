#include <stdio.h>
#include "nand_common.h"


static unsigned char esmt_errstat_2[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x01, 2048, 2, 4, 2, 1, esmt_errstat_2, 0),
};

static struct nand_desc esmt_nand = {

	.id_manufactory = 0xC8,
	.device_counts = ARRAY_SIZE(device),
	.device = device,
};

int esmt_nand_register_func(void) {
	return nand_register(&esmt_nand);
}
