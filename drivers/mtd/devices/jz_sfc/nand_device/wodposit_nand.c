#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include <asm/arch/spinand.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP		5
#define THOLD		5
#define TSHSL_R		20
#define TSHSL_W		20

#define TRD		450
#define TPP		800
#define TBE		10


static struct jz_sfcnand_base_param wodposit_param[] = {

	[0] = {
		/* WPS3NS01W */
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

		.ecc_max = 0x8,
		.need_quad = 1,
	},
	[1] = {
		/* WPS3NS02W */
		.pagesize = 2048,
		.blocksize = 2048 * 64,
		.oobsize = 64,
		.flashsize = 2048 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
	[2] = {
		/* WPS3NS04W */
		.pagesize = 4096,
		.blocksize = 4096 * 64,
		.oobsize = 128,
		.flashsize = 4096 * 64 * 2048,

		.tSETUP  = TSETUP,
		.tHOLD   = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 0x8,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0xA081, "WPS3NS01W", &wodposit_param[0]),
	DEVICE_ID_STRUCT(0xA082, "WPS3NS02W", &wodposit_param[1]),
	DEVICE_ID_STRUCT(0xA083, "WPS3NS04W", &wodposit_param[2]),
};

static int32_t wodposit_get_read_feature(struct flash_operation_message *op_info)
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
		case 0xA081:
		case 0xA082:
		case 0xA083:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
					return 0;
				case 0x1:
					return 4;
				case 0x2:
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


static int wodposit_nand_init(void) {
	struct jz_sfcnand_device *wodposit_nand;
	wodposit_nand = kzalloc(sizeof(*wodposit_nand), GFP_KERNEL);
	if(!wodposit_nand) {
		pr_err("alloc wodposit_nand struct fail\n");
		return -ENOMEM;
	}

	wodposit_nand->id_manufactory = 0xA5;
	wodposit_nand->id_device_list = device_id;
	wodposit_nand->id_device_count = ARRAY_SIZE(wodposit_param);

	wodposit_nand->ops.nand_read_ops.get_feature = wodposit_get_read_feature;

	return jz_sfcnand_register(wodposit_nand);
}

SPINAND_MOUDLE_INIT(wodposit_nand_init);
