#ifndef __X2600_BOOTROOM_HELP_H__
#define __X2600_BOOTROOM_HELP_H__

void x2600_bootroom_help_stage1_entry(void) __attribute__((noreturn));
void x2600_bootroom_help_run(void) __attribute__((noreturn));

int spl_jzsdhci_bootroom_init(void);
unsigned int mmc_block_read(unsigned int start, unsigned int blkcnt,
			    unsigned int *dst);
int x2600_bootroom_mmc_init(void);
unsigned int x2600_bootroom_mmc_block_read(unsigned int start,
					   unsigned int blkcnt,
					   unsigned int *dst);

#endif
