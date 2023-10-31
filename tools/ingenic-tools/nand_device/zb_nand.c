#include <stdio.h>
#include "nand_common.h"

#define ZB_MID			    0x5E
#define ZB_NAND_DEVICD_COUNT	    1

static unsigned char zb_errstat[]= {0x2};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x41, 2048, 2, 4, 2, 1, zb_errstat),
};

static struct nand_desc zb_nand = {

	.id_manufactory = ZB_MID,
	.device_counts = ZB_NAND_DEVICD_COUNT,
	.device = device,
};

int zb_nand_register_func(void) {
	return nand_register(&zb_nand);
}
