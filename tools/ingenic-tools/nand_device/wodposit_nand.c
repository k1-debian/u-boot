#include <stdio.h>
#include "nand_common.h"


static unsigned char wodposit_errstat_2[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0xA081, 2048, 2, 4, 3, 2, wodposit_errstat_2, 0),
	DEVICE_STRUCT(0xA082, 2048, 2, 4, 3, 2, wodposit_errstat_2, 0),
	DEVICE_STRUCT(0xA083, 1096, 2, 4, 3, 2, wodposit_errstat_2, 0),
};

static struct nand_desc wodposit_nand = {

	.id_manufactory = 0xA5,
	.device_counts  = ARRAY_SIZE(device),
	.device = device,
};

int wodposit_nand_register_func(void) {
	return nand_register(&wodposit_nand);
}
