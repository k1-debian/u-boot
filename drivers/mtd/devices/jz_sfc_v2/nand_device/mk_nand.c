#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define THOLD	    20
#define TSETUP	    20
#define TSHSL_R	    50
#define TSHSL_W	    50 /* write failure with default 20ns */

#define TRD	    380
#define TPP	    600
#define TBE	    5

struct jz_sfcnand_device *mk_nand;

static struct jz_sfcnand_base_param mk_param[] = {
	[0] = {
		/*MKSV1GIL-AE & MKSV1GCL-AC*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 12,
		.need_quad = 1,
	},
	[1] = {
		/*MKSV2GIL-AE*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 12,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x0A, "MKSV1GIL-AE | MKSV1GCL-AC", &mk_param[0]),
	DEVICE_ID_STRUCT(0x0B, "MKSV2GIL-AE", &mk_param[1]),
};

static cdt_params_t *mk_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(mk_nand->cdt_params);
	switch(device_id) {
		case 0x0A:
		case 0x0B:
			break;
		default:
			pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}
	return &mk_nand->cdt_params;
}

static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{
	switch(device_id) {
		case 0x0A:
		case 0x0B:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
				case 0x1:
					return 0;
				case 0x2:
					return 12;
				case 0x3:
					return -EBADMSG;
				default:
					break;
			}
			break;
		default:
			printf("device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			break;
	}
	return -EINVAL;
}


static int mk_nand_init(void) {

	mk_nand = kzalloc(sizeof(*mk_nand), GFP_KERNEL);
	if(!mk_nand) {
		pr_err("alloc mk_nand struct fail\n");
		return -ENOMEM;
	}

	mk_nand->id_manufactory = 0xF2;
	mk_nand->id_device_list = device_id;
	mk_nand->id_device_count = ARRAY_SIZE(mk_param);

	mk_nand->ops.get_cdt_params = mk_get_cdt_params;
	mk_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	mk_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(mk_nand);
}

SPINAND_MOUDLE_INIT(mk_nand_init);
