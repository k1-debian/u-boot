#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		20
#define	TSHSL_W		20

#define TRD		600
#define TPP		1000
#define TBE		5

static struct jz_sfcnand_device *xcsp_mid8c_nand;

static struct jz_sfcnand_base_param xcsp_mid8c_param[] = {

	[0] = {
		/*XCSP1AAPK-IT*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		/*Block 1013 ~ 1023 (10 blocks) are restricted area.*/
		.flashsize = 2 * 1024 * 64 * 1024 - (10 * 2 * 1024 * 64),

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x8,
		.need_quad = 1,
	},

	[1] = {
		/*XCSP2AAPK-IT*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = 150,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 0,
		.ecc_max = 0x8,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x01, "XCSP1AAPK-IT", &xcsp_mid8c_param[0]),
	DEVICE_ID_STRUCT(0xA1, "XCSP2AAPK-IT", &xcsp_mid8c_param[1]),
};


static cdt_params_t *xcsp_mid8c_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(xcsp_mid8c_nand->cdt_params);

	switch(device_id) {
		case 0x01:
		case 0xA1:
			break;
		default:
			pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}

	return &xcsp_mid8c_nand->cdt_params;
}

static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{
	int ret = 0;
	switch(device_id) {
		case 0x01:
		case 0xA1:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
					return 0;
				case 0x1:
					return 4;
				case 0x2:
					return -EBADMSG;
				case 0x3:
					return 8;
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


static int xcsp_mid8c_nand_init(void)
{
	xcsp_mid8c_nand = kzalloc(sizeof(*xcsp_mid8c_nand), GFP_KERNEL);
	if(!xcsp_mid8c_nand) {
		pr_err("alloc xcsp_mid8c_nand struct fail\n");
		return -ENOMEM;
	}

	xcsp_mid8c_nand->id_manufactory = 0x8C;
	xcsp_mid8c_nand->id_device_list = device_id;
	xcsp_mid8c_nand->id_device_count = ARRAY_SIZE(xcsp_mid8c_param);

	xcsp_mid8c_nand->ops.get_cdt_params = xcsp_mid8c_get_cdt_params;
	xcsp_mid8c_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	xcsp_mid8c_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(xcsp_mid8c_nand);
}

SPINAND_MOUDLE_INIT(xcsp_mid8c_nand_init);
