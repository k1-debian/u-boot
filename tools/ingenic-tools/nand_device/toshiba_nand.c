#include <stdio.h>
#include "nand_common.h"

#define TOSHIBA_MID              0x98

#define TOSHIBA_NAND_DEVICE_COUNT	    1


static unsigned char toshiba_eccerr[] = {0x2, 0x3};

static struct device_struct device[TOSHIBA_NAND_DEVICE_COUNT] = {
        DEVICE_STRUCT(0xed, 4096, 2, 4, 2, 1, toshiba_eccerr),
 };


static struct nand_desc toshiba_nand = {
	.id_manufactory = TOSHIBA_MID,
	.device_counts  = TOSHIBA_NAND_DEVICE_COUNT,
	.device = device,
};

int toshiba_nand_register_func(void) {
	return nand_register(&toshiba_nand);
}
