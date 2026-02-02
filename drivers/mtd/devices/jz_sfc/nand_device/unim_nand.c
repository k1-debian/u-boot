#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include <asm/arch/spinand.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define THOLD	    5
#define TSETUP	    5
#define TSHSL_R	    30
#define TSHSL_W	    30

#define TRD	    95
#define TPP	    600
#define TBE	    10


static struct jz_sfcnand_base_param unim_param[] = {
	[0] = {
		/*UM19A9HISW*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 512,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
	[1] = {
		/*UM19A8HISW*/
		.pagesize = 2 * 1024,
		.blocksize = 2 * 1024 * 64,
		.oobsize = 64,
		.flashsize = 2 * 1024 * 64 * 256,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x4,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x0C, "UM19A9HISW", &unim_param[0]),
	DEVICE_ID_STRUCT(0xFC, "UM19A8HISW", &unim_param[1]),

};

static int32_t unim_get_read_feature(struct flash_operation_message *op_info)
{

	struct sfc_flash *flash = op_info->flash;
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	struct sfc_transfer transfer;
	uint16_t device_id = nand_info->id_device;
	uint8_t ecc_status = 0;
	int32_t ret = 0;

retry:
	ecc_status = 0;
	memset(&transfer, 0, sizeof(transfer));
	sfc_list_init(&transfer);

	transfer.cmd_info.cmd = SPINAND_CMD_GET_FEATURE;
	transfer.sfc_mode = TM_STD_SPI;

	transfer.addr = SPINAND_ADDR_STATUS;
	transfer.addr_len = 1;

	transfer.cmd_info.dataen = ENABLE;
	transfer.data = &ecc_status;
	transfer.len = 1;
	transfer.direction = GLB_TRAN_DIR_READ;

	transfer.data_dummy_bits = 0;
	transfer.ops_mode = CPU_OPS;

	if(sfc_sync(flash->sfc, &transfer)) {
	        printf("sfc_sync error ! %s %s %d\n",__FILE__,__func__,__LINE__);
		return -EIO;
	}

	if(ecc_status & SPINAND_IS_BUSY)
		goto retry;

	switch(device_id) {
		case 0x0C:
		case 0xFC:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
					return 0;
				case 0x1:
					return 2;
				case 0x2:
					return -EBADMSG;
				case 0x3:
					return 4;
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

static int unim_nand_init(void) {
	struct jz_sfcnand_device *unim_nand;
	unim_nand = kzalloc(sizeof(*unim_nand), GFP_KERNEL);
	if(!unim_nand) {
		pr_err("alloc unim_nand struct fail\n");
		return -ENOMEM;
	}

	unim_nand->id_manufactory = 0xB0;
	unim_nand->id_device_list = device_id;
	unim_nand->id_device_count = ARRAY_SIZE(unim_param);

	unim_nand->ops.nand_read_ops.get_feature = unim_get_read_feature;

	return jz_sfcnand_register(unim_nand);
}

SPINAND_MOUDLE_INIT(unim_nand_init);
