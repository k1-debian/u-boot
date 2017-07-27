#include <cloner/cloner.h>
#include "burn_printf.h"
#include "cloner_sfcnor.c"
#include "cloner_spinand.c"

struct spi_param *spi_args;

int buf_compare(unsigned char *org_data,unsigned char *read_data,unsigned int len,unsigned int offset)
{
	unsigned int i,val = 0;
	unsigned int *buf1 = (unsigned int *)org_data;
	unsigned int *buf2 = (unsigned int *)read_data;
	for(i = 0; i < len / 4; i++)
	{
		if(buf1[i] != buf2[i]){
			printf("XXXXXXXXXX  compare error: org_data[%d] = 0x%08x read_data[%d] = 0x%08x addr= 0x%08x  len = %d\n",
					i, buf1[i], i, buf2[i], offset + i * 4, len);
			val = -1;
		}
	}
	return val;
}


int clmd_spisfc_info(struct cloner *cloner)
{
	unsigned int id_code = 0;
#ifdef CONFIG_JZ_SFC_NOR
	id_code = get_norflash_id();
	printf("id_code=%x\n", id_code);
#endif
	memcpy(cloner->ep0req->buf, &id_code, sizeof(unsigned int));
	return (int)id_code;
}


int clmd_spisfc_init(struct cloner *cloner, void *args, void *ops_data)
{
	spi_args = (struct spi_param *)args;
	if(!spi_args)
	{
		printf("Not found sfc parameters (%s)\n",__func__);
		return -EINVAL;
	}
	int ret = 0;

	if(!policy_args)
	{
		printf("Not fount policy parameters (%s)\n",__func__);
		return -EINVAL;
	}
#ifdef CONFIG_JZ_SFC_NOR
	if(policy_args->use_sfc_nor){
		norflash_get_params_from_burner((unsigned char *)spi_args + sizeof(struct spi_param));
		if (spi_args->spi_erase == SPI_ERASE_PART) {
			sfc_erase();
		}
	}
#endif
#if CONFIG_MTD_SFCNAND
	if(policy_args->use_sfc_nand){
		get_burner_nandinfo(spi_args->flash_info,&nand_param_from_burner);
		mtd_sfcnand_probe_burner(&(spi_args->spi_erase),spi_args->sfc_quad_mode,&nand_param_from_burner);
	}
#endif

	return ret;
}

int clmd_spisfc_write(struct cloner *cloner, int sub_type, void *ops_data)
{
	switch(sub_type)
	{
#ifdef CONFIG_JZ_SFC_NOR
		case SFC_NOR:
			cloner->ack = sfc_program(cloner);
			break;
#endif
#if CONFIG_MTD_SFCNAND
		case SFC_NAND:
			cloner->ack = spinand_program(cloner);
			break;
#endif
		default:
			printf("Not found sfc sub_type!\n");
			return -EINVAL;
	}
	return 0;
}

int cloner_spisfc_init(void)
{
	struct cloner_moudle *clmd = malloc(sizeof(struct cloner_moudle));
	int ret;

	if (!clmd)
		return -ENOMEM;
	clmd->medium = MAGIC_SFC;
	clmd->ops = SPISFC;
	clmd->write = clmd_spisfc_write;;
	clmd->init = clmd_spisfc_init;
	clmd->info = clmd_spisfc_info;
	clmd->read = NULL;
	clmd->check = NULL;
	clmd->data = NULL;
	printf("cloner spisfc register\n");
	return register_cloner_moudle(clmd);
}
CLONER_MOUDLE_INIT(cloner_spisfc_init);
