#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define ZB_DEVICES_NUM         1
#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

#define TRD		400
#define TPP		1000
#define TBE		5

static struct jz_sfcnand_device *zb_nand;

static struct jz_sfcnand_base_param zb_param[ZB_DEVICES_NUM] = {

	[0] = {
		/*ZB35Q01A*/
		.pagesize = 2 * 1024,
		.oobsize = 64,
		.blocksize = 2 * 1024 * 64,
		.flashsize = 2 * 1024 * 64 * 1024,

		.tSETUP = TSETUP,
		.tHOLD  = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 8,
		.need_quad = 1,
	},

};

static struct device_id_struct device_id[ZB_DEVICES_NUM] = {
	DEVICE_ID_STRUCT(0x41, "ZB35Q01A", &zb_param[0]),
};


static cdt_params_t *zb_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(zb_nand->cdt_params);

	switch(device_id) {
		case 0x41:
			break;
		default:
			printf("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}

	return &zb_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{
	switch(device_id) {
		case 0x41:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
				case 0x1:
					return 0x0;
				case 0x3:
					return 0x8;
				case 0x2:
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


static int zb_nand_init(void) {

	zb_nand = kzalloc(sizeof(*zb_nand), GFP_KERNEL);
	if(!zb_nand) {
		pr_err("alloc zb_nand struct fail\n");
		return -ENOMEM;
	}

	zb_nand->id_manufactory = 0x5E;
	zb_nand->id_device_list = device_id;
	zb_nand->id_device_count = ZB_DEVICES_NUM;

	zb_nand->ops.get_cdt_params = zb_get_cdt_params;
	zb_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	zb_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(zb_nand);
}

SPINAND_MOUDLE_INIT(zb_nand_init);
