#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <malloc.h>
#include <div64.h>
#include <asm/arch/cpm.h>
#include "spl_ota_jzsd.h"
#include "spl_gpt_partition.h"
#include "spl_ota_slavecore_loader.h"
#include "spl_slavecore_sync.h"
#include "ccu.h"

static struct jzsd_ota_ops *ota_ops = NULL;

struct nv_flags {
    unsigned int version;
    unsigned int boot;
    unsigned int step;
    unsigned int start;
    unsigned int finish;
    unsigned int needfullpkg;
};

#ifdef CONFIG_SLAVE_CORE_LOAD
#define SLAVECORE_CPU_ID 1
#define SLAVECORE_TX_TIMEOUT_US (5U * 1000U * 1000U)

extern void flush_cache_all(void);

static struct slave_share_mem *spl_jzsd_get_slave_share(void)
{
    return (struct slave_share_mem *)(CONFIG_SLAVE_SHARE_START);
}

static int spl_jzsd_raw_read(void *priv, unsigned int src, unsigned int len,
                             unsigned long dst)
{
    struct jzsd_ota_ops *ops = priv;
    unsigned int blkcnt = (len + 512 - 1) / 512;
    u32 ret;

    ret = ops->jzsd_read(src, blkcnt, (u32 *)dst);
    if (ret != blkcnt)
        return -EIO;

    return 0;
}

static void spl_jzsd_flush_cache(void *priv)
{
    (void)priv;
    flush_cache_all();
}

static unsigned int spl_jzsd_get_reset(void *priv)
{
    (void)priv;
    return get_ccu_csrr();
}

static void spl_jzsd_set_reset(void *priv, unsigned int value)
{
    (void)priv;
    set_ccu_csrr(value);
}

static void spl_jzsd_set_reset_entry(void *priv, unsigned int value)
{
    (void)priv;
    set_ccu_rer(value);
}

static void spl_jzsd_prepare_slave_share(void)
{
    struct slave_share_mem *share = spl_jzsd_get_slave_share();

    spl_slavecore_share_init(share);
    flush_cache_all();
}

static int spl_jzsd_wait_slavecore_tx_done(unsigned int timeout_us)
{
    struct slave_share_mem *share = spl_jzsd_get_slave_share();

    while (timeout_us--) {
        int state = spl_slavecore_check_state(share);

        if (state == SLAVECORE_CHECK_DONE) {
            printf("slavecore tx done\n");
            return 0;
        }

        if (state == SLAVECORE_CHECK_FAIL) {
            printf("WARNING: slavecore tx failed\n");
            return state;
        }

        udelay(1);
    }

    printf("WARNING: slavecore tx timeout\n");
    return -ETIMEDOUT;
}

static void spl_jzsd_reclaim_slavecore_cpu1(void)
{
    unsigned int reset = get_ccu_csrr();

    reset |= 1U << SLAVECORE_CPU_ID;
    set_ccu_csrr(reset);
    printf("slavecore cpu1 reclaimed\n");
}

static int spl_jzsd_load_slavecore(void)
{
    struct spl_ota_slavecore_ops ops;
    unsigned int start_sector;
    int ret;

    ret = spl_get_built_in_gpt_partition("slavecore", &start_sector, NULL);
    if (ret) {
        printf("WARNING: slavecore not found\n");
        return ret;
    }

    spl_jzsd_prepare_slave_share();

    memset(&ops, 0, sizeof(ops));
    ops.priv = ota_ops;
    ops.load = spl_jzsd_raw_read;
    ops.flush_cache_all = spl_jzsd_flush_cache;
    ops.get_reset = spl_jzsd_get_reset;
    ops.set_reset = spl_jzsd_set_reset;
    ops.set_reset_entry = spl_jzsd_set_reset_entry;

    ret = spl_ota_slavecore_load_and_start(&ops, start_sector,
                                           CONFIG_SLAVE_CORE_START, SLAVECORE_CPU_ID);
    if (ret)
        printf("WARNING: slavecore load failed: %d\n", ret);

    return ret;
}
#endif

void register_jzsd_ota_ops(struct jzsd_ota_ops *ops)
{
    ota_ops = ops;
}

static char *spl_jzsd_load_normal_image(void)
{
#if defined(CONFIG_BOARD_DM_VERITY_ENABLE) && CONFIG_BOARD_DM_VERITY_ENABLE
	if (ota_ops && ota_ops->jzsd_load_uboot) {
		//printf("BOOTROOM-HELP: stage2 ota choose dmverity\n");
		ota_ops->jzsd_load_uboot();
		return NULL;
	}
#endif

	//printf("BOOTROOM-HELP: stage2 ota choose kernel\n");
	ota_ops->jzsd_load_img_from_partition("kernel");
	return CONFIG_SYS_SPL_ARGS_ADDR;
}

static void nv_read(unsigned int start, unsigned int blkcnt, unsigned int *dst)
{
    ota_ops->jzsd_read(start, blkcnt, dst);
}

static int get_signature(const int signature)
{
    unsigned int flag = cpm_get_scrpad();

    //printf("RECOVERY_SIGNATURE: %x\n", flag);
    if ((flag & 0xffff) == signature) {
        /*
         * Clear the signature,
         * reset the signature to force into normal boot after factory reset
         */
        cpm_set_scrpad(flag & ~(0xffff));
        return 1;
    }

    return 0;
}

char* spl_jzsd_ota_load_image(void)
{
    char *cmdargs = NULL;
    unsigned int nvdata[512] = {0};
    unsigned int start_sector;
    int ret;

    ret = spl_get_built_in_gpt_partition("nv", &start_sector, NULL);
    if (ret) {
        printf("mmc: failed to get partition nv\n");
        return NULL;
    }

    nv_read(start_sector, 1, nvdata);
#if 0
    int i;
    for(i=0;i<512;i++){
        printf("%x ",nvdata[i]);
        if(!(i%8))
            printf("\n");
    }
#endif
    printf("NV FLAGS:\n nv.boot \t%x\n nv.step \t%x\n nv.start \t%x\n nv.end \t%x\n nv.needfullpkg \t%x\n",
           ((struct nv_flags*)nvdata)->boot, ((struct nv_flags*)nvdata)->step,
           ((struct nv_flags*)nvdata)->start, ((struct nv_flags*)nvdata)->finish,
           ((struct nv_flags*)nvdata)->needfullpkg);

#ifdef CONFIG_SLAVE_CORE_LOAD
    ret = spl_jzsd_load_slavecore();
    if (!ret) {
        ret = spl_jzsd_wait_slavecore_tx_done(SLAVECORE_TX_TIMEOUT_US);
        if (ret)
            printf("WARNING: slavecore tx incomplete: %d\n", ret);
        spl_jzsd_reclaim_slavecore_cpu1();
    }
#endif

	if(get_signature(RECOVERY_SIGNATURE) || (((struct nv_flags*)nvdata)->start == 0x5a5a5a5a)) {
	    if(((struct nv_flags*)nvdata)->boot) {
	        //printf("BOOTROOM-HELP: stage2 ota choose recovery\n");
	        ota_ops->jzsd_load_img_from_partition("recovery");
	        cmdargs = CONFIG_SYS_SPL_OTA_ARGS_ADDR;
	    } else {
	        cmdargs = spl_jzsd_load_normal_image();
	    }
	} else {
	    cmdargs = spl_jzsd_load_normal_image();
	}

    return cmdargs;
}
