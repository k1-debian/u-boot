#ifndef __SPL_PDMA_MCU_H
#define __SPL_PDMA_MCU_H

#include <linux/types.h>

#define REG32(addr)                             (*(volatile unsigned int *)(addr))

/*pdma mcu load addr and bin size .*/
#define MCU_BIN_MMC_ADDR  0x7D000000
#define MCU_BIN_SIZE 8*1024

/*pdma mcu*/
#define PDMA_BASE				0xB3420000
#define PDMA_TCSM_BANK_BASE			0xB3422000
#define DMCS_OFF				0x1030
#define DMCS					(PDMA_BASE + DMCS_OFF)
#define BANK_SIZE				0x1000
#define TCSM_BANK(x)				(PDMA_TCSM_BANK_BASE + BANK_SIZE * x)
#define TCSM_BANK0                              TCSM_BANK(0)

/*mcu control*/
#define reset_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 1)
#define boot_up_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 0)

void spl_mmc_load_mcu(void);
void spl_start_mcu(void);

#endif
