#include <stdio.h>
#include "nand_common.h"

#define WINBOND_MID			    0xEF
#define WINBOND_NAND_DEVICD_COUNT	    4

static unsigned char winbond_eccerr[] = {0x2, 0x3};

static struct device_struct device[] = {
	DEVICE_STRUCT(0xAA21, 2048, 2, 4, 2, 2, winbond_eccerr),
	DEVICE_STRUCT(0xAA22, 2048, 2, 4, 2, 2, winbond_eccerr),
	DEVICE_STRUCT(0xAB21, 2048, 2, 4, 2, 2, winbond_eccerr),
	DEVICE_STRUCT(0xAE21, 2048, 2, 4, 1, 2, winbond_eccerr),
};

static struct nand_desc winbond_nand = {

	.id_manufactory = WINBOND_MID,
	.device_counts = WINBOND_NAND_DEVICD_COUNT,
	.device = device,
};

int winbond_nand_register_func(void) {
	return nand_register(&winbond_nand);
}
