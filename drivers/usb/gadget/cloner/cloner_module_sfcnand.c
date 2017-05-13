#include <cloner/cloner.h>
#include "burn_printf.h"
#include "cloner_spinand.c"

struct spi_param *spi_args;


int clmd_sfcnand_init(struct cloner *cloner, void *args, void *ops_data)
{
	spi_args = (struct spi_param *)args;
	if(!spi_args)
	{
		printf("Not found sfcnand parameters\n");
		return -EINVAL;
	}
	get_burner_nandinfo(spi_args->flash_info,&nand_param_from_burner);
	mtd_sfcnand_probe_burner(&(spi_args->spi_erase),spi_args->sfc_quad_mode,&nand_param_from_burner);
	return ret;
}


int clmd_sfcnand_write(struct cloner *cloner, int sub_type, void *ops_data)
{
	return spinand_program(cloner);
}

int cloner_sfcnand_init(void)
{
	struct cloner_moudle *clmd = malloc(sizeof(struct cloner_moudle));
	int ret;

	if (!clmd)
		return -ENOMEM;
	clmd->medium = MAGIC_SFCNAND;
	clmd->ops = SFC_NAND;
	clmd->write = clmd_sfcnand_write;;
	clmd->init = clmd_sfcnand_init;
	clmd->info = NULL;
	clmd->read = NULL;
	clmd->check = NULL;
	clmd->data = NULL;
	printf("cloner sfcnand register\n");
	return register_cloner_moudle(clmd);
}
CLONER_MOUDLE_INIT(cloner_sfcnand_init);
