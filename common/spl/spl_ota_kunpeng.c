#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <linux/err.h>
#include <malloc.h>
#include <div64.h>
#include <linux/string.h>
#include <asm/arch/cpm.h>
#include "spl_ota_kunpeng.h"
#include "spl_read_reserved.h"
#include "ccu.h"

#define ENV_DATA_SIZE 1024

typedef struct env_flags {
    unsigned char   flags;                    /* active/obsolete flags    */
    unsigned char   data[ENV_DATA_SIZE];      /* Environment data     */
} env_t;


struct nv_flags {
    unsigned int version;
    unsigned int boot;
    unsigned int step;
    unsigned int start;
    unsigned int finish;
    unsigned int needfullpkg;
    unsigned int rot_angle;
    unsigned int partition;
    unsigned int slave_rot_angle;
    unsigned int reservedspace[13];
    env_t env;
};

static struct ota_ops *ota_ops = NULL;
void register_ota_ops(struct ota_ops *ops)
{
	ota_ops = ops;
}

static void ota_init(void)
{
	if (ota_ops->flash_init)
		ota_ops->flash_init();
}

static void nv_read(unsigned int src, unsigned int dst, unsigned int len)
{
	ota_ops->flash_read(src, len, dst);
}
#ifndef CONFIG_OTA_ABUPDATE
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
#endif
#define ARGS_BUFFER_MAX_SIZE 256
#define SN_MAX_SIZE 64
#define MAC_MAX_SIZE 32
#define INGENIC_LOGO_MAGIC 0x4c4f474fU

static char *args_buffer = (char *)0x80001000;
static char *sn_buffer   = (char *)0x80001000 + 256;
static char *mac_buffer  = (char *)0x80001000 + 256 + 64;


#define NAND_SN_OFF(sz) ((sz) - 2 * 1024 * 1024)
#define NAND_MAC_OFF(sz) ((sz) - 3 * 1024 * 1024)

static void spl_ota_load_deviceinfo(void)
{
    int nandsize = 0;
    memset(sn_buffer, 0, SN_MAX_SIZE);
    memset(mac_buffer, 0, MAC_MAX_SIZE);

#ifdef CONFIG_READ_SN
    if(nandsize == 0) {
        if(spl_read_nandsize(ota_ops,&nandsize) != 0)
            nandsize = 0;
    }

    if(NAND_SN_OFF(nandsize) > 0) {
        if(spinand_read_reserve(ota_ops,NAND_SN_OFF(nandsize),sn_buffer,SN_MAX_SIZE) != 0) {
            sn_buffer[0] = 0;
        }
    }
#endif
#ifdef CONFIG_READ_MAC
    if(nandsize == 0) {
        if(spl_read_nandsize(ota_ops,&nandsize) != 0)
            nandsize = 0;
    }

    if(NAND_MAC_OFF(nandsize) > 0) {
        if(spinand_read_reserve(ota_ops,NAND_MAC_OFF(nandsize),mac_buffer,MAC_MAX_SIZE) != 0) {
            mac_buffer[0] = 0;
        }
    }
#endif
}
#ifdef CONFIG_SLAVE_CORE_LOAD
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

struct logo_blob_header {
	int width;
	int height;
	int bpp;
	unsigned int p8;
	unsigned int background_color;
} __attribute__ ((packed));

extern int sfc_nand_load(unsigned int src_addr, unsigned int count,
                         unsigned int dst_addr);
extern void flush_cache_all(void);

static int spl_ota_prepare_logo(unsigned int logo_ddr, unsigned int logo_size)
{
	struct logo_blob_header *hdr = (struct logo_blob_header *)logo_ddr;
	u64 payload_size;

	if (logo_size < sizeof(*hdr))
		return -EINVAL;

	if (hdr->p8 != INGENIC_LOGO_MAGIC) {
		if (hdr->p8 != 0)
			return -EINVAL;

		/* Upgrade legacy blobs in DDR so both kernels see the same format. */
		hdr->p8 = INGENIC_LOGO_MAGIC;
	}

	if (hdr->width <= 0 || hdr->height <= 0)
		return -EINVAL;

	if (hdr->bpp != 16 && hdr->bpp != 32)
		return -EINVAL;

	payload_size = (u64)hdr->width * hdr->height * (hdr->bpp / 8);
	if (payload_size > logo_size - sizeof(*hdr))
		return -EINVAL;

	return 0;
}

static void spl_ota_load_slavecore (struct jz_sfcnand_partition_param *partitions,char* name)
{
    unsigned int addr;
    unsigned int len = 0;
    addr = ota_ops->flash_get_part_offset_by_name(partitions, name);
    if(addr && addr != -1) {
        unsigned int reset;
        int cpu = 1;
        sfc_nand_load(addr, 2048, CONFIG_SLAVE_CORE_START);
        len = *(unsigned int *)(CONFIG_SLAVE_CORE_START + 12);
        sfc_nand_load(addr, len, CONFIG_SLAVE_CORE_START);
        flush_cache_all();
        set_ccu_rer(CONFIG_SLAVE_CORE_START & 0x1fffffff);
        reset = get_ccu_csrr();
        reset |= 1 << cpu;
        set_ccu_csrr(reset);
        reset &= ~(1 << cpu);
        set_ccu_csrr(reset);
    } else {
        printf("WARNING: %s not finded!\n",name);
    }
}
#endif

char* spl_ota_load_image(void)
{
	char *cmdargs = NULL;
	unsigned int addr = 0;
	unsigned int bootimg_addr = 0;
	struct jz_sfcnand_partition_param *partitions;
	struct nv_flags nv = {0};
	char *kname = NULL;
    char *kname_addr  = NULL;
    char *core1_name = NULL;
    unsigned int logo_addr = 0;
    unsigned int logo_size = 0;
    unsigned int ximage_logo_ddr = 0;
	struct reserved_info *sn_info = (struct reserved_info *)sn_buffer;
	struct reserved_info *mac_info = (struct reserved_info *)mac_buffer;
	ota_init();
	partitions = ota_ops->flash_get_partitions();

	addr = ota_ops->flash_get_part_offset_by_name(partitions, CONFIG_PAT_NV_NAME);
    if(addr != (unsigned int)-1) {
        nv_read(addr, (unsigned int)&nv, sizeof(struct nv_flags));
    }

	printf("NV FLAGS:\n nv.boot \t%x\n nv.step \t%x\n nv.start \t%x\n nv.end \t%x\n nv.needfullpkg \t%x\n nv.rot_angle \t%x\n nv.partition \t%x\n",
			nv.boot, nv.step, nv.start, nv.finish, nv.needfullpkg, nv.rot_angle, nv.partition);
/* load logo数据 */
#ifdef CONFIG_SFC_LOAD_LOGO
	logo_addr = get_part_offset_by_name(partitions, CONFIG_XIMAGE_LOGO_NAME);
	logo_size = get_part_size_by_name(partitions, CONFIG_XIMAGE_LOGO_NAME);
    ximage_logo_ddr = CONFIG_XIMAGE_LOGO_DDR;
	if (logo_size == -1){
		serial_debug("LOGO not found: "CONFIG_XIMAGE_LOGO_NAME"\n");
		hang();
	}
	sfc_nand_load(logo_addr, logo_size, (void *)ximage_logo_ddr);
	if (spl_ota_prepare_logo(ximage_logo_ddr, logo_size)) {
		printf("WARNING: invalid logo header at 0x%x, ignore external logo\n",
		       ximage_logo_ddr);
		memset((void *)ximage_logo_ddr, 0, sizeof(struct logo_blob_header));
		logo_size = 0;
	}
#endif
    spl_ota_load_deviceinfo();

#ifdef CONFIG_OTA_ABUPDATE
    /* AB partition upgrade */

#define SLPC_BASIC_COUNT 0xaa55aa00
#define CHANGE_NUM 6
#define MAX_NUM 12
#define PARTITIONA 0
#define PARTITIONB 0xa5

#ifdef CONFIG_OTA_ABUPDATE_ROLLBACK
	unsigned int rsr_data;
	unsigned int slpc_data;
	/*CPM_SLPC software restart without loss*/
	slpc_data = cpm_readl(CPM_SLPC);
	printf("slpc:%x\n",slpc_data);
	if ((slpc_data >> 8) == (SLPC_BASIC_COUNT >> 8)){
		if ((slpc_data & 0xff) >= CHANGE_NUM) {
			nv.partition = nv.partition == PARTITIONA ? PARTITIONB : PARTITIONA;
			if ((slpc_data & 0xff) >= MAX_NUM) {
				while(1){
					printf("Both partitions A/B failed to start!!! \n");
				}
			}
		}
		cpm_writel(++slpc_data, CPM_SLPC);
	} else {
		cpm_writel(SLPC_BASIC_COUNT, CPM_SLPC);
	}
#else
	cpm_writel(SLPC_BASIC_COUNT, CPM_SLPC);
#endif

	if (nv.partition == PARTITIONB) {
		kname = CONFIG_PATB_KERNEL_NAME;
        core1_name = "slavecoreB";
		cmdargs = CONFIG_SPL_BOOT_PARTITION_B;
		printf("The startup area for this time is partitionB !!! \n");
	} else {
		kname = CONFIG_PATA_KERNEL_NAME;
		cmdargs = CONFIG_SPL_BOOT_PARTITION_A;
        core1_name = "slavecoreA";
		printf("The startup area for this time is partitionA !!! \n");
	}
    kname_addr = kname;
#else
	/* recovery Upgrade method */
    if(get_signature(RECOVERY_SIGNATURE) || (nv.start == 0x5a5a5a5a)) {
		if(nv.boot) {
            kname_addr = CONFIG_PAT_RECOVERY_NAME;
			cmdargs = CONFIG_SYS_SPL_OTA_ARGS_ADDR;
		} else {
			kname_addr = CONFIG_PAT_KERNEL_NAME;
			cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;
		}
	} else {
        kname_addr = CONFIG_PAT_KERNEL_NAME;
		cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;
	}
    core1_name = "slavecore";
    kname = CONFIG_PAT_KERNEL_NAME;
#endif

#ifdef CONFIG_SLAVE_CORE_LOAD
    struct slave_share_mem *share = (struct slave_share_mem *)CONFIG_SLAVE_SHARE_START;
    memset(share,0,sizeof(struct slave_share_mem));
    share->debug = 0;
    share->rot = nv.slave_rot_angle;
    if(sn_info->len > 0 && sn_info->len < SN_MAX_SIZE) {
        share->sn_len = sn_info->len;
        memcpy(share->sn,sn_info->data,sn_info->len);
    }
    if(mac_info->len > 0 && mac_info->len < MAC_MAX_SIZE) {
        share->mac_len = mac_info->len;
        memcpy(share->sn,mac_info->data,mac_info->len);
    }
    if(logo_size > 0 && logo_size != (unsigned int)-1) {
        share->logo_len = logo_size;
        share->logo = ximage_logo_ddr;
    }
    spl_ota_load_slavecore (partitions,core1_name);
#endif

	bootimg_addr = ota_ops->flash_get_part_offset_by_name(partitions, kname_addr);
    if(bootimg_addr && bootimg_addr != -1)
        ota_ops->flash_load_kernel(bootimg_addr, kname);
    else
        printf("ERROR: kernel address error!\n");

#if defined(USE_NV_CMDARGS) && \
	(defined(CONFIG_NV_ROTATE) || defined(CONFIG_READ_SN) || defined(CONFIG_READ_MAC))
#error "USE_NV_CMDARGS cannot be used with CONFIG_NV_ROTATE, CONFIG_READ_SN, or CONFIG_READ_MAC"
#endif

    {
        int len = strlen(cmdargs);
        memcpy(args_buffer,cmdargs,len);
        args_buffer[len] = 0;
        cmdargs = args_buffer;
    }
#ifdef CONFIG_NV_ROTATE
    {
        unsigned int hightBits,degree,dec_degree;
        hightBits = nv.rot_angle & 0xFFFF0000;
        if (hightBits == 0xEEEE0000) {
            degree = nv.rot_angle & 0x0000FFFF;
            dec_degree = (int)degree;
            if(dec_degree == 0) {
                strcat(cmdargs, " rot_angle=0");
            } else if(dec_degree == 90) {
                strcat(cmdargs, " rot_angle=90");
            } else if(dec_degree == 180) {
                strcat(cmdargs, " rot_angle=180");
            } else if(dec_degree == 270) {
                strcat(cmdargs, " rot_angle=270");
            }
        }
    }
#endif

    if(sn_info->len > 0) {
        strcat(cmdargs, " sn=");
        strcat(cmdargs, (char*)sn_info->data);
    }

    if(mac_info->len > 0) {
        strcat(cmdargs, " mac=");
        strcat(cmdargs, (char*)mac_info->data);
    }

#ifdef CONFIG_USE_NV_CMDARGS
#define ENV_FLAG 0xA5
    static env_t env;
    env_t *env_p = &nv.env;
    if (nv.env.flags == ENV_FLAG) {
        if (nv.env.data[0]!=0 && nv.env.data[0]!=0xff) {
            memcpy(&env, env_p, sizeof(env_t));
            cmdargs = env.data;
            printf("use env new cmdargs\n");
        } else {
            printf("env cmdargs error,use default old cmdargs\n");
        }
    }
#endif

    return cmdargs;
}
