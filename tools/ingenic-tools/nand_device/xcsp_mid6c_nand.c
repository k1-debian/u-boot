#include <stdio.h>
#include "nand_common.h"


static unsigned char xcsp_mid6c_errstat_1[] = {0x02};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x010A, 2048, 2, 4, 2, 1, xcsp_mid6c_errstat_1, 0),
	DEVICE_STRUCT(0xA10A, 2048, 2, 4, 2, 1, xcsp_mid6c_errstat_1, 0),
};

static struct nand_desc xcsp_mid6c_nand = {

	.id_manufactory = 0x6C,
	.device_counts  = ARRAY_SIZE(device),
	.device = device,
};

int xcsp_mid6c_nand_register_func(void) {
	return nand_register(&xcsp_mid6c_nand);
}
