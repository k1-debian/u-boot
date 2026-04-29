#ifndef __BOARD_INGENIC_X2600H_HALLEY7_DMVERITY_HELPER_H__
#define __BOARD_INGENIC_X2600H_HALLEY7_DMVERITY_HELPER_H__

#include <dm_verity_boot.h>

int board_dm_verity_helper_get_config(struct dm_verity_boot_config *out);

#endif
