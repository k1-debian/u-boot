#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include "spl/spl_slavecore_sync.h"
#include "usb_load.h"

/*******************************************************************************
*   SLAVECORE Layout
*  ┌─────────────────────────────────────────────────┐
*  │ SLAVE CORE                                      │
*  ├─────────────┬─────────────┬─────────────────────┤
*  │ Core1       │ Slave-spl   │ Slave-kernel        │
*  │ (uboot)     │ (spl)       │ (kernel)            │
*  │ 256KB       │ 16KB        │ kernel-size         │
*  └─────────────┴─────────────┴─────────────────────┘
*  说明：
*    - SLAVE CORE：使用 CORE1 USB Load 把slave-spl slave-kernel 烧录到slave cpu上
*    - Core1：运行在 CORE1 上，使用UBOOT的 USB Load程序(cmd_usb_price) 烧录代码到slave cpu 上
*    - Slave-spl： 运行在slave cpu上，完成DDR初始化和Slave Kernel的USB通讯
*    - Slave-kernel：运行在slave cpu上，并mount nfs 初始化根文件系统，然后Switch root
*
*******************************************************************************/

#define RESET_PIN GPIO_PB(28)
struct binhead
{
    unsigned int total_len;
    unsigned int uboot_len;
    unsigned int spl_len;
    unsigned int kernel_len;
};
#define CONFIG_DEV_SPL_START 0x80001800
#define CONFIG_DEV_ROT_START 0x80001000
#define CONFIG_DEV_SN_START 0x80002000
#define CONFIG_DEV_KERNEL_START 0x80f00000
#ifndef CONFIG_DEV_LOGO_START
#error "CONFIG_DEV_LOGO_START is not defined! Please define it in your board header file."
#endif
extern void flush_cache_all(void);

static int do_usb_price(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    struct binhead *head = (struct binhead *)(CONFIG_SYS_TEXT_BASE + 12);
    int ret;
    struct slave_share_mem *share =
        (struct slave_share_mem *)(CONFIG_SYS_TEXT_BASE + CONFIG_LAYOUT_SHARE_START);

    gpio_direction_output(RESET_PIN,0);
    usb_stop();
    gpio_direction_output(RESET_PIN,1);
    printf("shared: rot = %d\n",share->rot);
    printf("shared: sn len = %d\n",share->sn_len);
    printf("shared: mac len = %d\n",share->mac_len);
    printf("shared: logo len = %d\n",share->logo_len);

    share->magic = SLAVECORE_SHARE_MAGIC;
    share->state = SLAVECORE_STATE_TX_RUNNING;
    flush_cache_all();

    ret = usb_init();
    if (ret < 0)
        goto tx_fail;

    ret = usb_load_scan(1);
    if (ret < 0)
        goto tx_fail;

    ret = usb_load_run_stage1_firmware((unsigned char *)(CONFIG_SYS_TEXT_BASE + CONFIG_LAYOUT_SPL_BIN_OFF),
                                       CONFIG_DEV_SPL_START, head->spl_len);
    if (ret)
        goto tx_fail;

    ret = usb_load_run_send_data((unsigned char *)&share->rot, CONFIG_DEV_ROT_START, 4);
    if (ret)
        goto tx_fail;

    if (share->sn_len) {
        ret = usb_load_run_send_data((unsigned char *)share->sn, CONFIG_DEV_SN_START,
                                     share->sn_len + 1);
        if (ret)
            goto tx_fail;
    }

    if (share->logo_len) {
        ret = usb_load_run_send_data((unsigned char *)share->logo, CONFIG_DEV_LOGO_START,
                                     share->logo_len);
        if (ret)
            goto tx_fail;
    }

    ret = usb_load_run_stage2_firmware((unsigned char *)(CONFIG_SYS_TEXT_BASE + CONFIG_LAYOUT_KENEL_BIN_OFF),
                                       CONFIG_DEV_KERNEL_START, head->kernel_len);
    if (ret)
        goto tx_fail;

    usb_stop();
    share->state = SLAVECORE_STATE_TX_DONE;
    flush_cache_all();
    printf("download ok!\n");
    while(1) asm volatile ("wait 	\n");

 tx_fail:
    usb_stop();
    share->state = SLAVECORE_STATE_TX_FAIL;
    flush_cache_all();
    printf("download failed: %d\n", ret);
    while(1) asm volatile ("wait 	\n");

    return 0;
}

U_BOOT_CMD(
	usbprice, 1, 0, do_usb_price,
	"load binary data from usb host.",
    "usbprice"
);
