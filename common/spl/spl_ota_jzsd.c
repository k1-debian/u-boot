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
struct slave_share_mem {
    int debug;
    int rot;
    int sn_len;
    char sn[64];
    int mac_len;
    char mac[32];
    int logo_len;
    void *logo;
};

extern void flush_cache_all(void);

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
    struct slave_share_mem *share = (struct slave_share_mem *)CONFIG_SLAVE_SHARE_START;

    memset(share, 0, sizeof(*share));
}

static void spl_jzsd_load_slavecore(void)
{
    struct spl_ota_slavecore_ops ops;
    unsigned int start_sector;
    int ret;

    ret = spl_get_built_in_gpt_partition("slavecore", &start_sector, NULL);
    if (ret) {
        printf("WARNING: slavecore not found\n");
        return;
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
                                           CONFIG_SLAVE_CORE_START, 1);
    if (ret)
        printf("WARNING: slavecore load failed: %d\n", ret);
}
#endif

void register_jzsd_ota_ops(struct jzsd_ota_ops *ops)
{
    ota_ops = ops;
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
    spl_jzsd_load_slavecore();
#endif

    if(get_signature(RECOVERY_SIGNATURE) || (((struct nv_flags*)nvdata)->start == 0x5a5a5a5a)) {
        if(((struct nv_flags*)nvdata)->boot) {
            ota_ops->jzsd_load_img_from_partition("recovery");
            cmdargs = CONFIG_SYS_SPL_OTA_ARGS_ADDR;
        } else {
            ota_ops->jzsd_load_img_from_partition("kernel");
            cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;
        }
    } else {
        ota_ops->jzsd_load_img_from_partition("kernel");
        cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;
    }

    return cmdargs;
}
