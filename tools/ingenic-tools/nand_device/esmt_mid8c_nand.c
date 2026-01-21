#include <stdio.h>
#include "nand_common.h"


static unsigned char esmt_errstat_2[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x2C, 2048, 2, 4, 2, 1, esmt_errstat_2, 0),
};

static struct nand_desc esmtlc_nand = {

	.id_manufactory = 0x8C,
	.device_counts = ARRAY_SIZE(device),
	.device = device,
};

int esmtlc_nand_register_func(void) {
	return nand_register(&esmtlc_nand);
}
