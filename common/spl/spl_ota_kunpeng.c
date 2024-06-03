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

struct nv_flags {
    unsigned int version;
    unsigned int boot;
    unsigned int step;
    unsigned int start;
    unsigned int finish;
    unsigned int needfullpkg;
    unsigned int rot_angle;
    unsigned int partition;
    unsigned int reservedspace[14];
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

char* spl_ota_load_image(void)
{
	char *cmdargs = NULL;
	unsigned int addr = 0;
	unsigned int bootimg_addr = 0;
	struct jz_sfcnand_partition_param *partitions;
	struct nv_flags nv;
    int len;
    unsigned short degree;
    unsigned int dec_degree;
    unsigned int hightBits;

	ota_init();
	partitions = ota_ops->flash_get_partitions();
	addr = ota_ops->flash_get_part_offset_by_name(partitions, CONFIG_PAT_NV_NAME);
	nv_read(addr, (unsigned int)&nv, sizeof(struct nv_flags));

	printf("NV FLAGS:\n nv.boot \t%x\n nv.step \t%x\n nv.start \t%x\n nv.end \t%x\n nv.needfullpkg \t%x\n nv.rot_angle \t%x\n nv.partition \t%x\n",
			nv.boot, nv.step, nv.start, nv.finish, nv.needfullpkg, nv.rot_angle, nv.partition);

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

#ifdef CONFIG_NV_ROTATE
    {
        static char buffer[512];
        hightBits = nv.rot_angle & 0xFFFF0000;
        if (hightBits == 0xEEEE0000) {
            degree = nv.rot_angle & 0x0000FFFF;
            dec_degree = (int)degree;
            if (dec_degree == 0 || dec_degree == 90 || dec_degree == 180 || dec_degree == 270) {
                len = snprintf(buffer, sizeof(buffer), "%s rot_angle=%d", cmdargs, dec_degree);
                if (len >= sizeof(buffer)) {
                    printf("nv rot_angle failed!\n", len);
                } else {
                    cmdargs = buffer;
                }
            }
        }
    }
#endif

    ota_ops->flash_load_kernel(bootimg_addr);
    return cmdargs;
}
