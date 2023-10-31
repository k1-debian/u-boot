#include <stdio.h>
#include "nand_common.h"

#define KOWIN_MID			    0x01
#define KOWIN_NAND_DEVICD_COUNT	    1

static unsigned char kowin_errstat[]= {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0x15, 2048, 2, 4, 2, 2, kowin_errstat),
};

static struct nand_desc kowin_nand = {

	.id_manufactory = KOWIN_MID,
	.device_counts = KOWIN_NAND_DEVICD_COUNT,
	.device = device,
};

int kowin_nand_register_func(void) {
	return nand_register(&kowin_nand);
}
