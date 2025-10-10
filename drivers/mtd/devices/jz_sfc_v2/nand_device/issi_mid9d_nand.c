#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define THOLD	    5
#define TSETUP	    5
#define TSHSL_R	    30
#define TSHSL_W	    30

#define TRD	    100
#define TPP	    800
#define TBE	    10

static struct jz_sfcnand_device *issi_mid9d_nand;

static struct jz_sfcnand_base_param issi_param[] = {
	[0] = {
		/*IS37/38SML01G8B*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 1024,

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
		/*IS37/38SML02G8B*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 128,
		.flashsize = 2 * 1024 * 64 * 2048,

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
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x14, "IS37/38SML01G8B", &issi_param[0]),
	DEVICE_ID_STRUCT(0x24, "IS37/38SML02G8B", &issi_param[1]),
};

static cdt_params_t *issi_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(issi_mid9d_nand->cdt_params);
	switch(device_id) {
		case 0x14:
		case 0x24:
			break;
		default:
			pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}
	return &issi_mid9d_nand->cdt_params;
}

static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{
	switch(device_id) {
		case 0x14:
		case 0x24:
			switch((ecc_status >> 4) & 0x7) {
				case 0x0:
					return 0;
				case 0x1:
					return 3;
				case 0x2:
					return -EBADMSG;
				case 0x3:
					return 6;
				case 0x5:
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


static int issi_mid9d_nand_init(void) {

	issi_mid9d_nand = kzalloc(sizeof(*issi_mid9d_nand), GFP_KERNEL);
	if(!issi_mid9d_nand) {
		pr_err("alloc issi_mid9d_nand struct fail\n");
		return -ENOMEM;
	}

	issi_mid9d_nand->id_manufactory = 0x9D;
	issi_mid9d_nand->id_device_list = device_id;
	issi_mid9d_nand->id_device_count = ARRAY_SIZE(issi_param);

	issi_mid9d_nand->ops.get_cdt_params = issi_get_cdt_params;
	issi_mid9d_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	issi_mid9d_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(issi_mid9d_nand);
}

SPINAND_MOUDLE_INIT(issi_mid9d_nand_init);
