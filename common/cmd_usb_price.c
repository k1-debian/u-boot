#include <common.h>
#include <command.h>
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
struct slave_share_mem
{
    int debug;
    int rot;
    int sn_len;
    char sn[64];
    int mac_len;
    char mac[32];
    int logo_len;
    void* logo;
};

#define CCU_IO_BASE			0xb2200000
#define ccu_inl(addr) *(volatile unsigned int *)(addr)
#define ccu_outl(val,addr) *(volatile unsigned int *)(addr)=(unsigned int)val

#define get_ccu_csrr()			ccu_inl(CCU_IO_BASE + 0x40)
#define set_ccu_csrr(val)		ccu_outl(val, CCU_IO_BASE + 0x40)

static int do_usb_price(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long addr, offset;
    struct binhead *head = (struct binhead *)(CONFIG_SYS_TEXT_BASE + 12);
	int len, rc;
    struct slave_share_mem *share = (struct slave_share_mem*)(CONFIG_SYS_TEXT_BASE + CONFIG_LAYOUT_SHARE_START);
    gpio_direction_output(RESET_PIN,0);
    usb_stop();
    gpio_direction_output(RESET_PIN,1);
    printf("shared: rot = %d\n",share->rot);
    printf("shared: sn len = %d\n",share->sn_len);
    printf("shared: mac len = %d\n",share->mac_len);
    printf("shared: logo len = %d\n",share->logo_len);

    if(usb_init() >= 0) {
        usb_load_scan(1);
        usb_load_run_stage1_firmware((unsigned char*)(CONFIG_SYS_TEXT_BASE + CONFIG_LAYOUT_SPL_BIN_OFF), CONFIG_DEV_SPL_START, head->spl_len);

        usb_load_run_send_data((unsigned char*)&share->rot, CONFIG_DEV_ROT_START,4);
        if(share->sn_len)
            usb_load_run_send_data((unsigned char*)share->sn, CONFIG_DEV_SN_START,share->sn_len + 1);
        if(share->logo_len)
            usb_load_run_send_data((unsigned char*)share->logo, CONFIG_DEV_LOGO_START,share->logo_len);

        usb_load_run_stage2_firmware((unsigned char*)(CONFIG_SYS_TEXT_BASE + CONFIG_LAYOUT_KENEL_BIN_OFF), CONFIG_DEV_KERNEL_START, head->kernel_len);

        printf("download ok!\n");
        while(1) asm volatile ("wait \t\n");
    }

	return 0;
}

U_BOOT_CMD(
	usbprice, 1, 0, do_usb_price,
	"load binary data from usb host.",
    "usbprice"
);
