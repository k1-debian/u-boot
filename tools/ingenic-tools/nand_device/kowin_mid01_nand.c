#include <stdio.h>
#include "nand_common.h"

#define KOWIN_01_MID			    0x01
#define KOWIN_01_NAND_DEVICD_COUNT	    1

static unsigned char kowin_01_errstat[]= {0x2, 0x3};

static struct device_struct device[KOWIN_01_NAND_DEVICD_COUNT] = {
	DEVICE_STRUCT(0x15, 2048, 2, 4, 2, 2, kowin_01_errstat),
};

static struct nand_desc kowin_01_nand = {

	.id_manufactory = KOWIN_01_MID,
	.device_counts = KOWIN_01_NAND_DEVICD_COUNT,
	.device = device,
};

int kowin_mid01_nand_register_func(void) {
	return nand_register(&kowin_01_nand);
}
