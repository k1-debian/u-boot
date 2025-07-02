#ifndef __SPL_OTA__
#define __SPL_OTA__

#include "spl_ota_utils.h"
char* spl_ota_load_image(void);
void register_ota_ops(struct ota_ops *ops);

#endif	/* __SPL_OTA__ */
