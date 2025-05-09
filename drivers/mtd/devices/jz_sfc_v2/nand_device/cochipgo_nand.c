#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		5
#define TSHSL_R		20
#define TSHSL_W		20

#define TRD		120
#define TPP		600
#define TBE		10

static struct jz_sfcnand_device *cochipgo_nand;

static struct jz_sfcnand_base_param cochipgo_param[] = {

	[0] = {
		/* C5F1GM7UExxG */
		.pagesize = 2048,
		.blocksize = 2048 * 64,
		.oobsize = 64,
		.flashsize = 2048 * 64 * 1024,

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
	DEVICE_ID_STRUCT(0x91, "C5F1GM7UExxG", &cochipgo_param[0]),
	DEVICE_ID_STRUCT(0x81, "C5F1GM7RExxG", &cochipgo_param[0]),
};


static cdt_params_t *cochipgo_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(cochipgo_nand->cdt_params);

	switch(device_id) {
		case 0x91:
		case 0x81:
			break;
		default:
			pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}

	return &cochipgo_nand->cdt_params;
}


static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{
        int ret = 0;

	switch(device_id) {
		case 0x91:
		case 0x81:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
					return 0;
				case 0x1:
					ret = nand_get_ecc_conf(flash, 0xf0);
					if (ret < 0)
						return ret;
					return ((ret >> 4) & 0x3) + 4;
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


static int cochipgo_nand_init(void) {

	cochipgo_nand = kzalloc(sizeof(*cochipgo_nand), GFP_KERNEL);
	if(!cochipgo_nand) {
		pr_err("alloc cochipgo_nand struct fail\n");
		return -ENOMEM;
	}

	cochipgo_nand->id_manufactory = 0xD8;
	cochipgo_nand->id_device_list = device_id;
	cochipgo_nand->id_device_count = ARRAY_SIZE(cochipgo_param);

	cochipgo_nand->ops.get_cdt_params = cochipgo_get_cdt_params;
	cochipgo_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	cochipgo_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(cochipgo_nand);
}

SPINAND_MOUDLE_INIT(cochipgo_nand_init);
