#include <stdio.h>
#include "nand_common.h"

#define GSTO_MID		    0x52
#define GSTO_NAND_DEVICD_COUNT	    1

static unsigned char gsto_eccerr[] = {0x2};

static struct device_struct device[] = {
	DEVICE_STRUCT(0xca13, 2048, 2, 4, 2, 1, gsto_eccerr),
};

static struct nand_desc gsto_nand = {

	.id_manufactory = GSTO_MID,
	.device_counts = GSTO_NAND_DEVICD_COUNT,
	.device = device,
};

int gsto_nand_register_func(void) {
	return nand_register(&gsto_nand);
}
