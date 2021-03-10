#ifndef __DDR_COMMON_H__
#define __DDR_COMMON_H__

#include <ddr/ddr_params.h>

#ifdef CONFIG_CPU_XBURST
#include <ddr/ddr_chips.h>
#include <asm/ddr_dwc.h>
#include <ddr/ddrc.h>
#ifdef CONFIG_X1XXX_INNOPHY
#include <ddr/ddrp_inno.h>
#else
#include <ddr/ddrp_dwc.h>
#endif
#endif


#if defined(CONFIG_X2000) || defined(CONFIG_X2000_V12) || defined(CONFIG_M300) || defined(CONFIG_X2100) || defined(CONFIG_X2500)
#include <ddr/ddr_chips_v2.h>
#include <asm/ddr_innophy.h>
#include <ddr/ddrp_inno.h>
#include <ddr/ddrc_x2000.h>
#endif


#endif /* __DDR_COMMON_H__ */
