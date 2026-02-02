#include <errno.h>
#include <malloc.h>
#include <linux/mtd/partitions.h>
#include <asm/arch/spinand.h>
#include "../jz_sfc_common.h"
#include "nand_common.h"

#define TSETUP                  5
#define THOLD                   5
#define TSHSL_R                 30
#define TSHSL_W                 30

#define TRD                     45
#define TPP                     350
#define TBE                     4


static struct jz_sfcnand_base_param etron_param[] = {

	[0] = {
		/*EM73C044VCG-H*/
		.pagesize  = 2048,
		.oobsize   = 64,
		.blocksize = 2048 * 64,
		.flashsize = 2048 * 64 * 1024,

		.tSETUP = TSETUP,
		.tHOLD  = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 4,
		.need_quad = 1,
	},
	[1] = {
		/*EM73C044VCS-H*/
		.pagesize  = 2048,
		.oobsize   = 128,
		.blocksize = 2048 * 64,
		.flashsize = 2048 * 64 * 2048,

		.tSETUP = TSETUP,
		.tHOLD  = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 4,
		.need_quad = 1,
	},
	[2] = {
		/*HYF4GQ4UTACAE*/
		.pagesize  = 2048,
		.oobsize   = 128,
		.blocksize = 2048 * 64,
		.flashsize = 2048 * 64 * 4096,

		.tSETUP = TSETUP,
		.tHOLD  = THOLD,
		.tSHSL_R = TSHSL_R,
		.tSHSL_W = TSHSL_W,

		.tRD = TRD,
		.tPP = TPP,
		.tBE = TBE,

		.ecc_max = 4,
		.need_quad = 1,
	},
};

static struct device_id_struct device_id[] = {
	DEVICE_ID_STRUCT(0x15, "EM73C044VCG-H", &etron_param[0]),
	DEVICE_ID_STRUCT(0x25, "EM73C044VCS-H", &etron_param[1]),
	DEVICE_ID_STRUCT(0x35, "HYF4GQ4UTACAE", &etron_param[2]),
};


static int32_t etron_get_read_feature(struct flash_operation_message *op_info)
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
		case 0x15:
		case 0x25:
		case 0x35:
			switch((ecc_status >> 4) & 0x3) {
				case 0x0:
				case 0x1:
					return 0;
				case 0x2:
					return 4;
				case 0x3:
					return -EBADMSG;
				default:
					break;
			}
		default:
			printf("device_id err, it maybe don`t support this device, check your device id: device_id = 0x%02x\n", device_id);
			break;
	}
	return -EINVAL;
}

#if 0
static int etron_set_featrue(struct sfc_flash *flash, uint8_t flag)
{
	struct jz_sfcnand_flashinfo *nand_info = flash->flash_info;
	uint16_t device_id = nand_info->id_device;
	int ret;

	switch(device_id) {
		case 0x15:
		case 0x25:
		case 0x35:
			if((ret = sfc_nand_set_feature(flash, SPINAND_ADDR_PROTECT, 2)))
				return -EIO;
			break;
		default:
			printf("device_id err, please check your  device id: device_id = 0x%02x\n", device_id);
			return -EIO;
	}

	return 0;
}
#endif

static int etron_nand_init(void) {
	struct jz_sfcnand_device *etron_nand;
	etron_nand = kzalloc(sizeof(*etron_nand), GFP_KERNEL);
	if(!etron_nand) {
		pr_err("alloc etron_nand struct fail\n");
		return -ENOMEM;
	}

	etron_nand->id_manufactory = 0x01;
	etron_nand->id_device_list = device_id;
	etron_nand->id_device_count = ARRAY_SIZE(etron_param);

	etron_nand->ops.nand_read_ops.get_feature = etron_get_read_feature;
//	etron_nand->ops.set_feature = etron_set_featrue;

	return jz_sfcnand_register(etron_nand);
}

SPINAND_MOUDLE_INIT(etron_nand_init);
