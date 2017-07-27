#include <common.h>
#include <asm/arch-x1000/spi.h>
#include <nand.h>
#include "../../../spi/jz_spi.h"
#include <linux/mtd/mtd.h>
#include <ingenic_nand_mgr/nand_param.h>
#include "../../../mtd/nand/jz_spinand.h"

extern struct jz_spinand_partition *get_partion_index(u32 startaddr,u32 length,int *pt_index);
extern struct nand_param_from_burner nand_param_from_burner;
/*******************************************************************************
 * in burner init,we find spinand information from stage2_arg
 * and change it to struct nand_param_from_burner which uboot can use
 *for chip probe,but after chip probe the struct nand_param_from_burner
 *is changed,and para_num is changed to 1,and jz_spi_support_from_burner
 *pointer addr changed to the address which param we probe.
 * ******************************************************************************/
void get_burner_nandinfo(char *flash_info,struct nand_param_from_burner *param)
{
	char *member_addr=flash_info;
	param->version=*(int *)member_addr;
	member_addr+=sizeof(param->version);
	param->flash_type=*(int *)member_addr;
	member_addr+=sizeof(param->flash_type);
        param->para_num=*(int *)member_addr;
	member_addr+=sizeof(param->para_num);
        param->addr=member_addr;
	member_addr+=param->para_num*sizeof(struct jz_spi_support_from_burner);
        param->partition_num=*(int *)member_addr;
	member_addr+=sizeof(param->partition_num);
        param->partition=member_addr;
}

extern nand_info_t nand_info[CONFIG_SYS_MAX_NAND_DEVICE];
static unsigned int bad_len = 0;



static int sfc_nand_skip_bad(unsigned int addr)
{
	nand_info_t *nand;
	nand = &nand_info[0];
	unsigned int offset;
	unsigned int block_size = nand->erasesize;

	offset = addr + bad_len;
	while (nand_block_isbad(nand, offset)) {
		printf("Skip bad block 0x%lx\n", offset);
		bad_len += block_size;
		offset += block_size;
	}
	return offset;
}


int spinand_program(struct cloner *cloner)
{
	u32 length = cloner->cmd->write.length;
	u32 full_size = cloner->full_size;
	void *databuf = (void *)cloner->write_req->buf;
	u32 startaddr = cloner->cmd->write.partition + (cloner->cmd->write.offset);
	char command[128];
	volatile int pt_index = -1;
	struct jz_spinand_partition *partition;
	int ret;

	static int pt_index_bak = -1;
	static char *part_name = NULL;
	nand_info_t *nand;
	nand = &nand_info[0];
	unsigned int block_size = nand->erasesize;

	partition = get_partion_index(startaddr,length,&pt_index);
	if (pt_index < 0)
		return -EIO;
	if (startaddr==0 && spi_args->download_params != 0) {
		add_information_to_spl(databuf);
	}

	if ((partition->manager_mode == MTD_MODE) || (partition->manager_mode == MTD_D_MODE)) {
		if (!spi_args->spi_erase) {
			if (partition->manager_mode == MTD_D_MODE)
				pt_index = startaddr / block_size;
			if (pt_index != pt_index_bak) {
				pt_index_bak = pt_index;
				memset(command, 0 , 128);
				if (partition->manager_mode == MTD_D_MODE)
					sprintf(command, "nand erase 0x%x 0x%x", startaddr, ALIGN(length, block_size));
				else
					sprintf(command, "nand erase 0x%x 0x%x", partition->offset, partition->size);
				BURNNER_PRI("%s\n", command);
				ret = run_command(command, 0);
				if (ret)
					goto out;
			}
		}

		if ((startaddr + length) <= (partition->size + partition->offset)) {
			startaddr = sfc_nand_skip_bad(startaddr);
			ret = nand_write(nand, startaddr, &length, databuf);
			BURNNER_PRI("nand write to offset 0x%lx, length = 0x%lx : %s\n",
					startaddr, length, ret ? "ERROR" : "OK");
		} else {
			BURNNER_PRI("ERROR : out of partition !!!\n");
		}

		if (debug_args->write_back_chk) {
			if (!readbuf) {
				readbuf = malloc(READBUF_SIZE);
				memset(readbuf,0,READBUF_SIZE);
			}
			memset(command, 0 , 128);
			sprintf(command,"nand read.jffs2 0x%x 0x%x 0x%x",readbuf,startaddr, length);
			run_command(command,0);
			ret = buf_compare(cloner->write_req->buf,readbuf,length,startaddr);
			if (ret) {
				return -1;
			}
		}

	} else if (partition->manager_mode == UBI_MANAGER) {
		if (startaddr == partition->offset) {
			if (!spi_args->spi_erase) {
				if (pt_index != pt_index_bak) {
					pt_index_bak = pt_index;
					memset(command, 0 , 128);
					sprintf(command, "nand erase 0x%x 0x%x", partition->offset, partition->size);
					BURNNER_PRI("%s\n", command);
					ret = run_command(command, 0);
					if (ret)
						goto out;
				}
			}

			memset(command, 0, 128);
			sprintf(command, "ubi part %s", partition->name);
			BURNNER_PRI("%s\n", command);
			ret = run_command(command, 0);
			if (ret) {
				BURNNER_PRI("ubi part error...\n");
				return ret;
			}

			memset(command, 0, X_COMMAND_LENGTH);
			sprintf(command, "ubi create %s",partition->name);
			BURNNER_PRI("%s\n", command);
			ret = run_command(command, 0);
			if (ret) {
				BURNNER_PRI("ubi create error...\n");
				return ret;
			}
		}

		memset(command, 0, 128);
		static wlen = 0;
		wlen += length;
		if (full_size && (full_size <= length)) {
			length = full_size;
			sprintf(command, "ubi write 0x%x %s 0x%x", (unsigned)databuf, partition->name, length);
		} else if (full_size) {
			sprintf(command, "ubi write.part 0x%x %s 0x%x 0x%x",(unsigned)databuf, partition->name, length, full_size);
		} else {
			sprintf(command, "ubi write.part 0x%x %s 0x%x",(unsigned)databuf, partition->name, length);
		}


		ret = run_command(command, 0);
		if (ret) {
			BURNNER_PRI("...error\n");
			return ret;
		}
	}
	if (cloner->full_size)
		cloner->full_size = 0;
	return 0;
out:
	BURNNER_PRI("...error\n");
	return ret;

}
/****************************************************************************************
 * copy spinand information from burner to u-boot-with-spl.bin
 * char *databuf:u-boot-with-spl.bin date pointer
 * in function :
 * param is global variable of struct nand_param_from_burner this struct is information in spinand
 * **************************************************************************************/
void add_information_to_spl(char *databuf)
{
	int page_spl=0;
	int32_t nand_magic=0x6e616e64;
	char *member_addr=databuf;
	page_spl=((nand_param_from_burner.addr->page_num/32)<<16)|((nand_param_from_burner.addr->page_size/1024)<<24);//compatible
	*((int *)(databuf+8))=( *((int *)(databuf+8)))|page_spl;	//write pagesize to spl head
	member_addr+=CONFIG_SPIFLASH_PART_OFFSET;			//spinand parameter number addr
	memcpy((char *)member_addr,&nand_magic,sizeof(int32_t));
	member_addr+=sizeof(int32_t);					//spinand parameter magic  addr
	memcpy((char *)member_addr,&nand_param_from_burner.version,sizeof(nand_param_from_burner.version));
	member_addr+=sizeof(nand_param_from_burner.version);		//spinand parameter number addr
	memcpy((char *)member_addr,&nand_param_from_burner.flash_type,sizeof(nand_param_from_burner.flash_type));
	member_addr+=sizeof(nand_param_from_burner.flash_type);
	memcpy((char *)member_addr,&nand_param_from_burner.para_num,sizeof(nand_param_from_burner.para_num));
	member_addr+=sizeof(nand_param_from_burner.para_num);		//spinand parameter addr
	memcpy((char *)member_addr,nand_param_from_burner.addr,nand_param_from_burner.para_num*sizeof(struct jz_spi_support_from_burner));

	member_addr+=nand_param_from_burner.para_num*sizeof(struct jz_spi_support_from_burner);//spinand partition number addr
	memcpy(member_addr,&nand_param_from_burner.partition_num,sizeof(nand_param_from_burner.partition_num));
	member_addr+=sizeof(nand_param_from_burner.partition_num);		//partition addr
	memcpy(member_addr,nand_param_from_burner.partition,nand_param_from_burner.partition_num*sizeof(struct jz_spinand_partition));	//partition
}
