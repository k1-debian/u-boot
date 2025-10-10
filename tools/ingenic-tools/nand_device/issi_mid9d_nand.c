#include <stdio.h>
#include "nand_common.h"


static unsigned char issi_errstat_1[] = {0x2};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x14, 2048, 2, 4, 1, 1, issi_errstat_1, 0),
	DEVICE_STRUCT(0x24, 2048, 2, 4, 1, 1, issi_errstat_1, 0),
};

static struct nand_desc issi_mid9d_nand = {

	.id_manufactory = 0x9D,
	.device_counts = ARRAY_SIZE(device),
	.device = device,
};

int issi_mid9d_nand_register_func(void) {
	return nand_register(&issi_mid9d_nand);
}
