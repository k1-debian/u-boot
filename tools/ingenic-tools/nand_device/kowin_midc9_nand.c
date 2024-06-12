#include <stdio.h>
#include "nand_common.h"

#define KOWIN_C9_MID			    0xC9
#define KOWIN_C9_NAND_DEVICD_COUNT	    1

static unsigned char kowin_c9_errstat[]= {0x3};

static struct device_struct device[KOWIN_C9_NAND_DEVICD_COUNT] = {
	DEVICE_STRUCT(0xD4, 4096, 2, 4, 2, 1, kowin_c9_errstat),
};

static struct nand_desc kowin_c9_nand = {

	.id_manufactory = KOWIN_C9_MID,
	.device_counts = KOWIN_C9_NAND_DEVICD_COUNT,
	.device = device,
};

int kowin_midc9_nand_register_func(void) {
	return nand_register(&kowin_c9_nand);
}
