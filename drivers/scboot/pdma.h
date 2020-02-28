#ifndef _PDMA_H_
#define _PDMA_H_

#include <common.h>
#if defined(CONFIG_X1000)
#include "x1000/jz_pdma.h"
#elif defined(CONFIG_X2000_V12)
#include "x2000_v12/jz_pdma.h"
#endif

#define SE_PASS 0
#define SE_FAILURE 1
//#define REG32(addr)	*((volatile unsigned int *)(addr))
#define reset_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 1)
#define boot_up_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 0)
#define sc_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 0x8)
#endif
