#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP		20
#define THOLD		20
#define	TSHSL_R		100
#define	TSHSL_W		100

#define TRD		120
#define TPP		750
#define TBE		10

static struct jz_sfcnand_device *puya_nand;

static struct jz_sfcnand_base_param puya_param[] = {

	[0] = {
		/*P25N01GHA*/
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
		.ecc_max = 13,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x2A, "P25N01GHA", &puya_param[0]),
};


static cdt_params_t *puya_get_cdt_params(struct sfc_flash *flash, uint16_t device_id)
{
	CDT_PARAMS_INIT(puya_nand->cdt_params);

	switch(device_id) {
		case 0x2A:
			break;
		default:
			pr_err("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return NULL;
	}

	return &puya_nand->cdt_params;
}

static inline int deal_ecc_status(struct sfc_flash *flash, uint16_t device_id, uint8_t ecc_status)
{
	int ret = 0;
	switch(device_id) {
		case 0x2A:
			switch((ecc_status >> 4) & 0x7) {
				case 0x0:
					return 0;
				case 0x1:
					return 4;
				case 0x2:
					return -EBADMSG;
				case 0x3:
					return 8;
				case 0x5:
					return 13;
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


static int puya_nand_init(void)
{
	puya_nand = kzalloc(sizeof(*puya_nand), GFP_KERNEL);
	if(!puya_nand) {
		pr_err("alloc puya_nand struct fail\n");
		return -ENOMEM;
	}

	puya_nand->id_manufactory = 0x85;
	puya_nand->id_device_list = device_id;
	puya_nand->id_device_count = ARRAY_SIZE(puya_param);

	puya_nand->ops.get_cdt_params = puya_get_cdt_params;
	puya_nand->ops.deal_ecc_status = deal_ecc_status;

	/* use private get feature interface, please define it in this document */
	puya_nand->ops.get_feature = NULL;

	return jz_sfcnand_register(puya_nand);
}

SPINAND_MOUDLE_INIT(puya_nand_init);
