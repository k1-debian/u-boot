#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpm.h>
#include <mmc.h>
#include <asm/arch/mmc.h>
#include "spl_pdma_mcu.h"

//#define DEBUG 1

void spl_mmc_load_mcu(void)
{
    unsigned int emmc_sector = MCU_BIN_MMC_ADDR / 512; // 计算扇区号
    unsigned int program_size = MCU_BIN_SIZE;
    void *dst_ptr = (void *)(TCSM_BANK0);
    unsigned char buffer[MCU_BIN_SIZE];

    // 验证程序大小是否超过 TCSM 的大小
    if (program_size > 8192) { // TCSM 大小为 8KB
        printf("Error: Program size exceeds TCSM size.\n");
        return;
    }

    // 计算要读取的扇区数量
    int blkcnt = (program_size + 511) / 512; // 向上取整
     // 获取 eMMC 设备
    struct mmc *mmc = find_mmc_device(0);
    if (!mmc) {
        printf("Error: Failed to find MMC device.\n");
        return;
    }

    if (mmc->block_dev.block_read(0, emmc_sector, blkcnt,dst_ptr) != blkcnt) {
        printf("Error: Failed to read from eMMC.\n");
        return;
    }
}
void spl_start_mcu(void)
{
	int ret = 0;
	serial_debug("mcuboot for x16xx.\n");
#if DEBUG
	serial_debug("clkgate0: %x\n", REG32(CPM_BASE + CPM_CLKGR0));
	serial_debug("clkgate1: %x\n", REG32(CPM_BASE + CPM_CLKGR1));
#endif
	REG32(CPM_BASE + CPM_CLKGR0) &= ~(1 << 21); //pdma
//	REG32(CPM_BASE + CPM_CLKGR1) &= ~(1 << 26); //intrc
#if DEBUG
	serial_debug("clkgate0: %x\n", REG32(CPM_BASE + CPM_CLKGR0));
	serial_debug("clkgate1: %x\n", REG32(CPM_BASE + CPM_CLKGR1));
#endif
	reset_mcu();
	serial_debug("%s %d: 0x%x\n",__func__,__LINE__,REG32(PDMA_BASE + DMCS_OFF));
}
