#include <config.h>
#include <common.h>
//#include <ddr/ddr_common.h>

#define PI_MAGIC_GINFO   (('B' << 24) | ('D' << 16) | ('I' << 8) | ('F' << 0))
#define PI_MAGIC_DDR     (('D' << 24) | ('D' << 16) | ('R' << 8) | 0)
#define PI_MAGIC_DETECT  (('D' << 24) | ('E' << 16) | ('T' << 8) | ('E' << 0))

DECLARE_GLOBAL_DATA_PTR;
struct ddr_registers *g_ddr_param = 0;
int soc_detected = 0;

struct param_info
{
	unsigned int magic_id;
	unsigned int size;
	unsigned int data;
};

void burner_param_info(void)
{
	const struct param_info *pi = (struct param_info *)CONFIG_SPL_GINFO_BASE;

	while (pi->magic_id != 0) {
		switch (pi->magic_id) {
			case PI_MAGIC_GINFO:
				gd->arch.gi = (struct global_info *)&pi->data;
				break;
			case PI_MAGIC_DDR:
				g_ddr_param = (struct ddr_registers *)&pi->data;
				break;
			case PI_MAGIC_DETECT:
				soc_detected = !!pi->data;
				if (soc_detected)
					return;
				break;
			default:
				return;
		}
		pi = (const struct param_info *)((char *)pi + pi->size + 8);
	}
}
