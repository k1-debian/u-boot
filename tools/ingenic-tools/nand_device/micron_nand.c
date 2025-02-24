#include <stdio.h>
#include "nand_common.h"


static unsigned char micron_errstat_2[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x15, 2048, 2, 4, 3, 1, micron_errstat_2, 1),
};

static struct nand_desc micron_nand = {

	.id_manufactory = 0x2C,
	.device_counts  = ARRAY_SIZE(device),
	.device = device,
};

int micron_nand_register_func(void) {
	return nand_register(&micron_nand);
}
