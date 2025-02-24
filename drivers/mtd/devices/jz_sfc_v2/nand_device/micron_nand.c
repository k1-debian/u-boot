#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		5
#define	TSHSL_R		50
#define	TSHSL_W		50

#define TRD		80
#define TPP		600
#define TBE		10

static struct jz_sfcnand_device *micron_nand;

static struct jz_sfcnand_base_param micron_param[] = {

	[0] = {
		/* MT29F1G01ABBFDSF-1.8V */
		.pagesize = 2048,
		.blocksize = 2048 * 64,
		.oobsize = 128,
		.flashsize = 2048 * 64 * 1024,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.plane_select = 1,
		.ecc_max = 0x8,
		.need_quad = 1,
	},

};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x15, "MT29F1G01ABBFDSF-1.8V", &micron_param[0]),
};


static cdt_params_t *micron_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(micron_nand->cdt_params);

	switch(device_id) {
		case 0x15:
			break;
		default:
			pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}

	return &micron_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{

	switch(device_id) {
		case 0x15:
			switch((ecc_status >> 4) & 0x7) {
				case 0x0:
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


static int micron_nand_init(void) {

	micron_nand = kzalloc(sizeof(*micron_nand), GFP_KERNEL);
	if(!micron_nand) {
		pr_err("alloc micron_nand struct fail\n");
		return -ENOMEM;
	}

	micron_nand->id_manufactory = 0x2C;
	micron_nand->id_device_list = device_id;
	micron_nand->id_device_count = ARRAY_SIZE(micron_param);

	micron_nand->ops.get_cdt_params = micron_get_cdt_params;
	micron_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	micron_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(micron_nand);
}

SPINAND_MOUDLE_INIT(micron_nand_init);
