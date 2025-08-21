#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <linux/err.h>
#include <malloc.h>
#include <div64.h>
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
#if 0
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

static char *args_buffer = (char *)0x80001000;
static char *sn_buffer   = (char *)0x80001000 + 256;
static char *mac_buffer  = (char *)0x80001000 + 256 + 64;


#define NAND_SN_OFF(sz) ((sz) - 2 * 1024 * 1024)
#define NAND_MAC_OFF(sz) ((sz) - 3 * 1024 * 1024)

static void spl_ota_load_deviceinfo(void)
{
    int nandsize = 0;
    sn_buffer[0] = 0;
    mac_buffer[0] = 0;

#ifdef CONFIG_READ_SN
    if(nandsize == 0) {
        if(spl_read_nandsize(ota_ops,&nandsize) != 0)
            nandsize = 0;
    }

    if(NAND_SN_OFF(nandsize) > 0) {
        if(spinand_read_reserve(ota_ops,NAND_SN_OFF(nandsize),sn_buffer,sizeof(sn_buffer)) != 0) {
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
        if(spinand_read_reserve(ota_ops,NAND_MAC_OFF(nandsize),mac_buffer,sizeof(mac_buffer)) != 0) {
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

extern int sfc_nand_load(unsigned int src_addr, unsigned int count,
                         unsigned int dst_addr);
extern void flush_cache_all(void);

static void spl_ota_load_slavecore (struct jz_sfcnand_partition_param *partitions)
{
    unsigned int addr;
    unsigned int len = 0;
    addr = ota_ops->flash_get_part_offset_by_name(partitions, "slavecore");
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
    }
}
#endif

char* spl_ota_load_image(void)
{
	char *cmdargs = NULL;
	unsigned int addr = 0;
	unsigned int bootimg_addr = 0;
	unsigned int  bootimg_size = 0;
	struct jz_sfcnand_partition_param *partitions;
	struct nv_flags nv = {0};
	char *kname;
	struct reserved_info *sn_info = (struct reserved_info *)sn_buffer;
	struct reserved_info *mac_info = (struct reserved_info *)mac_buffer;
	ota_init();
	partitions = ota_ops->flash_get_partitions();
	addr = ota_ops->flash_get_part_offset_by_name(partitions, CONFIG_PAT_NV_NAME);
    if(addr != -1)
        nv_read(addr, (unsigned int)&nv, sizeof(struct nv_flags));
	printf("NV FLAGS:\n nv.boot \t%x\n nv.step \t%x\n nv.start \t%x\n nv.end \t%x\n nv.needfullpkg \t%x\n nv.rot_angle \t%x\n nv.partition \t%x\n",
			nv.boot, nv.step, nv.start, nv.finish, nv.needfullpkg, nv.rot_angle, nv.partition);
/* load logo数据 */
#ifdef CONFIG_SFC_LOAD_LOGO
	bootimg_addr = get_part_offset_by_name(partitions, CONFIG_XIMAGE_LOGO_NAME);
	bootimg_size = get_part_size_by_name(partitions, CONFIG_XIMAGE_LOGO_NAME);
	if (bootimg_addr == -1){
		serial_debug("LOGO not found: "CONFIG_XIMAGE_LOGO_NAME"\n");
		hang();
	}
//	printf("SFC_LOAD_LOGO: bootimg_addr is: %x size: %x\n", bootimg_addr, bootimg_size);
	sfc_nand_load(bootimg_addr, bootimg_size, (void *)CONFIG_XIMAGE_LOGO_DDR);

#endif
    spl_ota_load_deviceinfo();

#ifdef CONFIG_SLAVE_CORE_LOAD
    struct slave_share_mem *share = (struct slave_share_mem *)CONFIG_SLAVE_SHARE_START;
    memset(share,0,sizeof(struct slave_share_mem));
    share->debug = 0;
    share->rot = nv.rot_angle;
    if(share->sn_len) {
        share->sn_len = sn_info->len;
        memcpy(share->sn,sn_info->data,sn_info->len);
    }
    if(share->mac_len) {
        share->mac_len = mac_info->len;
        memcpy(share->sn,mac_info->data,mac_info->len);
    }
    if(bootimg_size > 0) {
        share->logo = (void*)bootimg_addr;
    }
    spl_ota_load_slavecore (partitions);
    //    printf("slave core load finish\n");
    //    while(1);
#endif


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
		cmdargs = CONFIG_SPL_BOOT_PARTITION_B;
		printf("The startup area for this time is partitionB !!! \n");
	} else {
		kname = CONFIG_PATA_KERNEL_NAME;
		cmdargs = CONFIG_SPL_BOOT_PARTITION_A;
		printf("The startup area for this time is partitionA !!! \n");
	}

	bootimg_addr = ota_ops->flash_get_part_offset_by_name(partitions, kname);
    ota_ops->flash_load_kernel(bootimg_addr, kname);

#else
	/* recovery Upgrade method */
    if(get_signature(RECOVERY_SIGNATURE) || (nv.start == 0x5a5a5a5a)) {
		if(nv.boot) {
			bootimg_addr = ota_ops->flash_get_part_offset_by_name(partitions, CONFIG_PAT_RECOVERY_NAME);
			cmdargs = CONFIG_SYS_SPL_OTA_ARGS_ADDR;
		} else {
			bootimg_addr = ota_ops->flash_get_part_offset_by_name(partitions, CONFIG_PAT_KERNEL_NAME);
			cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;
		}
	} else {
		bootimg_addr = ota_ops->flash_get_part_offset_by_name(partitions, CONFIG_PAT_KERNEL_NAME);
		cmdargs = CONFIG_SYS_SPL_ARGS_ADDR;
	}

    ota_ops->flash_load_kernel(bootimg_addr, CONFIG_PAT_KERNEL_NAME);
#endif

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
        strcat(cmdargs, sn_info->data);
    }

    if(mac_info->len > 0) {
        strcat(cmdargs, " mac=");
        strcat(cmdargs, mac_info->data);
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
