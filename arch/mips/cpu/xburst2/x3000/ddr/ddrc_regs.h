/*
 * ddrc_regs.h - X3000 DDRC 寄存器定义
 *
 * 用法 :
 *   uint32_t v = ddrc_readl(DDRC_MSTR);
 *   ddrc_writel(DDRC_MSTR, v);
 *   uint32_t b = ddrc_read_bits(DDRC_MSTR, DDRC_MSTR_DEVICE_CONFIG);
 *   ddrc_write_bits(DDRC_MSTR, DDRC_MSTR_DEVICE_CONFIG, 0x1);
 *
 * 位域宏展开为 (hi, lo) 两个实参，直接传给访问函数。
 * 变量位域（x:N）以注释标出，需按实际配置实例化。
 */
#ifndef __DDRC_REGS_H__
#define __DDRC_REGS_H__

#ifndef DDRC_BASE
#define DDRC_BASE 0xb3012000
#endif

/* ============ 寄存器访问（offset 形式，DDRC_BASE + reg） ============ */
static inline uint32_t ddrc_readl(uint32_t reg)
{
	return *(volatile uint32_t *)(DDRC_BASE + reg);
}

static inline void ddrc_writel(uint32_t reg, uint32_t value)
{
	*(volatile uint32_t *)(DDRC_BASE + reg) = value;
}

static inline uint32_t ddrc_read_bits(uint32_t reg, uint8_t hi, uint8_t lo)
{
	uint32_t mask = ((hi - lo) >= 31) ? 0xffffffffu : ((1u << (hi - lo + 1)) - 1u);
	return (ddrc_readl(reg) & (mask << lo)) >> lo;
}

static inline void ddrc_write_bits(uint32_t reg, uint8_t hi, uint8_t lo, uint32_t value)
{
	uint32_t mask = ((hi - lo) >= 31) ? 0xffffffffu : ((1u << (hi - lo + 1)) - 1u);
	uint32_t r = ddrc_readl(reg) & ~(mask << lo);
	ddrc_writel(reg, r | ((value & mask) << lo));
}

/* ============ 寄存器与位域定义 ============ */

/* ----- 0x0000  MSTR ----- */
/* Master Register 0 */
#define DDRC_MSTR                       0x0000
/* [31:30] rw=R/W reset=0x0 */
#define DDRC_MSTR_DEVICE_CONFIG                    31, 30
/* [29:29] rw=R/W reset=0x0 */
#define DDRC_MSTR_FREQUENCY_MODE                   29, 29
/* 变量位域(按配置实例化): active_ranks [x:24] rw=R/W reset=0x3 */
/* [23:23] rw=- reset=0x0 */
#define DDRC_MSTR_RESERVED_1                       23, 23
/* [22:22] rw=R/W reset=0x0 */
#define DDRC_MSTR_FREQUENCY_RATIO                  22, 22
/* [21:20] rw=R/W reset=0x0 */
#define DDRC_MSTR_ACTIVE_LOGICAL_RANKS             21, 20
/* [19:16] rw=R/W reset=0x4 */
#define DDRC_MSTR_BURST_RDWR                       19, 16
/* [15:15] rw=R/W reset=0x0 */
#define DDRC_MSTR_DLL_OFF_MODE                     15, 15
/* [14:14] rw=- reset=0x0 */
#define DDRC_MSTR_RESERVED_2                       14, 14
/* [13:12] rw=R/W reset=0x0 */
#define DDRC_MSTR_DATA_BUS_WIDTH                   13, 12
/* [11:11] rw=R/W reset=0x0 */
#define DDRC_MSTR_GEARDOWN_MODE                    11, 11
/* [10:10] rw=R/W reset=0x0 */
#define DDRC_MSTR_EN_2T_TIMING_MODE                10, 10
/* [9:9] rw=R/W reset=0x0 */
#define DDRC_MSTR_BURSTCHOP                         9,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_MSTR_BURST_MODE                        8,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_MSTR_RESERVED_3                        7,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_MSTR_LPDDR4                            5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_MSTR_DDR4                              4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_MSTR_LPDDR3                            3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_MSTR_LPDDR2                            2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_MSTR_MOBILE                            1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_MSTR_DDR3                              0,  0

/* ----- 0x0004  STAT ----- */
/* Operating Mode Status Register */
#define DDRC_STAT                       0x0004
/* [31:13] rw=- reset=0x0 */
#define DDRC_STAT_RESERVED_1                       31, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_STAT_SELFREF_CAM_NOT_EMPTY            12, 12
/* [11:10] rw=- reset=0x0 */
#define DDRC_STAT_RESERVED_2                       11, 10
/* [9:8] rw=R reset=0x0 */
#define DDRC_STAT_SELFREF_STATE                     9,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_STAT_RESERVED_3                        7,  6
/* [5:4] rw=R reset=0x0 */
#define DDRC_STAT_SELFREF_TYPE                      5,  4
/* [2:0] rw=R reset=0x0 */
#define DDRC_STAT_OPERATING_MODE		    2,  0

/* ----- 0x0008  MSTR1 ----- */
/* Master Register 1 */
#define DDRC_MSTR1                      0x0008
/* [31:17] rw=- reset=0x0 */
#define DDRC_MSTR1_RESERVED_1                      31, 17
/* [16:16] rw=R/W reset=0x0 */
#define DDRC_MSTR1_ALT_ADDRMAP_EN                  16, 16
/* 变量位域(按配置实例化): rfc_tmgreg_sel [x:8] rw=R/W reset=0x0 */
/* 变量位域(按配置实例化): rank_tmgreg_sel [x:0] rw=R/W reset=0x0 */

/* ----- 0x0010  MRCTRL0 ----- */
/* Mode Register Read/Write Control Register 0. */
#define DDRC_MRCTRL0                    0x0010
/* [31:31] rw=R/W1S reset=0x0 */
#define DDRC_MRCTRL0_MR_WR                         31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_MRCTRL0_PBA_MODE                      30, 30
/* 变量位域(按配置实例化): mr_cid [x:16] rw=R/W reset=0x0 */
/* [15:12] rw=R/W reset=0x0 */
#define DDRC_MRCTRL0_MR_ADDR                       15, 12
/* 变量位域(按配置实例化): mr_rank [x:4] rw=R/W reset=0x3 */
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_MRCTRL0_SW_INIT_INT                    3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_MRCTRL0_PDA_EN                         2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_MRCTRL0_MPR_EN                         1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_MRCTRL0_MR_TYPE                        0,  0

/* ----- 0x0014  MRCTRL1 ----- */
/* Mode Register Read/Write Control Register 1 */
#define DDRC_MRCTRL1                    0x0014
/* 变量位域(按配置实例化): mr_data [x:0] rw=R/W reset=0x0 */

/* ----- 0x0018  MRSTAT ----- */
/* Mode Register Read/Write Status Register */
#define DDRC_MRSTAT                     0x0018
/* [31:9] rw=- reset=0x0 */
#define DDRC_MRSTAT_RESERVED_1                     31,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_MRSTAT_PDA_DONE                        8,  8
/* [7:1] rw=- reset=0x0 */
#define DDRC_MRSTAT_RESERVED_2                      7,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_MRSTAT_MR_WR_BUSY                      0,  0

/* ----- 0x001c  MRCTRL2 ----- */
/* Mode Register Read/Write Control Register 2 */
#define DDRC_MRCTRL2                    0x001c
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_MRCTRL2_MR_DEVICE_SEL                 31,  0

/* ----- 0x0020  DERATEEN ----- */
/* Temperature Derate Enable Register */
#define DDRC_DERATEEN                   0x0020
/* [31:14] rw=- reset=0x0 */
#define DDRC_DERATEEN_RESERVED_1                   31, 14
/* [13:13] rw=R/W reset=0x0 */
#define DDRC_DERATEEN_DERATE_MR4_PAUSE_FC          13, 13
/* [12:12] rw=R/W reset=0x0 */
#define DDRC_DERATEEN_DERATE_MR4_TUF_DIS           12, 12
/* [11:11] rw=- reset=0x0 */
#define DDRC_DERATEEN_RESERVED_2                   11, 11
/* [10:8] rw=R/W reset=0x0 */
#define DDRC_DERATEEN_RC_DERATE_VALUE              10,  8
/* [7:4] rw=R/W reset=0x0 */
#define DDRC_DERATEEN_DERATE_BYTE                   7,  4
/* [3:3] rw=- reset=0x0 */
#define DDRC_DERATEEN_RESERVED_3                    3,  3
/* [2:1] rw=R/W reset=0x0 */
#define DDRC_DERATEEN_DERATE_VALUE                  2,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DERATEEN_DERATE_ENABLE                 0,  0

/* ----- 0x0024  DERATEINT ----- */
/* Temperature Derate Interval Register */
#define DDRC_DERATEINT                  0x0024
/* [31:0] rw=R/W reset=0x800000 */
#define DDRC_DERATEINT_MR4_READ_INTERVAL           31,  0

/* ----- 0x0028  MSTR2 ----- */
/* Master Register 2 */
#define DDRC_MSTR2                      0x0028
/* [31:2] rw=- reset=0x0 */
#define DDRC_MSTR2_RESERVED_1                      31,  2
/* [1:0] rw=R/W reset=0x1 */
#define DDRC_MSTR2_TARGET_FREQUENCY                 1,  0

/* ----- 0x002c  DERATECTL ----- */
/* Temperature Derate Control Register */
#define DDRC_DERATECTL                  0x002c
/* [31:3] rw=- reset=0x0 */
#define DDRC_DERATECTL_RESERVED_1                  31,  3
/* [2:2] rw=R/W1C reset=0x0 */
#define DDRC_DERATECTL_DERATE_TEMP_LIMIT_INTR_FORCE  2,  2
/* [1:1] rw=R/W1C reset=0x0 */
#define DDRC_DERATECTL_DERATE_TEMP_LIMIT_INTR_CLR   1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_DERATECTL_DERATE_TEMP_LIMIT_INTR_EN    0,  0

/* ----- 0x0030  PWRCTL ----- */
/* Low Power Control Register */
#define DDRC_PWRCTL                     0x0030
/* [31:9] rw=- reset=0x0 */
#define DDRC_PWRCTL_RESERVED_1                     31,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_LPDDR4_SR_ALLOWED               8,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_DIS_CAM_DRAIN_SELFREF           7,  7
/* [6:6] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_STAY_IN_SELFREF                 6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_SELFREF_SW                      5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_MPSM_EN                         4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_EN_DFI_DRAM_CLK_DISABLE         3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_DEEPPOWERDOWN_EN                2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_POWERDOWN_EN                    1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_PWRCTL_SELFREF_EN                      0,  0

/* ----- 0x0034  PWRTMG ----- */
/* Low Power Timing Register */
#define DDRC_PWRTMG                     0x0034
/* [31:24] rw=- reset=0x0 */
#define DDRC_PWRTMG_RESERVED_1                     31, 24
/* [23:16] rw=R/W reset=0x40 */
#define DDRC_PWRTMG_SELFREF_TO_X32                 23, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_PWRTMG_T_DPD_X4096                    15,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_PWRTMG_RESERVED_2                      7,  5
/* [4:0] rw=R/W reset=0x10 */
#define DDRC_PWRTMG_POWERDOWN_TO_X32                4,  0

/* ----- 0x0038  HWLPCTL ----- */
/* Hardware Low Power Control Register */
#define DDRC_HWLPCTL                    0x0038
/* [31:28] rw=- reset=0x0 */
#define DDRC_HWLPCTL_RESERVED_1                    31, 28
/* [27:16] rw=R/W reset=0x0 */
#define DDRC_HWLPCTL_HW_LP_IDLE_X32                27, 16
/* [15:2] rw=- reset=0x0 */
#define DDRC_HWLPCTL_RESERVED_2                    15,  2
/* [1:1] rw=R/W reset=0x1 */
#define DDRC_HWLPCTL_HW_LP_EXIT_IDLE_EN             1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_HWLPCTL_HW_LP_EN                       0,  0

/* ----- 0x003c  HWFFCCTL ----- */
/* Hardware Fast Frequency Change (HWFFC) Control Register */
#define DDRC_HWFFCCTL                   0x003c
/* [31:17] rw=- reset=0x0 */
#define DDRC_HWFFCCTL_RESERVED_1                   31, 17
/* [16:16] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_SKIP_MRW_ODTVREF             16, 16
/* [15:12] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_CTRL_WORD_NUM                15, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_POWER_SAVING_CTRL_WORD       11,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_CKE_POWER_DOWN_MODE           7,  7
/* [6:6] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_TARGET_VRCG                   6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_INIT_VRCG                     5,  5
/* [4:4] rw=R/W reset=0x1 */
#define DDRC_HWFFCCTL_INIT_FSP                      4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_HWFFC_VREF_EN                 3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_HWFFC_ODT_EN                  2,  2
/* [1:0] rw=R/W reset=0x0 */
#define DDRC_HWFFCCTL_HWFFC_EN                      1,  0

/* ----- 0x0040  HWFFCSTAT ----- */
/* Hardware Fast Frequency Change (HWFFC) Status Register */
#define DDRC_HWFFCSTAT                  0x0040
/* [31:10] rw=- reset=0x0 */
#define DDRC_HWFFCSTAT_RESERVED_1                  31, 10
/* [9:9] rw=R reset=0x1 */
#define DDRC_HWFFCSTAT_CURRENT_VRCG                 9,  9
/* [8:8] rw=R reset=0x1 */
#define DDRC_HWFFCSTAT_CURRENT_FSP                  8,  8
/* 变量位域(按配置实例化): current_frequency [x:4] rw=R reset=0x0 */
/* [3:2] rw=- reset=0x0 */
#define DDRC_HWFFCSTAT_RESERVED_2                   3,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_HWFFCSTAT_HWFFC_OPERATING_MODE         1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_HWFFCSTAT_HWFFC_IN_PROGRESS            0,  0

/* ----- 0x0044  HWFFCEX_RANK1 ----- */
/* Hardware Fast Frequency Change(HWFFC) Function Extended for RANK1 Register */
#define DDRC_HWFFCEX_RANK1              0x0044
/* [31:31] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RESERVED_1              31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RANK1_MR6_VREF_RANGE    30, 30
/* [29:24] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RANK1_MR6_VREF_VALUE    29, 24
/* [23:19] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RESERVED_2              23, 19
/* [18:16] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RANK1_MR5_RTT_PARK      18, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RESERVED_3              15, 11
/* [10:8] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RANK1_MR2_RTT_WR        10,  8
/* [7:3] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RESERVED_4               7,  3
/* [2:0] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK1_RANK1_MR1_RTT_NOM        2,  0

/* ----- 0x0048  HWFFCEX_RANK2 ----- */
/* Hardware Fast Frequency Change(HWFFC) Function Extended for RANK2 Register */
#define DDRC_HWFFCEX_RANK2              0x0048
/* [31:31] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RESERVED_1              31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RANK2_MR6_VREF_RANGE    30, 30
/* [29:24] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RANK2_MR6_VREF_VALUE    29, 24
/* [23:19] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RESERVED_2              23, 19
/* [18:16] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RANK2_MR5_RTT_PARK      18, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RESERVED_3              15, 11
/* [10:8] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RANK2_MR2_RTT_WR        10,  8
/* [7:3] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RESERVED_4               7,  3
/* [2:0] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK2_RANK2_MR1_RTT_NOM        2,  0

/* ----- 0x004c  HWFFCEX_RANK3 ----- */
/* Hardware Fast Frequency Change(HWFFC) Function Extended for RANK3 Register */
#define DDRC_HWFFCEX_RANK3              0x004c
/* [31:31] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RESERVED_1              31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RANK3_MR6_VREF_RANGE    30, 30
/* [29:24] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RANK3_MR6_VREF_VALUE    29, 24
/* [23:19] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RESERVED_2              23, 19
/* [18:16] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RANK3_MR5_RTT_PARK      18, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RESERVED_3              15, 11
/* [10:8] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RANK3_MR2_RTT_WR        10,  8
/* [7:3] rw=- reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RESERVED_4               7,  3
/* [2:0] rw=R/W reset=0x0 */
#define DDRC_HWFFCEX_RANK3_RANK3_MR1_RTT_NOM        2,  0

/* ----- 0x0050  RFSHCTL0 ----- */
/* Refresh Control Register 0 */
#define DDRC_RFSHCTL0                   0x0050
/* [31:27] rw=R/W reset=0x10 */
#define DDRC_RFSHCTL0_REFRESH_TO_AB_X32            31, 27
/* [26:24] rw=- reset=0x0 */
#define DDRC_RFSHCTL0_RESERVED_1                   26, 24
/* [23:20] rw=R/W reset=0x2 */
#define DDRC_RFSHCTL0_REFRESH_MARGIN               23, 20
/* [19:17] rw=- reset=0x0 */
#define DDRC_RFSHCTL0_RESERVED_2                   19, 17
/* [16:12] rw=R/W reset=0x10 */
#define DDRC_RFSHCTL0_REFRESH_TO_X1_X32            16, 12
/* [11:10] rw=- reset=0x0 */
#define DDRC_RFSHCTL0_RESERVED_3                   11, 10
/* [9:4] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL0_REFRESH_BURST                 9,  4
/* [3:3] rw=- reset=0x0 */
#define DDRC_RFSHCTL0_RESERVED_4                    3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL0_PER_BANK_REFRESH              2,  2
/* [1:0] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL0_AUTO_REFAB_EN                 1,  0

/* ----- 0x0054  RFSHCTL1 ----- */
/* Refresh Control Register 1 */
#define DDRC_RFSHCTL1                   0x0054
/* [31:28] rw=- reset=0x0 */
#define DDRC_RFSHCTL1_RESERVED_1                   31, 28
/* [27:16] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL1_REFRESH_TIMER1_START_VALUE_X32 27, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_RFSHCTL1_RESERVED_2                   15, 12
/* [11:0] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL1_REFRESH_TIMER0_START_VALUE_X32 11,  0

/* ----- 0x0058  RFSHCTL2 ----- */
/* Refresh Control Register 2 */
#define DDRC_RFSHCTL2                   0x0058
/* [31:28] rw=- reset=0x0 */
#define DDRC_RFSHCTL2_RESERVED_1                   31, 28
/* [27:16] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL2_REFRESH_TIMER3_START_VALUE_X32 27, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_RFSHCTL2_RESERVED_2                   15, 12
/* [11:0] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL2_REFRESH_TIMER2_START_VALUE_X32 11,  0

/* ----- 0x005c  RFSHCTL4 ----- */
/* Refresh Control Register 4 */
#define DDRC_RFSHCTL4                   0x005c
/* [31:11] rw=- reset=0x0 */
#define DDRC_RFSHCTL4_RESERVED_1                   31, 11
/* [10:0] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL4_REFRESH_TIMER_LR_OFFSET_X32  10,  0

/* ----- 0x0060  RFSHCTL3 ----- */
/* Refresh Control Register 3 */
#define DDRC_RFSHCTL3                   0x0060
/* 变量位域(按配置实例化): rank_dis_refresh [x:16] rw=R/W reset=0x0 */
/* [15:7] rw=- reset=0x0 */
#define DDRC_RFSHCTL3_RESERVED_1                   15,  7
/* [6:4] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL3_REFRESH_MODE                  6,  4
/* [3:2] rw=- reset=0x0 */
#define DDRC_RFSHCTL3_RESERVED_2                    3,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL3_REFRESH_UPDATE_LEVEL          1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_RFSHCTL3_DIS_AUTO_REFRESH              0,  0

/* ----- 0x0064  RFSHTMG ----- */
/* Refresh Timing Register */
#define DDRC_RFSHTMG                    0x0064
/* [31:31] rw=R/W reset=0x0 */
#define DDRC_RFSHTMG_T_RFC_NOM_X1_SEL              31, 31
/* [30:28] rw=- reset=0x0 */
#define DDRC_RFSHTMG_RESERVED_1                    30, 28
/* [27:16] rw=R/W reset=0x62 */
#define DDRC_RFSHTMG_T_RFC_NOM_X1_X32              27, 16
/* [15:15] rw=R/W reset=0x0 */
#define DDRC_RFSHTMG_LPDDR3_TREFBW_EN              15, 15
/* [14:10] rw=- reset=0x0 */
#define DDRC_RFSHTMG_RESERVED_2                    14, 10
/* [9:0] rw=R/W reset=0x8c */
#define DDRC_RFSHTMG_T_RFC_MIN                      9,  0

/* ----- 0x0068  RFSHTMG1 ----- */
/* Refresh Timing Register 1 */
#define DDRC_RFSHTMG1                   0x0068
/* [31:24] rw=- reset=0x0 */
#define DDRC_RFSHTMG1_RESERVED_1                   31, 24
/* [23:16] rw=R/W reset=0x8c */
#define DDRC_RFSHTMG1_T_PBR2PBR                    23, 16
/* [15:8] rw=- reset=0x0 */
#define DDRC_RFSHTMG1_RESERVED_2                   15,  8
/* [7:0] rw=R/W reset=0x8c */
#define DDRC_RFSHTMG1_T_RFC_MIN_DLR                 7,  0

/* ----- 0x0070  ECCCFG0 ----- */
/* ECC Configuration Register 0 */
#define DDRC_ECCCFG0                    0x0070
/* [31:30] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_ECC_REGION_MAP_GRANU          31, 30
/* [29:29] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_ECC_REGION_MAP_OTHER          29, 29
/* 变量位域(按配置实例化): ecc_ap_err_threshold [x:24] rw=R/W reset=0x0 */
/* [23:22] rw=- reset=0x0 */
#define DDRC_ECCCFG0_RESERVED_1                    23, 22
/* [21:16] rw=R/W reset=0x3f */
#define DDRC_ECCCFG0_BLK_CHANNEL_IDLE_TIME_X32     21, 16
/* [15:15] rw=- reset=0x0 */
#define DDRC_ECCCFG0_RESERVED_2                    15, 15
/* [14:8] rw=R/W reset=0x7f */
#define DDRC_ECCCFG0_ECC_REGION_MAP                14,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_ECC_REGION_REMAP_EN            7,  7
/* [6:6] rw=R/W reset=0x1 */
#define DDRC_ECCCFG0_ECC_AP_EN                      6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_ECC_TYPE                       5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_DIS_SCRUB                      4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_TEST_MODE                      3,  3
/* [2:0] rw=R/W reset=0x0 */
#define DDRC_ECCCFG0_ECC_MODE                       2,  0

/* ----- 0x0074  ECCCFG1 ----- */
/* ECC Configuration Register 1 */
#define DDRC_ECCCFG1                    0x0074
/* [31:13] rw=- reset=0x0 */
#define DDRC_ECCCFG1_RESERVED_1                    31, 13
/* [12:8] rw=R/W reset=0x3 */
#define DDRC_ECCCFG1_ACTIVE_BLK_CHANNEL            12,  8
/* [7:7] rw=R/W reset=0x1 */
#define DDRC_ECCCFG1_BLK_CHANNEL_ACTIVE_TERM        7,  7
/* [6:6] rw=- reset=0x0 */
#define DDRC_ECCCFG1_RESERVED_2                     6,  6
/* [5:5] rw=R/W reset=0x1 */
#define DDRC_ECCCFG1_ECC_REGION_WASTE_LOCK          5,  5
/* [4:4] rw=R/W reset=0x1 */
#define DDRC_ECCCFG1_ECC_REGION_PARITY_LOCK         4,  4
/* [3:3] rw=- reset=0x0 */
#define DDRC_ECCCFG1_RESERVED_3                     3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_ECCCFG1_POISON_CHIP_EN                 2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_ECCCFG1_DATA_POISON_BIT                1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_ECCCFG1_DATA_POISON_EN                 0,  0

/* ----- 0x0078  ECCSTAT ----- */
/* SECDED ECC Status Register (Valid only in MEMC_ECC_SUPPORT==1 (SECDED ECC */
#define DDRC_ECCSTAT                    0x0078
/* 变量位域(按配置实例化): ecc_uncorrected_err [x:16] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_corrected_err [x:8] rw=R reset=0x0 */
/* [7:7] rw=- reset=0x0 */
#define DDRC_ECCSTAT_RESERVED_1                     7,  7
/* [6:0] rw=R reset=0x0 */
#define DDRC_ECCSTAT_ECC_CORRECTED_BIT_NUM          6,  0

/* ----- 0x007c  ECCCTL ----- */
/* ECC Clear Register */
#define DDRC_ECCCTL                     0x007c
/* [31:19] rw=- reset=0x0 */
#define DDRC_ECCCTL_RESERVED_1                     31, 19
/* [18:18] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_AP_ERR_INTR_FORCE          18, 18
/* [17:17] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_UNCORRECTED_ERR_INTR_FORCE 17, 17
/* [16:16] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_CORRECTED_ERR_INTR_FORCE   16, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_ECCCTL_RESERVED_2                     15, 11
/* [10:10] rw=R/W reset=0x1 */
#define DDRC_ECCCTL_ECC_AP_ERR_INTR_EN             10, 10
/* [9:9] rw=R/W reset=0x1 */
#define DDRC_ECCCTL_ECC_UNCORRECTED_ERR_INTR_EN     9,  9
/* [8:8] rw=R/W reset=0x1 */
#define DDRC_ECCCTL_ECC_CORRECTED_ERR_INTR_EN       8,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_ECCCTL_RESERVED_3                      7,  5
/* [4:4] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_AP_ERR_INTR_CLR             4,  4
/* [3:3] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_UNCORR_ERR_CNT_CLR          3,  3
/* [2:2] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_CORR_ERR_CNT_CLR            2,  2
/* [1:1] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_UNCORRECTED_ERR_CLR         1,  1
/* [0:0] rw=R/W1C reset=0x0 */
#define DDRC_ECCCTL_ECC_CORRECTED_ERR_CLR           0,  0

/* ----- 0x0080  ECCERRCNT ----- */
/* ECC Error Counter Register */
#define DDRC_ECCERRCNT                  0x0080
/* [31:16] rw=R reset=0x0 */
#define DDRC_ECCERRCNT_ECC_UNCORR_ERR_CNT          31, 16
/* [15:0] rw=R reset=0x0 */
#define DDRC_ECCERRCNT_ECC_CORR_ERR_CNT            15,  0

/* ----- 0x0084  ECCCADDR0 ----- */
/* ECC Corrected Error Address Register 0 */
#define DDRC_ECCCADDR0                  0x0084
/* 变量位域(按配置实例化): ecc_corr_rank [x:24] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_corr_row [x:0] rw=R reset=0x0 */

/* ----- 0x0088  ECCCADDR1 ----- */
/* ECC Corrected Error Address Register 1 */
#define DDRC_ECCCADDR1                  0x0088
/* 变量位域(按配置实例化): ecc_corr_cid [x:28] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_corr_bg [x:24] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_corr_bank [x:16] rw=R reset=0x0 */
/* [15:12] rw=- reset=0x0 */
#define DDRC_ECCCADDR1_RESERVED_1                  15, 12
/* [11:0] rw=R reset=0x0 */
#define DDRC_ECCCADDR1_ECC_CORR_COL                11,  0

/* ----- 0x008c  ECCCSYN0 ----- */
/* ECC Corrected Syndrome Register 0 */
#define DDRC_ECCCSYN0                   0x008c
/* [31:0] rw=R reset=0x0 */
#define DDRC_ECCCSYN0_ECC_CORR_SYNDROMES_31_0      31,  0

/* ----- 0x0090  ECCCSYN1 ----- */
/* ECC Corrected Syndrome Register 1 */
#define DDRC_ECCCSYN1                   0x0090
/* [31:0] rw=R reset=0x0 */
#define DDRC_ECCCSYN1_ECC_CORR_SYNDROMES_63_32     31,  0

/* ----- 0x0094  ECCCSYN2 ----- */
/* ECC Corrected Syndrome Register 2 */
#define DDRC_ECCCSYN2                   0x0094
/* [31:16] rw=- reset=0x0 */
#define DDRC_ECCCSYN2_RESERVED_1                   31, 16
/* [15:8] rw=R reset=0x0 */
#define DDRC_ECCCSYN2_ECC_CORR_SYNDROMES_79_72     15,  8
/* [7:0] rw=R reset=0x0 */
#define DDRC_ECCCSYN2_ECC_CORR_SYNDROMES_71_64      7,  0

/* ----- 0x0098  ECCBITMASK0 ----- */
/* ECC Corrected Data Bit Mask Register 0 */
#define DDRC_ECCBITMASK0                0x0098
/* [31:0] rw=R reset=0x0 */
#define DDRC_ECCBITMASK0_ECC_CORR_BIT_MASK_31_0    31,  0

/* ----- 0x009c  ECCBITMASK1 ----- */
/* ECC Corrected Data Bit Mask Register 1 */
#define DDRC_ECCBITMASK1                0x009c
/* [31:0] rw=R reset=0x0 */
#define DDRC_ECCBITMASK1_ECC_CORR_BIT_MASK_63_32   31,  0

/* ----- 0x00a0  ECCBITMASK2 ----- */
/* ECC Corrected Data Bit Mask Register 2 */
#define DDRC_ECCBITMASK2                0x00a0
/* [31:16] rw=- reset=0x0 */
#define DDRC_ECCBITMASK2_RESERVED_1                31, 16
/* [15:8] rw=R reset=0x0 */
#define DDRC_ECCBITMASK2_ECC_CORR_BIT_MASK_79_72   15,  8
/* [7:0] rw=R reset=0x0 */
#define DDRC_ECCBITMASK2_ECC_CORR_BIT_MASK_71_64    7,  0

/* ----- 0x00a4  ECCUADDR0 ----- */
/* ECC Uncorrected Error Address Register 0 */
#define DDRC_ECCUADDR0                  0x00a4
/* 变量位域(按配置实例化): ecc_uncorr_rank [x:24] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_uncorr_row [x:0] rw=R reset=0x0 */

/* ----- 0x00a8  ECCUADDR1 ----- */
/* ECC Uncorrected Error Address Register 1 */
#define DDRC_ECCUADDR1                  0x00a8
/* 变量位域(按配置实例化): ecc_uncorr_cid [x:28] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_uncorr_bg [x:24] rw=R reset=0x0 */
/* 变量位域(按配置实例化): ecc_uncorr_bank [x:16] rw=R reset=0x0 */
/* [15:12] rw=- reset=0x0 */
#define DDRC_ECCUADDR1_RESERVED_1                  15, 12
/* [11:0] rw=R reset=0x0 */
#define DDRC_ECCUADDR1_ECC_UNCORR_COL              11,  0

/* ----- 0x00ac  ECCUSYN0 ----- */
/* ECC Uncorrected Syndrome Register 0 */
#define DDRC_ECCUSYN0                   0x00ac
/* [31:0] rw=R reset=0x0 */
#define DDRC_ECCUSYN0_ECC_UNCORR_SYNDROMES_31_0    31,  0

/* ----- 0x00b0  ECCUSYN1 ----- */
/* ECC Uncorrected Syndrome Register 1 */
#define DDRC_ECCUSYN1                   0x00b0
/* [31:0] rw=R reset=0x0 */
#define DDRC_ECCUSYN1_ECC_UNCORR_SYNDROMES_63_32   31,  0

/* ----- 0x00b4  ECCUSYN2 ----- */
/* ECC Uncorrected Syndrome Register 2 */
#define DDRC_ECCUSYN2                   0x00b4
/* [31:16] rw=- reset=0x0 */
#define DDRC_ECCUSYN2_RESERVED_1                   31, 16
/* [15:8] rw=R reset=0x0 */
#define DDRC_ECCUSYN2_ECC_UNCORR_SYNDROMES_79_72   15,  8
/* [7:0] rw=R reset=0x0 */
#define DDRC_ECCUSYN2_ECC_UNCORR_SYNDROMES_71_64    7,  0

/* ----- 0x00b8  ECCPOISONADDR0 ----- */
/* ECC Data Poisoning Address Register 0. If a HIF write data beat matches the address */
#define DDRC_ECCPOISONADDR0             0x00b8
/* 变量位域(按配置实例化): ecc_poison_rank [x:24] rw=R/W reset=0x0 */
/* 变量位域(按配置实例化): ecc_poison_cid [x:16] rw=R/W reset=0x0 */
/* [15:12] rw=- reset=0x0 */
#define DDRC_ECCPOISONADDR0_RESERVED_1             15, 12
/* [11:0] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONADDR0_ECC_POISON_COL         11,  0

/* ----- 0x00bc  ECCPOISONADDR1 ----- */
/* ECC Data Poisoning Address Register 1. If a HIF write data beat matches the address */
#define DDRC_ECCPOISONADDR1             0x00bc
/* [31:30] rw=- reset=0x0 */
#define DDRC_ECCPOISONADDR1_RESERVED_1             31, 30
/* [29:28] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONADDR1_ECC_POISON_BG          29, 28
/* [27:27] rw=- reset=0x0 */
#define DDRC_ECCPOISONADDR1_RESERVED_2             27, 27
/* [26:24] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONADDR1_ECC_POISON_BANK        26, 24
/* 变量位域(按配置实例化): ecc_poison_row [x:0] rw=R/W reset=0x0 */

/* ----- 0x00c0  CRCPARCTL0 ----- */
/* CRC Parity Control Register 0. */
#define DDRC_CRCPARCTL0                 0x00c0
/* [31:19] rw=- reset=0x0 */
#define DDRC_CRCPARCTL0_RESERVED_1                 31, 19
/* [18:16] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL0_RETRY_CTRLUPD_WAIT         18, 16
/* [15:15] rw=R/W reset=0x1 */
#define DDRC_CRCPARCTL0_RETRY_CTRLUPD_ENABLE       15, 15
/* [14:9] rw=- reset=0x0 */
#define DDRC_CRCPARCTL0_RESERVED_2                 14,  9
/* [8:8] rw=R/W1C reset=0x0 */
#define DDRC_CRCPARCTL0_DFI_ALERT_ERR_MAX_REACHED_INT_CLR  8,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_CRCPARCTL0_RESERVED_3                  7,  5
/* [4:4] rw=R/W1C reset=0x0 */
#define DDRC_CRCPARCTL0_DFI_ALERT_ERR_FATL_INT_CLR  4,  4
/* [3:3] rw=- reset=0x0 */
#define DDRC_CRCPARCTL0_RESERVED_4                  3,  3
/* [2:2] rw=R/W1C reset=0x0 */
#define DDRC_CRCPARCTL0_DFI_ALERT_ERR_CNT_CLR       2,  2
/* [1:1] rw=R/W1C reset=0x0 */
#define DDRC_CRCPARCTL0_DFI_ALERT_ERR_INT_CLR       1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL0_DFI_ALERT_ERR_INT_EN        0,  0

/* ----- 0x00c4  CRCPARCTL1 ----- */
/* CRC Parity Control Register 1 */
#define DDRC_CRCPARCTL1                 0x00c4
/* [31:31] rw=- reset=0x0 */
#define DDRC_CRCPARCTL1_RESERVED_1                 31, 31
/* [30:24] rw=R/W reset=0x10 */
#define DDRC_CRCPARCTL1_DFI_T_PHY_RDLAT            30, 24
/* 变量位域(按配置实例化): retry_add_rd_lat [x:16] rw=R/W reset=0x0 */
/* [15:15] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL1_RETRY_ADD_RD_LAT_EN        15, 15
/* [14:13] rw=- reset=0x0 */
#define DDRC_CRCPARCTL1_RESERVED_2                 14, 13
/* [12:12] rw=R/W reset=0x1 */
#define DDRC_CRCPARCTL1_CAPARITY_DISABLE_BEFORE_SR 12, 12
/* [11:10] rw=- reset=0x0 */
#define DDRC_CRCPARCTL1_RESERVED_3                 11, 10
/* [9:9] rw=R/W reset=0x1 */
#define DDRC_CRCPARCTL1_ALERT_WAIT_FOR_SW           9,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL1_CRC_PARITY_RETRY_ENABLE     8,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL1_CRC_INC_DM                  7,  7
/* [6:5] rw=- reset=0x0 */
#define DDRC_CRCPARCTL1_RESERVED_4                  6,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL1_CRC_ENABLE                  4,  4
/* [3:1] rw=- reset=0x0 */
#define DDRC_CRCPARCTL1_RESERVED_5                  3,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_CRCPARCTL1_PARITY_ENABLE               0,  0

/* ----- 0x00c8  CRCPARCTL2 ----- */
/* CRC Parity Control Register 2 */
#define DDRC_CRCPARCTL2                 0x00c8
/* [31:25] rw=- reset=0x0 */
#define DDRC_CRCPARCTL2_RESERVED_1                 31, 25
/* [24:16] rw=R/W reset=0x30 */
#define DDRC_CRCPARCTL2_T_PAR_ALERT_PW_MAX         24, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_CRCPARCTL2_RESERVED_2                 15, 13
/* [12:8] rw=R/W reset=0x5 */
#define DDRC_CRCPARCTL2_T_CRC_ALERT_PW_MAX         12,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_CRCPARCTL2_RESERVED_3                  7,  6
/* [5:0] rw=R/W reset=0xc */
#define DDRC_CRCPARCTL2_RETRY_FIFO_MAX_HOLD_TIMER_X4  5,  0

/* ----- 0x00cc  CRCPARSTAT ----- */
/* CRC Parity Status Register */
#define DDRC_CRCPARSTAT                 0x00cc
/* [31:30] rw=- reset=0x0 */
#define DDRC_CRCPARSTAT_RESERVED_1                 31, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_CMD_IN_ERR_WINDOW          29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_RETRY_OPERATING_MODE       28, 28
/* [27:24] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_RETRY_CURRENT_STATE        27, 24
/* 变量位域(按配置实例化): dfi_alert_err_fatl_code [x:20] rw=R reset=0x0 */
/* [19:19] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_DFI_ALERT_ERR_NO_SW        19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_DFI_ALERT_ERR_MAX_REACHED_INT 18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_DFI_ALERT_ERR_FATL_INT     17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_DFI_ALERT_ERR_INT          16, 16
/* [15:0] rw=R reset=0x0 */
#define DDRC_CRCPARSTAT_DFI_ALERT_ERR_CNT          15,  0

/* ----- 0x00d0  INIT0 ----- */
/* SDRAM Initialization Register 0 */
#define DDRC_INIT0                      0x00d0
/* [31:30] rw=R/W reset=0x0 */
#define DDRC_INIT0_SKIP_DRAM_INIT                  31, 30
/* [29:26] rw=- reset=0x0 */
#define DDRC_INIT0_RESERVED_1                      29, 26
/* [25:16] rw=R/W reset=0x2 */
#define DDRC_INIT0_POST_CKE_X1024                  25, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_INIT0_RESERVED_2                      15, 12
/* [11:0] rw=R/W reset=0x4e */
#define DDRC_INIT0_PRE_CKE_X1024                   11,  0

/* ----- 0x00d4  INIT1 ----- */
/* SDRAM Initialization Register 1 */
#define DDRC_INIT1                      0x00d4
/* [31:25] rw=- reset=0x0 */
#define DDRC_INIT1_RESERVED_1                      31, 25
/* [24:16] rw=R/W reset=0x0 */
#define DDRC_INIT1_DRAM_RSTN_X1024                 24, 16
/* [15:4] rw=- reset=0x0 */
#define DDRC_INIT1_RESERVED_2                      15,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_INIT1_PRE_OCD_X32                      3,  0

/* ----- 0x00d8  INIT2 ----- */
/* SDRAM Initialization Register 2 */
#define DDRC_INIT2                      0x00d8
/* [31:16] rw=- reset=0x0 */
#define DDRC_INIT2_RESERVED_1                      31, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_INIT2_IDLE_AFTER_RESET_X32            15,  8
/* [7:0] rw=- reset=0x0 */
#define DDRC_INIT2_RESERVED_2                       7,  0

/* ----- 0x00dc  INIT3 ----- */
/* SDRAM Initialization Register 3 */
#define DDRC_INIT3                      0x00dc
/* [31:16] rw=R/W reset=0x0 */
#define DDRC_INIT3_MR                              31, 16
/* [15:0] rw=R/W reset=0x510 */
#define DDRC_INIT3_EMR                             15,  0

/* ----- 0x00e0  INIT4 ----- */
/* SDRAM Initialization Register 4 */
#define DDRC_INIT4                      0x00e0
/* [31:16] rw=R/W reset=0x0 */
#define DDRC_INIT4_EMR2                            31, 16
/* [15:0] rw=R/W reset=0x0 */
#define DDRC_INIT4_EMR3                            15,  0

/* ----- 0x00e4  INIT5 ----- */
/* SDRAM Initialization Register 5 */
#define DDRC_INIT5                      0x00e4
/* [31:24] rw=- reset=0x0 */
#define DDRC_INIT5_RESERVED_1                      31, 24
/* [23:16] rw=R/W reset=0x10 */
#define DDRC_INIT5_DEV_ZQINIT_X32                  23, 16
/* [15:10] rw=- reset=0x0 */
#define DDRC_INIT5_RESERVED_2                      15, 10
/* [9:0] rw=R/W reset=0x0 */
#define DDRC_INIT5_MAX_AUTO_INIT_X1024              9,  0

/* ----- 0x00e8  INIT6 ----- */
/* SDRAM Initialization Register 6 */
#define DDRC_INIT6                      0x00e8
/* [31:16] rw=R/W reset=0x0 */
#define DDRC_INIT6_MR4                             31, 16
/* [15:0] rw=R/W reset=0x0 */
#define DDRC_INIT6_MR5                             15,  0

/* ----- 0x00ec  INIT7 ----- */
/* SDRAM Initialization Register 7 */
#define DDRC_INIT7                      0x00ec
/* [31:16] rw=R/W reset=0x0 */
#define DDRC_INIT7_MR22                            31, 16
/* [15:0] rw=R/W reset=0x0 */
#define DDRC_INIT7_MR6                             15,  0

/* ----- 0x00f0  DIMMCTL ----- */
/* DIMM Control Register */
#define DDRC_DIMMCTL                    0x00f0
/* [31:15] rw=- reset=0x0 */
#define DDRC_DIMMCTL_RESERVED_1                    31, 15
/* [14:14] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_RCD_B_OUTPUT_DISABLED         14, 14
/* [13:13] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_RCD_A_OUTPUT_DISABLED         13, 13
/* [12:12] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_RCD_WEAK_DRIVE                12, 12
/* [11:10] rw=- reset=0x0 */
#define DDRC_DIMMCTL_RESERVED_2                    11, 10
/* [9:8] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_RCD_NUM                        9,  8
/* [7:7] rw=- reset=0x0 */
#define DDRC_DIMMCTL_RESERVED_3                     7,  7
/* [6:6] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_LRDIMM_BCOM_CMD_PROT           6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_DIMM_DIS_BG_MIRRORING          5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_MRS_BG1_EN                     4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_MRS_A17_EN                     3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_DIMM_OUTPUT_INV_EN             2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_DIMM_ADDR_MIRR_EN              1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DIMMCTL_DIMM_STAGGER_CS_EN             0,  0

/* ----- 0x00f4  RANKCTL ----- */
/* Rank Control Register */
#define DDRC_RANKCTL                    0x00f4
/* [31:27] rw=- reset=0x0 */
#define DDRC_RANKCTL_RESERVED_1                    31, 27
/* [26:26] rw=R/W reset=0x0 */
#define DDRC_RANKCTL_DIFF_RANK_WR_GAP_MSB          26, 26
/* [25:25] rw=- reset=0x0 */
#define DDRC_RANKCTL_RESERVED_2                    25, 25
/* [24:24] rw=R/W reset=0x0 */
#define DDRC_RANKCTL_DIFF_RANK_RD_GAP_MSB          24, 24
/* [23:20] rw=R/W reset=0x0 */
#define DDRC_RANKCTL_MAX_LOGICAL_RANK_WR           23, 20
/* [19:16] rw=R/W reset=0xf */
#define DDRC_RANKCTL_MAX_LOGICAL_RANK_RD           19, 16
/* [15:12] rw=R/W reset=0x0 */
#define DDRC_RANKCTL_MAX_RANK_WR                   15, 12
/* [11:8] rw=R/W reset=0x6 */
#define DDRC_RANKCTL_DIFF_RANK_WR_GAP              11,  8
/* [7:4] rw=R/W reset=0x6 */
#define DDRC_RANKCTL_DIFF_RANK_RD_GAP               7,  4
/* [3:0] rw=R/W reset=0xf */
#define DDRC_RANKCTL_MAX_RANK_RD                    3,  0

/* ----- 0x00f8  RANKCTL1 ----- */
/* Rank Control Register 1 */
#define DDRC_RANKCTL1                   0x00f8
/* [31:6] rw=- reset=0x0 */
#define DDRC_RANKCTL1_RESERVED_1                   31,  6
/* [5:0] rw=R/W reset=0x8 */
#define DDRC_RANKCTL1_WR2RD_DR                      5,  0

/* ----- 0x00fc  CHCTL ----- */
/* Channel Control Register */
#define DDRC_CHCTL                      0x00fc
/* [31:2] rw=- reset=0x0 */
#define DDRC_CHCTL_RESERVED_1                      31,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_CHCTL_DUAL_CHANNEL_MODE                1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_CHCTL_DUAL_CHANNEL_EN                  0,  0

/* ----- 0x0100  DRAMTMG0 ----- */
/* SDRAM Timing Register 0 */
#define DDRC_DRAMTMG0                   0x0100
/* [31:31] rw=- reset=0x0 */
#define DDRC_DRAMTMG0_RESERVED_1                   31, 31
/* [30:24] rw=R/W reset=0xf */
#define DDRC_DRAMTMG0_WR2PRE                       30, 24
/* [23:22] rw=- reset=0x0 */
#define DDRC_DRAMTMG0_RESERVED_2                   23, 22
/* [21:16] rw=R/W reset=0x10 */
#define DDRC_DRAMTMG0_T_FAW                        21, 16
/* [15:15] rw=- reset=0x0 */
#define DDRC_DRAMTMG0_RESERVED_3                   15, 15
/* [14:8] rw=R/W reset=0x1b */
#define DDRC_DRAMTMG0_T_RAS_MAX                    14,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DRAMTMG0_RESERVED_4                    7,  6
/* [5:0] rw=R/W reset=0xf */
#define DDRC_DRAMTMG0_T_RAS_MIN                     5,  0

/* ----- 0x0104  DRAMTMG1 ----- */
/* SDRAM Timing Register 1 */
#define DDRC_DRAMTMG1                   0x0104
/* [31:21] rw=- reset=0x0 */
#define DDRC_DRAMTMG1_RESERVED_1                   31, 21
/* [20:16] rw=R/W reset=0x8 */
#define DDRC_DRAMTMG1_T_XP                         20, 16
/* [15:14] rw=- reset=0x0 */
#define DDRC_DRAMTMG1_RESERVED_2                   15, 14
/* [13:8] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG1_RD2PRE                       13,  8
/* [7:7] rw=- reset=0x0 */
#define DDRC_DRAMTMG1_RESERVED_3                    7,  7
/* [6:0] rw=R/W reset=0x14 */
#define DDRC_DRAMTMG1_T_RC                          6,  0

/* ----- 0x0108  DRAMTMG2 ----- */
/* SDRAM Timing Register 2 */
#define DDRC_DRAMTMG2                   0x0108
/* [31:30] rw=- reset=0x0 */
#define DDRC_DRAMTMG2_RESERVED_1                   31, 30
/* [29:24] rw=R/W reset=0x3 */
#define DDRC_DRAMTMG2_WRITE_LATENCY                29, 24
/* [23:22] rw=- reset=0x0 */
#define DDRC_DRAMTMG2_RESERVED_2                   23, 22
/* [21:16] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG2_READ_LATENCY                 21, 16
/* [15:14] rw=- reset=0x0 */
#define DDRC_DRAMTMG2_RESERVED_3                   15, 14
/* [13:8] rw=R/W reset=0x6 */
#define DDRC_DRAMTMG2_RD2WR                        13,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DRAMTMG2_RESERVED_4                    7,  6
/* [5:0] rw=R/W reset=0xd */
#define DDRC_DRAMTMG2_WR2RD                         5,  0

/* ----- 0x010c  DRAMTMG3 ----- */
/* SDRAM Timing Register 3 */
#define DDRC_DRAMTMG3                   0x010c
/* [31:30] rw=- reset=0x0 */
#define DDRC_DRAMTMG3_RESERVED_1                   31, 30
/* [29:20] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG3_T_MRW                        29, 20
/* [19:18] rw=- reset=0x0 */
#define DDRC_DRAMTMG3_RESERVED_2                   19, 18
/* [17:12] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG3_T_MRD                        17, 12
/* [11:10] rw=- reset=0x0 */
#define DDRC_DRAMTMG3_RESERVED_3                   11, 10
/* [9:0] rw=R/W reset=0xc */
#define DDRC_DRAMTMG3_T_MOD                         9,  0

/* ----- 0x0110  DRAMTMG4 ----- */
/* SDRAM Timing Register 4 */
#define DDRC_DRAMTMG4                   0x0110
/* [31:29] rw=- reset=0x0 */
#define DDRC_DRAMTMG4_RESERVED_1                   31, 29
/* [28:24] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG4_T_RCD                        28, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_DRAMTMG4_RESERVED_2                   23, 20
/* [19:16] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG4_T_CCD                        19, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_DRAMTMG4_RESERVED_3                   15, 12
/* [11:8] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG4_T_RRD                        11,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_DRAMTMG4_RESERVED_4                    7,  5
/* [4:0] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG4_T_RP                          4,  0

/* ----- 0x0114  DRAMTMG5 ----- */
/* SDRAM Timing Register 5 */
#define DDRC_DRAMTMG5                   0x0114
/* [31:28] rw=- reset=0x0 */
#define DDRC_DRAMTMG5_RESERVED_1                   31, 28
/* [27:24] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG5_T_CKSRX                      27, 24
/* [23:16] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG5_T_CKSRE                      23, 16
/* [15:8] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG5_T_CKESR                      15,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_DRAMTMG5_RESERVED_2                    7,  5
/* [4:0] rw=R/W reset=0x3 */
#define DDRC_DRAMTMG5_T_CKE                         4,  0

/* ----- 0x0118  DRAMTMG6 ----- */
/* SDRAM Timing Register 6 */
#define DDRC_DRAMTMG6                   0x0118
/* [31:28] rw=- reset=0x0 */
#define DDRC_DRAMTMG6_RESERVED_1                   31, 28
/* [27:24] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG6_T_CKDPDE                     27, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_DRAMTMG6_RESERVED_2                   23, 20
/* [19:16] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG6_T_CKDPDX                     19, 16
/* [15:4] rw=- reset=0x0 */
#define DDRC_DRAMTMG6_RESERVED_3                   15,  4
/* [3:0] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG6_T_CKCSX                       3,  0

/* ----- 0x011c  DRAMTMG7 ----- */
/* SDRAM Timing Register 7 */
#define DDRC_DRAMTMG7                   0x011c
/* [31:12] rw=- reset=0x0 */
#define DDRC_DRAMTMG7_RESERVED_1                   31, 12
/* [11:8] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG7_T_CKPDE                      11,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_DRAMTMG7_RESERVED_2                    7,  4
/* [3:0] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG7_T_CKPDX                       3,  0

/* ----- 0x0120  DRAMTMG8 ----- */
/* SDRAM Timing Register 8 */
#define DDRC_DRAMTMG8                   0x0120
/* [31:31] rw=- reset=0x0 */
#define DDRC_DRAMTMG8_RESERVED_1                   31, 31
/* [30:24] rw=R/W reset=0x3 */
#define DDRC_DRAMTMG8_T_XS_FAST_X32                30, 24
/* [23:23] rw=- reset=0x0 */
#define DDRC_DRAMTMG8_RESERVED_2                   23, 23
/* [22:16] rw=R/W reset=0x3 */
#define DDRC_DRAMTMG8_T_XS_ABORT_X32               22, 16
/* [15:15] rw=- reset=0x0 */
#define DDRC_DRAMTMG8_RESERVED_3                   15, 15
/* [14:8] rw=R/W reset=0x44 */
#define DDRC_DRAMTMG8_T_XS_DLL_X32                 14,  8
/* [7:7] rw=- reset=0x0 */
#define DDRC_DRAMTMG8_RESERVED_4                    7,  7
/* [6:0] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG8_T_XS_X32                      6,  0

/* ----- 0x0124  DRAMTMG9 ----- */
/* SDRAM Timing Register 9 */
#define DDRC_DRAMTMG9                   0x0124
/* [31:31] rw=- reset=0x0 */
#define DDRC_DRAMTMG9_RESERVED_1                   31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG9_DDR4_WR_PREAMBLE             30, 30
/* [29:19] rw=- reset=0x0 */
#define DDRC_DRAMTMG9_RESERVED_2                   29, 19
/* [18:16] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG9_T_CCD_S                      18, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_DRAMTMG9_RESERVED_3                   15, 12
/* [11:8] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG9_T_RRD_S                      11,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DRAMTMG9_RESERVED_4                    7,  6
/* [5:0] rw=R/W reset=0xd */
#define DDRC_DRAMTMG9_WR2RD_S                       5,  0

/* ----- 0x0128  DRAMTMG10 ----- */
/* SDRAM Timing Register 10 */
#define DDRC_DRAMTMG10                  0x0128
/* [31:21] rw=- reset=0x0 */
#define DDRC_DRAMTMG10_RESERVED_1                  31, 21
/* [20:16] rw=R/W reset=0x1c */
#define DDRC_DRAMTMG10_T_SYNC_GEAR                 20, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_DRAMTMG10_RESERVED_2                  15, 13
/* [12:8] rw=R/W reset=0x18 */
#define DDRC_DRAMTMG10_T_CMD_GEAR                  12,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_DRAMTMG10_RESERVED_3                   7,  4
/* [3:2] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG10_T_GEAR_SETUP                 3,  2
/* [1:0] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG10_T_GEAR_HOLD                  1,  0

/* ----- 0x012c  DRAMTMG11 ----- */
/* SDRAM Timing Register 11 */
#define DDRC_DRAMTMG11                  0x012c
/* [31:31] rw=- reset=0x0 */
#define DDRC_DRAMTMG11_RESERVED_1                  31, 31
/* [30:24] rw=R/W reset=0x44 */
#define DDRC_DRAMTMG11_POST_MPSM_GAP_X32           30, 24
/* [23:21] rw=- reset=0x0 */
#define DDRC_DRAMTMG11_RESERVED_2                  23, 21
/* [20:16] rw=R/W reset=0xc */
#define DDRC_DRAMTMG11_T_MPX_LH                    20, 16
/* [15:10] rw=- reset=0x0 */
#define DDRC_DRAMTMG11_RESERVED_3                  15, 10
/* [9:8] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG11_T_MPX_S                      9,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_DRAMTMG11_RESERVED_4                   7,  5
/* [4:0] rw=R/W reset=0x1c */
#define DDRC_DRAMTMG11_T_CKMPE                      4,  0

/* ----- 0x0130  DRAMTMG12 ----- */
/* SDRAM Timing Register 12 */
#define DDRC_DRAMTMG12                  0x0130
/* [31:30] rw=- reset=0x0 */
#define DDRC_DRAMTMG12_RESERVED_1                  31, 30
/* [29:24] rw=R/W reset=0x1a */
#define DDRC_DRAMTMG12_T_WR_MPR                    29, 24
/* [23:18] rw=- reset=0x0 */
#define DDRC_DRAMTMG12_RESERVED_2                  23, 18
/* [17:16] rw=R/W reset=0x2 */
#define DDRC_DRAMTMG12_T_CMDCKE                    17, 16
/* [15:5] rw=- reset=0x0 */
#define DDRC_DRAMTMG12_RESERVED_3                  15,  5
/* [4:0] rw=R/W reset=0x10 */
#define DDRC_DRAMTMG12_T_MRD_PDA                    4,  0

/* ----- 0x0134  DRAMTMG13 ----- */
/* SDRAM Timing Register 13 */
#define DDRC_DRAMTMG13                  0x0134
/* [31:31] rw=- reset=0x0 */
#define DDRC_DRAMTMG13_RESERVED_1                  31, 31
/* [30:24] rw=R/W reset=0x1c */
#define DDRC_DRAMTMG13_ODTLOFF                     30, 24
/* [23:22] rw=- reset=0x0 */
#define DDRC_DRAMTMG13_RESERVED_2                  23, 22
/* [21:16] rw=R/W reset=0x20 */
#define DDRC_DRAMTMG13_T_CCD_MW                    21, 16
/* [15:3] rw=- reset=0x0 */
#define DDRC_DRAMTMG13_RESERVED_3                  15,  3
/* [2:0] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG13_T_PPD                        2,  0

/* ----- 0x0138  DRAMTMG14 ----- */
/* SDRAM Timing Register 14 */
#define DDRC_DRAMTMG14                  0x0138
/* [31:12] rw=- reset=0x0 */
#define DDRC_DRAMTMG14_RESERVED_1                  31, 12
/* [11:0] rw=R/W reset=0xa0 */
#define DDRC_DRAMTMG14_T_XSR                       11,  0

/* ----- 0x013c  DRAMTMG15 ----- */
/* SDRAM Timing Register 15 */
#define DDRC_DRAMTMG15                  0x013c
/* [31:31] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG15_EN_DFI_LP_T_STAB            31, 31
/* [30:25] rw=- reset=0x0 */
#define DDRC_DRAMTMG15_RESERVED_1                  30, 25
/* [24:24] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG15_EN_HWFFC_T_STAB             24, 24
/* [23:8] rw=- reset=0x0 */
#define DDRC_DRAMTMG15_RESERVED_2                  23,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG15_T_STAB_X32                   7,  0

/* ----- 0x0140  DRAMTMG16 ----- */
/* SDRAM Timing Register 16 */
#define DDRC_DRAMTMG16                  0x0140
/* [31:24] rw=R/W reset=0x5 */
#define DDRC_DRAMTMG16_T_RP_CA_PARITY              31, 24
/* [23:21] rw=- reset=0x0 */
#define DDRC_DRAMTMG16_RESERVED_1                  23, 21
/* [20:16] rw=R/W reset=0x10 */
#define DDRC_DRAMTMG16_T_FAW_DLR                   20, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_DRAMTMG16_RESERVED_2                  15, 11
/* [10:8] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG16_T_RRD_DLR                   10,  8
/* [7:3] rw=- reset=0x0 */
#define DDRC_DRAMTMG16_RESERVED_3                   7,  3
/* [2:0] rw=R/W reset=0x4 */
#define DDRC_DRAMTMG16_T_CCD_DLR                    2,  0

/* ----- 0x0144  DRAMTMG17 ----- */
/* SDRAM Timing Register 17 */
#define DDRC_DRAMTMG17                  0x0144
/* [31:24] rw=- reset=0x0 */
#define DDRC_DRAMTMG17_RESERVED_1                  31, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG17_T_VRCG_ENABLE               23, 16
/* [15:8] rw=- reset=0x0 */
#define DDRC_DRAMTMG17_RESERVED_2                  15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DRAMTMG17_T_VRCG_DISABLE               7,  0

/* ----- 0x0150  RFSHTMG_HET ----- */
/* Refresh Timing Register Heterogeneous */
#define DDRC_RFSHTMG_HET                0x0150
/* [31:10] rw=- reset=0x0 */
#define DDRC_RFSHTMG_HET_RESERVED_1                31, 10
/* [9:0] rw=R/W reset=0x8c */
#define DDRC_RFSHTMG_HET_T_RFC_MIN_HET              9,  0

/* ----- 0x0170  MRAMTMG0 ----- */
/* MRAM Timing Register 0 */
#define DDRC_MRAMTMG0                   0x0170
/* [31:24] rw=- reset=0x0 */
#define DDRC_MRAMTMG0_RESERVED_1                   31, 24
/* [23:16] rw=R/W reset=0x10 */
#define DDRC_MRAMTMG0_T_FAW_MRAM                   23, 16
/* [15:7] rw=- reset=0x0 */
#define DDRC_MRAMTMG0_RESERVED_2                   15,  7
/* [6:0] rw=R/W reset=0xf */
#define DDRC_MRAMTMG0_T_RAS_MIN_MRAM                6,  0

/* ----- 0x0174  MRAMTMG1 ----- */
/* MRAM Timing Register 1 */
#define DDRC_MRAMTMG1                   0x0174
/* [31:8] rw=- reset=0x0 */
#define DDRC_MRAMTMG1_RESERVED_1                   31,  8
/* [7:0] rw=R/W reset=0x14 */
#define DDRC_MRAMTMG1_T_RC_MRAM                     7,  0

/* ----- 0x0178  MRAMTMG4 ----- */
/* MRAM Timing Register 4 */
#define DDRC_MRAMTMG4                   0x0178
/* [31:31] rw=- reset=0x0 */
#define DDRC_MRAMTMG4_RESERVED_1                   31, 31
/* [30:24] rw=R/W reset=0x5 */
#define DDRC_MRAMTMG4_T_RCD_MRAM                   30, 24
/* [23:14] rw=- reset=0x0 */
#define DDRC_MRAMTMG4_RESERVED_2                   23, 14
/* [13:8] rw=R/W reset=0x4 */
#define DDRC_MRAMTMG4_T_RRD_MRAM                   13,  8
/* [7:7] rw=- reset=0x0 */
#define DDRC_MRAMTMG4_RESERVED_3                    7,  7
/* [6:0] rw=R/W reset=0x5 */
#define DDRC_MRAMTMG4_T_RP_MRAM                     6,  0

/* ----- 0x017c  MRAMTMG9 ----- */
/* MRAM Timing Register 9 */
#define DDRC_MRAMTMG9                   0x017c
/* [31:14] rw=- reset=0x0 */
#define DDRC_MRAMTMG9_RESERVED_1                   31, 14
/* [13:8] rw=R/W reset=0x4 */
#define DDRC_MRAMTMG9_T_RRD_S_MRAM                 13,  8
/* [7:0] rw=- reset=0x0 */
#define DDRC_MRAMTMG9_RESERVED_2                    7,  0

/* ----- 0x0180  ZQCTL0 ----- */
/* ZQ Control Register 0 */
#define DDRC_ZQCTL0                     0x0180
/* [31:31] rw=R/W reset=0x0 */
#define DDRC_ZQCTL0_DIS_AUTO_ZQ                    31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_ZQCTL0_DIS_SRX_ZQCL                   30, 30
/* [29:29] rw=R/W reset=0x0 */
#define DDRC_ZQCTL0_ZQ_RESISTOR_SHARED             29, 29
/* [28:28] rw=R/W reset=0x0 */
#define DDRC_ZQCTL0_DIS_MPSMX_ZQCL                 28, 28
/* [27:27] rw=- reset=0x0 */
#define DDRC_ZQCTL0_RESERVED_1                     27, 27
/* [26:16] rw=R/W reset=0x200 */
#define DDRC_ZQCTL0_T_ZQ_LONG_NOP                  26, 16
/* [15:10] rw=- reset=0x0 */
#define DDRC_ZQCTL0_RESERVED_2                     15, 10
/* [9:0] rw=R/W reset=0x40 */
#define DDRC_ZQCTL0_T_ZQ_SHORT_NOP                  9,  0

/* ----- 0x0184  ZQCTL1 ----- */
/* ZQ Control Register 1 */
#define DDRC_ZQCTL1                     0x0184
/* [31:30] rw=- reset=0x0 */
#define DDRC_ZQCTL1_RESERVED_1                     31, 30
/* [29:20] rw=R/W reset=0x20 */
#define DDRC_ZQCTL1_T_ZQ_RESET_NOP                 29, 20
/* [19:0] rw=R/W reset=0x100 */
#define DDRC_ZQCTL1_T_ZQ_SHORT_INTERVAL_X1024      19,  0

/* ----- 0x0188  ZQCTL2 ----- */
/* ZQ Control Register 2 */
#define DDRC_ZQCTL2                     0x0188
/* [31:1] rw=- reset=0x0 */
#define DDRC_ZQCTL2_RESERVED_1                     31,  1
/* [0:0] rw=R/W1S reset=0x0 */
#define DDRC_ZQCTL2_ZQ_RESET                        0,  0

/* ----- 0x018c  ZQSTAT ----- */
/* ZQ Status Register */
#define DDRC_ZQSTAT                     0x018c
/* [31:1] rw=- reset=0x0 */
#define DDRC_ZQSTAT_RESERVED_1                     31,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_ZQSTAT_ZQ_RESET_BUSY                   0,  0

/* ----- 0x0190  DFITMG0 ----- */
/* DFI Timing Register 0 */
#define DDRC_DFITMG0                    0x0190
/* [31:29] rw=- reset=0x0 */
#define DDRC_DFITMG0_RESERVED_1                    31, 29
/* [28:24] rw=R/W reset=0x7 */
#define DDRC_DFITMG0_DFI_T_CTRL_DELAY              28, 24
/* [23:23] rw=R/W reset=0x0 */
#define DDRC_DFITMG0_DFI_RDDATA_USE_DFI_PHY_CLK    23, 23
/* [22:16] rw=R/W reset=0x2 */
#define DDRC_DFITMG0_DFI_T_RDDATA_EN               22, 16
/* [15:15] rw=R/W reset=0x0 */
#define DDRC_DFITMG0_DFI_WRDATA_USE_DFI_PHY_CLK    15, 15
/* [14:14] rw=- reset=0x0 */
#define DDRC_DFITMG0_RESERVED_2                    14, 14
/* [13:8] rw=R/W reset=0x0 */
#define DDRC_DFITMG0_DFI_TPHY_WRDATA               13,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DFITMG0_RESERVED_3                     7,  6
/* [5:0] rw=R/W reset=0x2 */
#define DDRC_DFITMG0_DFI_TPHY_WRLAT                 5,  0

/* ----- 0x0194  DFITMG1 ----- */
/* DFI Timing Register 1 */
#define DDRC_DFITMG1                    0x0194
/* [31:28] rw=R/W reset=0x0 */
#define DDRC_DFITMG1_DFI_T_CMD_LAT                 31, 28
/* [27:26] rw=- reset=0x0 */
#define DDRC_DFITMG1_RESERVED_1                    27, 26
/* [25:24] rw=R/W reset=0x0 */
#define DDRC_DFITMG1_DFI_T_PARIN_LAT               25, 24
/* [23:21] rw=- reset=0x0 */
#define DDRC_DFITMG1_RESERVED_2                    23, 21
/* [20:16] rw=R/W reset=0x0 */
#define DDRC_DFITMG1_DFI_T_WRDATA_DELAY            20, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_DFITMG1_RESERVED_3                    15, 13
/* [12:8] rw=R/W reset=0x4 */
#define DDRC_DFITMG1_DFI_T_DRAM_CLK_DISABLE        12,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_DFITMG1_RESERVED_4                     7,  5
/* [4:0] rw=R/W reset=0x4 */
#define DDRC_DFITMG1_DFI_T_DRAM_CLK_ENABLE          4,  0

/* ----- 0x0198  DFILPCFG0 ----- */
/* DFI Low Power Configuration Register 0 */
#define DDRC_DFILPCFG0                  0x0198
/* [31:29] rw=- reset=0x0 */
#define DDRC_DFILPCFG0_RESERVED_1                  31, 29
/* [28:24] rw=R/W reset=0x8 */
#define DDRC_DFILPCFG0_DFI_TLP_RESP                28, 24
/* [23:20] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG0_DFI_LP_WAKEUP_DPD           23, 20
/* [19:17] rw=- reset=0x0 */
#define DDRC_DFILPCFG0_RESERVED_2                  19, 17
/* [16:16] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG0_DFI_LP_EN_DPD               16, 16
/* [15:12] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG0_DFI_LP_WAKEUP_SR            15, 12
/* [11:9] rw=- reset=0x0 */
#define DDRC_DFILPCFG0_RESERVED_3                  11,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG0_DFI_LP_EN_SR                 8,  8
/* [7:4] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG0_DFI_LP_WAKEUP_PD             7,  4
/* [3:1] rw=- reset=0x0 */
#define DDRC_DFILPCFG0_RESERVED_4                   3,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG0_DFI_LP_EN_PD                 0,  0

/* ----- 0x019c  DFILPCFG1 ----- */
/* DFI Low Power Configuration Register 1 */
#define DDRC_DFILPCFG1                  0x019c
/* [31:8] rw=- reset=0x0 */
#define DDRC_DFILPCFG1_RESERVED_1                  31,  8
/* [7:4] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG1_DFI_LP_WAKEUP_MPSM           7,  4
/* [3:1] rw=- reset=0x0 */
#define DDRC_DFILPCFG1_RESERVED_2                   3,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DFILPCFG1_DFI_LP_EN_MPSM               0,  0

/* ----- 0x01a0  DFIUPD0 ----- */
/* DFI Update Register 0 */
#define DDRC_DFIUPD0                    0x01a0
/* [31:31] rw=R/W reset=0x0 */
#define DDRC_DFIUPD0_DIS_AUTO_CTRLUPD              31, 31
/* [30:30] rw=R/W reset=0x0 */
#define DDRC_DFIUPD0_DIS_AUTO_CTRLUPD_SRX          30, 30
/* [29:29] rw=R/W reset=0x0 */
#define DDRC_DFIUPD0_CTRLUPD_PRE_SRX               29, 29
/* [28:26] rw=- reset=0x0 */
#define DDRC_DFIUPD0_RESERVED_1                    28, 26
/* [25:16] rw=R/W reset=0x40 */
#define DDRC_DFIUPD0_DFI_T_CTRLUP_MAX              25, 16
/* [15:10] rw=- reset=0x0 */
#define DDRC_DFIUPD0_RESERVED_2                    15, 10
/* [9:0] rw=R/W reset=0x3 */
#define DDRC_DFIUPD0_DFI_T_CTRLUP_MIN               9,  0

/* ----- 0x01a4  DFIUPD1 ----- */
/* DFI Update Register 1 */
#define DDRC_DFIUPD1                    0x01a4
/* [31:24] rw=- reset=0x0 */
#define DDRC_DFIUPD1_RESERVED_1                    31, 24
/* [23:16] rw=R/W reset=0x1 */
#define DDRC_DFIUPD1_DFI_T_CTRLUPD_INTERVAL_MIN_X1024 23, 16
/* [15:8] rw=- reset=0x0 */
#define DDRC_DFIUPD1_RESERVED_2                    15,  8
/* [7:0] rw=R/W reset=0x1 */
#define DDRC_DFIUPD1_DFI_T_CTRLUPD_INTERVAL_MAX_X1024  7,  0

/* ----- 0x01a8  DFIUPD2 ----- */
/* DFI Update Register 2 */
#define DDRC_DFIUPD2                    0x01a8
/* [31:31] rw=R/W reset=0x1 */
#define DDRC_DFIUPD2_DFI_PHYUPD_EN                 31, 31
/* [30:0] rw=- reset=0x0 */
#define DDRC_DFIUPD2_RESERVED_1                    30,  0

/* ----- 0x01b0  DFIMISC ----- */
/* DFI Miscellaneous Control Register */
#define DDRC_DFIMISC                    0x01b0
/* [31:13] rw=- reset=0x0 */
#define DDRC_DFIMISC_RESERVED_1                    31, 13
/* [12:8] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_DFI_FREQUENCY                 12,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_LP_OPTIMIZED_WRITE             7,  7
/* [6:6] rw=R/W reset=0x1 */
#define DDRC_DFIMISC_DIS_DYN_ADR_TRI                6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_DFI_INIT_START                 5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_CTL_IDLE_EN                    4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_SHARE_DFI_DRAM_CLK_DISABLE     3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_DFI_DATA_CS_POLARITY           2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_DFIMISC_PHY_DBI_MODE                   1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_DFIMISC_DFI_INIT_COMPLETE_EN           0,  0

/* ----- 0x01b4  DFITMG2 ----- */
/* DFI Timing Register 2 */
#define DDRC_DFITMG2                    0x01b4
/* [31:15] rw=- reset=0x0 */
#define DDRC_DFITMG2_RESERVED_1                    31, 15
/* [14:8] rw=R/W reset=0x2 */
#define DDRC_DFITMG2_DFI_TPHY_RDCSLAT              14,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DFITMG2_RESERVED_2                     7,  6
/* [5:0] rw=R/W reset=0x2 */
#define DDRC_DFITMG2_DFI_TPHY_WRCSLAT               5,  0

/* ----- 0x01b8  DFITMG3 ----- */
/* DFI Timing Register 3 */
#define DDRC_DFITMG3                    0x01b8
/* [31:5] rw=- reset=0x0 */
#define DDRC_DFITMG3_RESERVED_1                    31,  5
/* [4:0] rw=R/W reset=0x0 */
#define DDRC_DFITMG3_DFI_T_GEARDOWN_DELAY           4,  0

/* ----- 0x01bc  DFISTAT ----- */
/* DFI Status Register */
#define DDRC_DFISTAT                    0x01bc
/* [31:2] rw=- reset=0x0 */
#define DDRC_DFISTAT_RESERVED_1                    31,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_DFISTAT_DFI_LP_ACK                     1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_DFISTAT_DFI_INIT_COMPLETE              0,  0

/* ----- 0x01c0  DBICTL ----- */
/* DM/DBI Control Register */
#define DDRC_DBICTL                     0x01c0
/* [31:3] rw=- reset=0x0 */
#define DDRC_DBICTL_RESERVED_1                     31,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_DBICTL_RD_DBI_EN                       2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_DBICTL_WR_DBI_EN                       1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_DBICTL_DM_EN                           0,  0

/* ----- 0x01c4  DFIPHYMSTR ----- */
/* DFI PHY Master */
#define DDRC_DFIPHYMSTR                 0x01c4
/* [31:24] rw=R/W reset=0x80 */
#define DDRC_DFIPHYMSTR_DFI_PHYMSTR_BLK_REF_X32    31, 24
/* [23:1] rw=- reset=0x0 */
#define DDRC_DFIPHYMSTR_RESERVED_1                 23,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_DFIPHYMSTR_DFI_PHYMSTR_EN              0,  0

/* ----- 0x0200  ADDRMAP0 ----- */
/* Address Map Register 0 */
#define DDRC_ADDRMAP0                   0x0200
/* [31:21] rw=- reset=0x0 */
#define DDRC_ADDRMAP0_RESERVED_1                   31, 21
/* [20:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP0_ADDRMAP_DCH_BIT0             20, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_ADDRMAP0_RESERVED_2                   15, 13
/* [12:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP0_ADDRMAP_CS_BIT1              12,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_ADDRMAP0_RESERVED_3                    7,  5
/* [4:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP0_ADDRMAP_CS_BIT0               4,  0

/* ----- 0x0204  ADDRMAP1 ----- */
/* Address Map Register 1 */
#define DDRC_ADDRMAP1                   0x0204
/* [31:22] rw=- reset=0x0 */
#define DDRC_ADDRMAP1_RESERVED_1                   31, 22
/* [21:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP1_ADDRMAP_BANK_B2              21, 16
/* [15:14] rw=- reset=0x0 */
#define DDRC_ADDRMAP1_RESERVED_2                   15, 14
/* [13:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP1_ADDRMAP_BANK_B1              13,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_ADDRMAP1_RESERVED_3                    7,  6
/* [5:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP1_ADDRMAP_BANK_B0               5,  0

/* ----- 0x0208  ADDRMAP2 ----- */
/* Address Map Register 2 */
#define DDRC_ADDRMAP2                   0x0208
/* [31:28] rw=- reset=0x0 */
#define DDRC_ADDRMAP2_RESERVED_1                   31, 28
/* [27:24] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP2_ADDRMAP_COL_B5               27, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_ADDRMAP2_RESERVED_2                   23, 20
/* [19:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP2_ADDRMAP_COL_B4               19, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_ADDRMAP2_RESERVED_3                   15, 13
/* [12:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP2_ADDRMAP_COL_B3               12,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP2_RESERVED_4                    7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP2_ADDRMAP_COL_B2                3,  0

/* ----- 0x020c  ADDRMAP3 ----- */
/* Address Map Register 3 */
#define DDRC_ADDRMAP3                   0x020c
/* [31:29] rw=- reset=0x0 */
#define DDRC_ADDRMAP3_RESERVED_1                   31, 29
/* [28:24] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP3_ADDRMAP_COL_B9               28, 24
/* [23:21] rw=- reset=0x0 */
#define DDRC_ADDRMAP3_RESERVED_2                   23, 21
/* [20:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP3_ADDRMAP_COL_B8               20, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_ADDRMAP3_RESERVED_3                   15, 13
/* [12:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP3_ADDRMAP_COL_B7               12,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_ADDRMAP3_RESERVED_4                    7,  5
/* [4:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP3_ADDRMAP_COL_B6                4,  0

/* ----- 0x0210  ADDRMAP4 ----- */
/* Address Map Register 4 */
#define DDRC_ADDRMAP4                   0x0210
/* [31:31] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP4_COL_ADDR_SHIFT               31, 31
/* [30:13] rw=- reset=0x0 */
#define DDRC_ADDRMAP4_RESERVED_1                   30, 13
/* [12:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP4_ADDRMAP_COL_B11              12,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_ADDRMAP4_RESERVED_2                    7,  5
/* [4:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP4_ADDRMAP_COL_B10               4,  0

/* ----- 0x0214  ADDRMAP5 ----- */
/* Address Map Register 5 */
#define DDRC_ADDRMAP5                   0x0214
/* [31:28] rw=- reset=0x0 */
#define DDRC_ADDRMAP5_RESERVED_1                   31, 28
/* [27:24] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP5_ADDRMAP_ROW_B11              27, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_ADDRMAP5_RESERVED_2                   23, 20
/* [19:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP5_ADDRMAP_ROW_B2_10            19, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_ADDRMAP5_RESERVED_3                   15, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP5_ADDRMAP_ROW_B1               11,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP5_RESERVED_4                    7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP5_ADDRMAP_ROW_B0                3,  0

/* ----- 0x0218  ADDRMAP6 ----- */
/* Address Map Register 6 */
#define DDRC_ADDRMAP6                   0x0218
/* [31:29] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP6_LPDDR34_3GB_6GB_12GB         31, 29
/* [28:28] rw=- reset=0x0 */
#define DDRC_ADDRMAP6_RESERVED_1                   28, 28
/* [27:24] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP6_ADDRMAP_ROW_B15              27, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_ADDRMAP6_RESERVED_2                   23, 20
/* [19:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP6_ADDRMAP_ROW_B14              19, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_ADDRMAP6_RESERVED_3                   15, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP6_ADDRMAP_ROW_B13              11,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP6_RESERVED_4                    7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP6_ADDRMAP_ROW_B12               3,  0

/* ----- 0x021c  ADDRMAP7 ----- */
/* Address Map Register 7 */
#define DDRC_ADDRMAP7                   0x021c
/* [31:12] rw=- reset=0x0 */
#define DDRC_ADDRMAP7_RESERVED_1                   31, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP7_ADDRMAP_ROW_B17              11,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP7_RESERVED_2                    7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP7_ADDRMAP_ROW_B16               3,  0

/* ----- 0x0220  ADDRMAP8 ----- */
/* Address Map Register 8 */
#define DDRC_ADDRMAP8                   0x0220
/* [31:14] rw=- reset=0x0 */
#define DDRC_ADDRMAP8_RESERVED_1                   31, 14
/* [13:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP8_ADDRMAP_BG_B1                13,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_ADDRMAP8_RESERVED_2                    7,  6
/* [5:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP8_ADDRMAP_BG_B0                 5,  0

/* ----- 0x0224  ADDRMAP9 ----- */
/* Address Map Register 9 */
#define DDRC_ADDRMAP9                   0x0224
/* [31:28] rw=- reset=0x0 */
#define DDRC_ADDRMAP9_RESERVED_1                   31, 28
/* [27:24] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP9_ADDRMAP_ROW_B5               27, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_ADDRMAP9_RESERVED_2                   23, 20
/* [19:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP9_ADDRMAP_ROW_B4               19, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_ADDRMAP9_RESERVED_3                   15, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP9_ADDRMAP_ROW_B3               11,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP9_RESERVED_4                    7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP9_ADDRMAP_ROW_B2                3,  0

/* ----- 0x0228  ADDRMAP10 ----- */
/* Address Map Register 10 */
#define DDRC_ADDRMAP10                  0x0228
/* [31:28] rw=- reset=0x0 */
#define DDRC_ADDRMAP10_RESERVED_1                  31, 28
/* [27:24] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP10_ADDRMAP_ROW_B9              27, 24
/* [23:20] rw=- reset=0x0 */
#define DDRC_ADDRMAP10_RESERVED_2                  23, 20
/* [19:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP10_ADDRMAP_ROW_B8              19, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_ADDRMAP10_RESERVED_3                  15, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP10_ADDRMAP_ROW_B7              11,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP10_RESERVED_4                   7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP10_ADDRMAP_ROW_B6               3,  0

/* ----- 0x022c  ADDRMAP11 ----- */
/* Address Map Register 11 */
#define DDRC_ADDRMAP11                  0x022c
/* [31:21] rw=- reset=0x0 */
#define DDRC_ADDRMAP11_RESERVED_1                  31, 21
/* [20:16] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP11_ADDRMAP_CID_B1              20, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_ADDRMAP11_RESERVED_2                  15, 13
/* [12:8] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP11_ADDRMAP_CID_B0              12,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_ADDRMAP11_RESERVED_3                   7,  4
/* [3:0] rw=R/W reset=0x0 */
#define DDRC_ADDRMAP11_ADDRMAP_ROW_B10              3,  0

/* ----- 0x0240  ODTCFG ----- */
/* ODT Configuration Register */
#define DDRC_ODTCFG                     0x0240
/* [31:28] rw=- reset=0x0 */
#define DDRC_ODTCFG_RESERVED_1                     31, 28
/* [27:24] rw=R/W reset=0x4 */
#define DDRC_ODTCFG_WR_ODT_HOLD                    27, 24
/* [23:21] rw=- reset=0x0 */
#define DDRC_ODTCFG_RESERVED_2                     23, 21
/* [20:16] rw=R/W reset=0x0 */
#define DDRC_ODTCFG_WR_ODT_DELAY                   20, 16
/* [15:12] rw=- reset=0x0 */
#define DDRC_ODTCFG_RESERVED_3                     15, 12
/* [11:8] rw=R/W reset=0x4 */
#define DDRC_ODTCFG_RD_ODT_HOLD                    11,  8
/* [7:7] rw=- reset=0x0 */
#define DDRC_ODTCFG_RESERVED_4                      7,  7
/* [6:2] rw=R/W reset=0x0 */
#define DDRC_ODTCFG_RD_ODT_DELAY                    6,  2
/* [1:0] rw=- reset=0x0 */
#define DDRC_ODTCFG_RESERVED_5                      1,  0

/* ----- 0x0244  ODTMAP ----- */
/* ODT/Rank Map Register */
#define DDRC_ODTMAP                     0x0244
/* 变量位域(按配置实例化): rank3_rd_odt [x:28] rw=R/W reset=0x0 */
/* 变量位域(按配置实例化): rank3_wr_odt [x:24] rw=R/W reset=0x0 */
/* 变量位域(按配置实例化): rank2_rd_odt [x:20] rw=R/W reset=0x0 */
/* 变量位域(按配置实例化): rank2_wr_odt [x:16] rw=R/W reset=0x0 */
/* 变量位域(按配置实例化): rank1_rd_odt [x:12] rw=R/W reset=0x2 */
/* 变量位域(按配置实例化): rank1_wr_odt [x:8] rw=R/W reset=0x2 */
/* 变量位域(按配置实例化): rank0_rd_odt [x:4] rw=R/W reset=0x1 */
/* 变量位域(按配置实例化): rank0_wr_odt [x:0] rw=R/W reset=0x1 */

/* ----- 0x0250  SCHED ----- */
/* Scheduler Control Register */
#define DDRC_SCHED                      0x0250
/* [31:31] rw=R/W reset=0x1 */
#define DDRC_SCHED_OPT_VPRW_SCH                    31, 31
/* [30:24] rw=R/W reset=0x0 */
#define DDRC_SCHED_RDWR_IDLE_GAP                   30, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_SCHED_GO2CRITICAL_HYSTERESIS          23, 16
/* [15:15] rw=R/W reset=0x0 */
#define DDRC_SCHED_LPDDR4_OPT_ACT_TIMING           15, 15
/* 变量位域(按配置实例化): lpr_num_entries [x:8] rw=R/W reset=0x20 */
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_SCHED_AUTOPRE_RMW                      7,  7
/* [6:6] rw=R/W reset=0x0 */
#define DDRC_SCHED_DIS_OPT_NTT_BY_PRE               6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_SCHED_DIS_OPT_NTT_BY_ACT               5,  5
/* [4:4] rw=R/W reset=0x1 */
#define DDRC_SCHED_OPT_WRCAM_FILL_LEVEL             4,  4
/* [3:3] rw=R/W reset=0x1 */
#define DDRC_SCHED_RDWR_SWITCH_POLICY_SEL           3,  3
/* [2:2] rw=R/W reset=0x1 */
#define DDRC_SCHED_PAGECLOSE                        2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_SCHED_PREFER_WRITE                     1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_SCHED_DIS_OPT_WRECC_COLLISION_FLUSH    0,  0

/* ----- 0x0254  SCHED1 ----- */
/* Scheduler Control Register 1 */
#define DDRC_SCHED1                     0x0254
/* [31:31] rw=R/W reset=0x0 */
#define DDRC_SCHED1_OPT_HIT_GT_HPR                 31, 31
/* [30:28] rw=R/W reset=0x0 */
#define DDRC_SCHED1_PAGE_HIT_LIMIT_RD              30, 28
/* [27:27] rw=- reset=0x0 */
#define DDRC_SCHED1_RESERVED_1                     27, 27
/* [26:24] rw=R/W reset=0x0 */
#define DDRC_SCHED1_PAGE_HIT_LIMIT_WR              26, 24
/* [23:23] rw=- reset=0x0 */
#define DDRC_SCHED1_RESERVED_2                     23, 23
/* [22:20] rw=R/W reset=0x0 */
#define DDRC_SCHED1_VISIBLE_WINDOW_LIMIT_RD        22, 20
/* [19:19] rw=- reset=0x0 */
#define DDRC_SCHED1_RESERVED_3                     19, 19
/* [18:16] rw=R/W reset=0x0 */
#define DDRC_SCHED1_VISIBLE_WINDOW_LIMIT_WR        18, 16
/* [15:12] rw=R/W reset=0x2 */
#define DDRC_SCHED1_DELAY_SWITCH_WRITE             15, 12
/* [11:8] rw=- reset=0x0 */
#define DDRC_SCHED1_RESERVED_4                     11,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_SCHED1_PAGECLOSE_TIMER                 7,  0

/* ----- 0x0258  SCHED2 ----- */
/* Scheduler Control Register 2 */
#define DDRC_SCHED2                     0x0258
/* 变量位域(按配置实例化): dealloc_num_bsm_m1 [x:16] rw=R/W reset=0x2 */
/* 变量位域(按配置实例化): dealloc_bsm_thr [x:8] rw=R/W reset=0x1e */
/* [7:3] rw=- reset=0x0 */
#define DDRC_SCHED2_RESERVED_1                      7,  3
/* [2:2] rw=R/W1C reset=0x0 */
#define DDRC_SCHED2_MAX_NUM_UNALLOC_ENTRIES_CLR     2,  2
/* [1:1] rw=R/W1C reset=0x0 */
#define DDRC_SCHED2_MAX_NUM_ALLOC_BSM_CLR           1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_SCHED2_DYN_BSM_MODE                    0,  0

/* ----- 0x025c  PERFHPR1 ----- */
/* High Priority Read CAM Register 1 */
#define DDRC_PERFHPR1                   0x025c
/* [31:24] rw=R/W reset=0xf */
#define DDRC_PERFHPR1_HPR_XACT_RUN_LENGTH          31, 24
/* [23:16] rw=- reset=0x0 */
#define DDRC_PERFHPR1_RESERVED_1                   23, 16
/* [15:0] rw=R/W reset=0x1 */
#define DDRC_PERFHPR1_HPR_MAX_STARVE               15,  0

/* ----- 0x0264  PERFLPR1 ----- */
/* Low Priority Read CAM Register 1 */
#define DDRC_PERFLPR1                   0x0264
/* [31:24] rw=R/W reset=0xf */
#define DDRC_PERFLPR1_LPR_XACT_RUN_LENGTH          31, 24
/* [23:16] rw=- reset=0x0 */
#define DDRC_PERFLPR1_RESERVED_1                   23, 16
/* [15:0] rw=R/W reset=0x7f */
#define DDRC_PERFLPR1_LPR_MAX_STARVE               15,  0

/* ----- 0x026c  PERFWR1 ----- */
/* Write CAM Register 1 */
#define DDRC_PERFWR1                    0x026c
/* [31:24] rw=R/W reset=0xf */
#define DDRC_PERFWR1_W_XACT_RUN_LENGTH             31, 24
/* [23:16] rw=- reset=0x0 */
#define DDRC_PERFWR1_RESERVED_1                    23, 16
/* [15:0] rw=R/W reset=0x7f */
#define DDRC_PERFWR1_W_MAX_STARVE                  15,  0

/* ----- 0x0270  SCHED3 ----- */
/* Scheduler Control Register 3 */
#define DDRC_SCHED3                     0x0270
/* 变量位域(按配置实例化): rd_pghit_num_thresh [x:24] rw=R/W reset=0x4 */
/* 变量位域(按配置实例化): wr_pghit_num_thresh [x:16] rw=R/W reset=0x4 */
/* 变量位域(按配置实例化): wrcam_highthresh [x:8] rw=R/W reset=0x2 */
/* 变量位域(按配置实例化): wrcam_lowthresh [x:0] rw=R/W reset=0x8 */

/* ----- 0x0274  SCHED4 ----- */
/* Scheduler Control Register 4 */
#define DDRC_SCHED4                     0x0274
/* [31:24] rw=R/W reset=0x8 */
#define DDRC_SCHED4_WR_PAGE_EXP_CYCLES             31, 24
/* [23:16] rw=R/W reset=0x40 */
#define DDRC_SCHED4_RD_PAGE_EXP_CYCLES             23, 16
/* [15:8] rw=R/W reset=0x8 */
#define DDRC_SCHED4_WR_ACT_IDLE_GAP                15,  8
/* [7:0] rw=R/W reset=0x10 */
#define DDRC_SCHED4_RD_ACT_IDLE_GAP                 7,  0

/* ----- 0x0278  SCHED5 ----- */
/* Scheduler Control Register 5. */
#define DDRC_SCHED5                     0x0278
/* [31:30] rw=- reset=0x0 */
#define DDRC_SCHED5_RESERVED_1                     31, 30
/* [29:29] rw=R/W reset=0x0 */
#define DDRC_SCHED5_DIS_OPT_VALID_WRECC_CAM_FILL_LEVEL 29, 29
/* [28:28] rw=R/W reset=0x1 */
#define DDRC_SCHED5_DIS_OPT_LOADED_WRECC_CAM_FILL_LE 28, 28
/* 变量位域(按配置实例化): wrecc_cam_highthresh [x:8] rw=R/W reset=0x2 */
/* 变量位域(按配置实例化): wrecc_cam_lowthresh [x:0] rw=R/W reset=0x4 */

/* ----- 0x0280  DQMAP0 ----- */
/* DQ Map Register 0 */
#define DDRC_DQMAP0                     0x0280
/* [31:24] rw=R/W reset=0x0 */
#define DDRC_DQMAP0_DQ_NIBBLE_MAP_12_15            31, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_DQMAP0_DQ_NIBBLE_MAP_8_11             23, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_DQMAP0_DQ_NIBBLE_MAP_4_7              15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DQMAP0_DQ_NIBBLE_MAP_0_3               7,  0

/* ----- 0x0284  DQMAP1 ----- */
/* DQ Map Register 1 */
#define DDRC_DQMAP1                     0x0284
/* [31:24] rw=R/W reset=0x0 */
#define DDRC_DQMAP1_DQ_NIBBLE_MAP_28_31            31, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_DQMAP1_DQ_NIBBLE_MAP_24_27            23, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_DQMAP1_DQ_NIBBLE_MAP_20_23            15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DQMAP1_DQ_NIBBLE_MAP_16_19             7,  0

/* ----- 0x0288  DQMAP2 ----- */
/* DQ Map Register 2 */
#define DDRC_DQMAP2                     0x0288
/* [31:24] rw=R/W reset=0x0 */
#define DDRC_DQMAP2_DQ_NIBBLE_MAP_44_47            31, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_DQMAP2_DQ_NIBBLE_MAP_40_43            23, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_DQMAP2_DQ_NIBBLE_MAP_36_39            15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DQMAP2_DQ_NIBBLE_MAP_32_35             7,  0

/* ----- 0x028c  DQMAP3 ----- */
/* DQ Map Register 3 */
#define DDRC_DQMAP3                     0x028c
/* [31:24] rw=R/W reset=0x0 */
#define DDRC_DQMAP3_DQ_NIBBLE_MAP_60_63            31, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_DQMAP3_DQ_NIBBLE_MAP_56_59            23, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_DQMAP3_DQ_NIBBLE_MAP_52_55            15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DQMAP3_DQ_NIBBLE_MAP_48_51             7,  0

/* ----- 0x0290  DQMAP4 ----- */
/* DQ Map Register 4 */
#define DDRC_DQMAP4                     0x0290
/* [31:24] rw=R/W reset=0x0 */
#define DDRC_DQMAP4_DQ_NIBBLE_MAP_CB_12_15         31, 24
/* [23:16] rw=R/W reset=0x0 */
#define DDRC_DQMAP4_DQ_NIBBLE_MAP_CB_8_11          23, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_DQMAP4_DQ_NIBBLE_MAP_CB_4_7           15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_DQMAP4_DQ_NIBBLE_MAP_CB_0_3            7,  0

/* ----- 0x0294  DQMAP5 ----- */
/* DQ Map Register 5 */
#define DDRC_DQMAP5                     0x0294
/* [31:1] rw=- reset=0x0 */
#define DDRC_DQMAP5_RESERVED_1                     31,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DQMAP5_DIS_DQ_RANK_SWAP                0,  0

/* ----- 0x0300  DBG0 ----- */
/* Debug Register 0 */
#define DDRC_DBG0                       0x0300
/* [31:8] rw=- reset=0x0 */
#define DDRC_DBG0_RESERVED_1                       31,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_DBG0_DIS_MAX_RANK_WR_OPT               7,  7
/* [6:6] rw=R/W reset=0x0 */
#define DDRC_DBG0_DIS_MAX_RANK_RD_OPT               6,  6
/* [5:5] rw=- reset=0x0 */
#define DDRC_DBG0_RESERVED_2                        5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_DBG0_DIS_COLLISION_PAGE_OPT            4,  4
/* [3:3] rw=- reset=0x0 */
#define DDRC_DBG0_RESERVED_3                        3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_DBG0_DIS_ACT_BYPASS                    2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_DBG0_DIS_RD_BYPASS                     1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DBG0_DIS_WC                            0,  0

/* ----- 0x0304  DBG1 ----- */
/* Debug Register 1 */
#define DDRC_DBG1                       0x0304
/* [31:2] rw=- reset=0x0 */
#define DDRC_DBG1_RESERVED_1                       31,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_DBG1_DIS_HIF                           1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_DBG1_DIS_DQ                            0,  0

/* ----- 0x0308  DBGCAM ----- */
/* CAM Debug Register */
#define DDRC_DBGCAM                     0x0308
/* [31:31] rw=R reset=0x0 */
#define DDRC_DBGCAM_DBG_STALL_RD                   31, 31
/* [30:30] rw=R reset=0x0 */
#define DDRC_DBGCAM_DBG_STALL_WR                   30, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_DBGCAM_WR_DATA_PIPELINE_EMPTY         29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_DBGCAM_RD_DATA_PIPELINE_EMPTY         28, 28
/* [27:27] rw=- reset=0x0 */
#define DDRC_DBGCAM_RESERVED_1                     27, 27
/* [26:26] rw=R reset=0x0 */
#define DDRC_DBGCAM_DBG_WR_Q_EMPTY                 26, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_DBGCAM_DBG_RD_Q_EMPTY                 25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_DBGCAM_DBG_STALL                      24, 24
/* 变量位域(按配置实例化): dbg_w_q_depth [x:16] rw=R reset=0x0 */
/* 变量位域(按配置实例化): dbg_lpr_q_depth [x:8] rw=R reset=0x0 */
/* 变量位域(按配置实例化): dbg_hpr_q_depth [x:0] rw=R reset=0x0 */

/* ----- 0x030c  DBGCMD ----- */
/* Command Debug Register */
#define DDRC_DBGCMD                     0x030c
/* [31:20] rw=- reset=0x0 */
#define DDRC_DBGCMD_RESERVED_1                     31, 20
/* [19:19] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK15_REFRESH                 19, 19
/* [18:18] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK14_REFRESH                 18, 18
/* [17:17] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK13_REFRESH                 17, 17
/* [16:16] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK12_REFRESH                 16, 16
/* [15:15] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK11_REFRESH                 15, 15
/* [14:14] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK10_REFRESH                 14, 14
/* [13:13] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK9_REFRESH                  13, 13
/* [12:12] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK8_REFRESH                  12, 12
/* [11:11] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK7_REFRESH                  11, 11
/* [10:10] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK6_REFRESH                  10, 10
/* [9:9] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK5_REFRESH                   9,  9
/* [8:8] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK4_REFRESH                   8,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DBGCMD_RESERVED_2                      7,  6
/* [5:5] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_CTRLUPD                         5,  5
/* [4:4] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_ZQ_CALIB_SHORT                  4,  4
/* [3:3] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK3_REFRESH                   3,  3
/* [2:2] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK2_REFRESH                   2,  2
/* [1:1] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK1_REFRESH                   1,  1
/* [0:0] rw=R/W1S reset=0x0 */
#define DDRC_DBGCMD_RANK0_REFRESH                   0,  0

/* ----- 0x0310  DBGSTAT ----- */
/* Status Debug Register */
#define DDRC_DBGSTAT                    0x0310
/* [31:20] rw=- reset=0x0 */
#define DDRC_DBGSTAT_RESERVED_1                    31, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK15_REFRESH_BUSY           19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK14_REFRESH_BUSY           18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK13_REFRESH_BUSY           17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK12_REFRESH_BUSY           16, 16
/* [15:15] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK11_REFRESH_BUSY           15, 15
/* [14:14] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK10_REFRESH_BUSY           14, 14
/* [13:13] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK9_REFRESH_BUSY            13, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK8_REFRESH_BUSY            12, 12
/* [11:11] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK7_REFRESH_BUSY            11, 11
/* [10:10] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK6_REFRESH_BUSY            10, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK5_REFRESH_BUSY             9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK4_REFRESH_BUSY             8,  8
/* [7:6] rw=- reset=0x0 */
#define DDRC_DBGSTAT_RESERVED_2                     7,  6
/* [5:5] rw=R reset=0x0 */
#define DDRC_DBGSTAT_CTRLUPD_BUSY                   5,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_DBGSTAT_ZQ_CALIB_SHORT_BUSY            4,  4
/* [3:3] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK3_REFRESH_BUSY             3,  3
/* [2:2] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK2_REFRESH_BUSY             2,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK1_REFRESH_BUSY             1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_DBGSTAT_RANK0_REFRESH_BUSY             0,  0

/* ----- 0x0318  DBGCAM1 ----- */
/* CAM Debug Register 1 */
#define DDRC_DBGCAM1                    0x0318
/* 变量位域(按配置实例化): dbg_wrecc_q_depth [x:0] rw=R reset=0x0 */

/* ----- 0x0320  SWCTL ----- */
/* Software Register Programming Control Enable */
#define DDRC_SWCTL                      0x0320
/* [31:1] rw=- reset=0x0 */
#define DDRC_SWCTL_RESERVED_1                      31,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_SWCTL_SW_DONE                          0,  0

/* ----- 0x0324  SWSTAT ----- */
/* Software Register Programming Control Status */
#define DDRC_SWSTAT                     0x0324
/* [31:1] rw=- reset=0x0 */
#define DDRC_SWSTAT_RESERVED_1                     31,  1
/* [0:0] rw=R reset=0x1 */
#define DDRC_SWSTAT_SW_DONE_ACK                     0,  0

/* ----- 0x0328  SWCTLSTATIC ----- */
/* Static Registers Write Enable */
#define DDRC_SWCTLSTATIC                0x0328
/* [31:1] rw=- reset=0x0 */
#define DDRC_SWCTLSTATIC_RESERVED_1                31,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_SWCTLSTATIC_SW_STATIC_UNLOCK           0,  0

/* ----- 0x0330  OCPARCFG0 ----- */
/* On-Chip Parity Configuration Register 0 */
#define DDRC_OCPARCFG0                  0x0330
/* [31:27] rw=- reset=0x0 */
#define DDRC_OCPARCFG0_RESERVED_1                  31, 27
/* [26:26] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_RADDR_ERR_INTR_FORCE    26, 26
/* [25:25] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_WADDR_ERR_INTR_FORCE    25, 25
/* [24:24] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_RADDR_ERR_INTR_CLR      24, 24
/* [23:23] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_RADDR_ERR_INTR_EN       23, 23
/* [22:22] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_WADDR_ERR_INTR_CLR      22, 22
/* [21:21] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_WADDR_ERR_INTR_EN       21, 21
/* [20:20] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_ADDR_SLVERR_EN          20, 20
/* [19:16] rw=- reset=0x0 */
#define DDRC_OCPARCFG0_RESERVED_2                  19, 16
/* [15:15] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_RDATA_ERR_INTR_FORCE    15, 15
/* [14:14] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_RDATA_ERR_INTR_CLR      14, 14
/* [13:13] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_RDATA_ERR_INTR_EN       13, 13
/* [12:12] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_RDATA_SLVERR_EN         12, 12
/* [11:9] rw=- reset=0x0 */
#define DDRC_OCPARCFG0_RESERVED_3                  11,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG0_PAR_WDATA_AXI_CHECK_BYPASS_EN  8,  8
/* [7:7] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_WDATA_ERR_INTR_FORCE     7,  7
/* [6:6] rw=R/W1C reset=0x0 */
#define DDRC_OCPARCFG0_PAR_WDATA_ERR_INTR_CLR       6,  6
/* [5:5] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_WDATA_SLVERR_EN          5,  5
/* [4:4] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_PAR_WDATA_ERR_INTR_EN        4,  4
/* [3:2] rw=- reset=0x0 */
#define DDRC_OCPARCFG0_RESERVED_4                   3,  2
/* [1:1] rw=R/W reset=0x1 */
#define DDRC_OCPARCFG0_OC_PARITY_TYPE               1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG0_OC_PARITY_EN                 0,  0

/* ----- 0x0334  OCPARCFG1 ----- */
/* On-Chip Parity Configuration Register 1 */
#define DDRC_OCPARCFG1                  0x0334
/* 变量位域(按配置实例化): par_poison_byte_num [x:16] rw=R/W reset=0x0 */
/* [15:12] rw=- reset=0x0 */
#define DDRC_OCPARCFG1_RESERVED_1                  15, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG1_PAR_POISON_LOC_WR_PORT      11,  8
/* [7:4] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG1_PAR_POISON_LOC_RD_PORT       7,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG1_PAR_POISON_LOC_RD_IECC_TYPE  3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG1_PAR_POISON_LOC_RD_DFI        2,  2
/* [1:1] rw=- reset=0x0 */
#define DDRC_OCPARCFG1_RESERVED_2                   1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_OCPARCFG1_PAR_POISON_EN                0,  0

/* ----- 0x0338  OCPARSTAT0 ----- */
/* On-Chip Parity Status Register 0 */
#define DDRC_OCPARSTAT0                 0x0338
/* [31:31] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_15      31, 31
/* [30:30] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_14      30, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_13      29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_12      28, 28
/* [27:27] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_11      27, 27
/* [26:26] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_10      26, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_9       25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_8       24, 24
/* [23:23] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_7       23, 23
/* [22:22] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_6       22, 22
/* [21:21] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_5       21, 21
/* [20:20] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_4       20, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_3       19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_2       18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_1       17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_RADDR_ERR_INTR_0       16, 16
/* [15:15] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_15      15, 15
/* [14:14] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_14      14, 14
/* [13:13] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_13      13, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_12      12, 12
/* [11:11] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_11      11, 11
/* [10:10] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_10      10, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_9        9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_8        8,  8
/* [7:7] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_7        7,  7
/* [6:6] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_6        6,  6
/* [5:5] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_5        5,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_4        4,  4
/* [3:3] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_3        3,  3
/* [2:2] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_2        2,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_1        1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_OCPARSTAT0_PAR_WADDR_ERR_INTR_0        0,  0

/* ----- 0x033c  OCPARSTAT1 ----- */
/* On-Chip Parity Status Register 1 */
#define DDRC_OCPARSTAT1                 0x033c
/* [31:31] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_15      31, 31
/* [30:30] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_14      30, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_13      29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_12      28, 28
/* [27:27] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_11      27, 27
/* [26:26] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_10      26, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_9       25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_8       24, 24
/* [23:23] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_7       23, 23
/* [22:22] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_6       22, 22
/* [21:21] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_5       21, 21
/* [20:20] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_4       20, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_3       19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_2       18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_1       17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_RDATA_ERR_INTR_0       16, 16
/* [15:15] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_15   15, 15
/* [14:14] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_14   14, 14
/* [13:13] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_13   13, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_12   12, 12
/* [11:11] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_11   11, 11
/* [10:10] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_10   10, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_9     9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_8     8,  8
/* [7:7] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_7     7,  7
/* [6:6] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_6     6,  6
/* [5:5] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_5     5,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_4     4,  4
/* [3:3] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_3     3,  3
/* [2:2] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_2     2,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_1     1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_OCPARSTAT1_PAR_WDATA_IN_ERR_INTR_0     0,  0

/* ----- 0x0340  OCPARSTAT2 ----- */
/* On-Chip Parity Status Register 2 */
#define DDRC_OCPARSTAT2                 0x0340
/* [31:12] rw=- reset=0x0 */
#define DDRC_OCPARSTAT2_RESERVED_1                 31, 12
/* [11:8] rw=R reset=0x0 */
#define DDRC_OCPARSTAT2_PAR_RDATA_LOG_PORT_NUM     11,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_OCPARSTAT2_RESERVED_2                  7,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_OCPARSTAT2_PAR_RDATA_IN_ERR_ECC_INTR   4,  4
/* 变量位域(按配置实例化): par_wdata_out_err_intr [x:0] rw=R reset=0x0 */

/* ----- 0x0344  OCPARSTAT3 ----- */
/* On-Chip Parity Read Data Log Register 0 */
#define DDRC_OCPARSTAT3                 0x0344
/* 变量位域(按配置实例化): par_rdata_log_byte_num [x:0] rw=R reset=0x0 */

/* ----- 0x0348  OCPARSTAT4 ----- */
/* On-Chip Parity Write Address Log Register 0 */
#define DDRC_OCPARSTAT4                 0x0348
/* [31:0] rw=R reset=0x0 */
#define DDRC_OCPARSTAT4_PAR_WADDR_LOG_LOW          31,  0

/* ----- 0x034c  OCPARSTAT5 ----- */
/* On-Chip Parity Write Address Log Register 1 */
#define DDRC_OCPARSTAT5                 0x034c
/* [31:28] rw=R reset=0x0 */
#define DDRC_OCPARSTAT5_PAR_WADDR_LOG_PORT_NUM     31, 28
/* 变量位域(按配置实例化): par_waddr_log_high [x:0] rw=R reset=0x0 */

/* ----- 0x0350  OCPARSTAT6 ----- */
/* On-Chip Parity Read Address Log Register 0 */
#define DDRC_OCPARSTAT6                 0x0350
/* [31:0] rw=R reset=0x0 */
#define DDRC_OCPARSTAT6_PAR_RADDR_LOG_LOW          31,  0

/* ----- 0x0354  OCPARSTAT7 ----- */
/* On-Chip Parity Read Address Log Register 1 */
#define DDRC_OCPARSTAT7                 0x0354
/* [31:28] rw=R reset=0x0 */
#define DDRC_OCPARSTAT7_PAR_RADDR_LOG_PORT_NUM     31, 28
/* 变量位域(按配置实例化): par_raddr_log_high [x:0] rw=R reset=0x0 */

/* ----- 0x0358  OCECCCFG0 ----- */
/* On-Chip ECC Configuration Register 0 */
#define DDRC_OCECCCFG0                  0x0358
/* [31:14] rw=- reset=0x0 */
#define DDRC_OCECCCFG0_RESERVED_1                  31, 14
/* [13:13] rw=R/W reset=0x1 */
#define DDRC_OCECCCFG0_OCECC_RDATA_SLVERR_EN       13, 13
/* [12:8] rw=- reset=0x0 */
#define DDRC_OCECCCFG0_RESERVED_2                  12,  8
/* [7:7] rw=R/W1C reset=0x0 */
#define DDRC_OCECCCFG0_OCECC_UNCORRECTED_ERR_INTR_FORCE  7,  7
/* [6:6] rw=R/W1C reset=0x0 */
#define DDRC_OCECCCFG0_OCECC_UNCORRECTED_ERR_INTR_CLR  6,  6
/* [5:5] rw=R/W reset=0x1 */
#define DDRC_OCECCCFG0_OCECC_WDATA_SLVERR_EN        5,  5
/* [4:4] rw=R/W reset=0x1 */
#define DDRC_OCECCCFG0_OCECC_UNCORRECTED_ERR_INTR_EN  4,  4
/* [3:1] rw=- reset=0x0 */
#define DDRC_OCECCCFG0_RESERVED_3                   3,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG0_OCECC_EN                     0,  0

/* ----- 0x035c  OCECCCFG1 ----- */
/* On-Chip ECC Configuration Register 1 */
#define DDRC_OCECCCFG1                  0x035c
/* [31:24] rw=- reset=0x0 */
#define DDRC_OCECCCFG1_RESERVED_1                  31, 24
/* [23:23] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_PGEN_MR_ECC    23, 23
/* [22:22] rw=- reset=0x0 */
#define DDRC_OCECCCFG1_RESERVED_2                  22, 22
/* [21:21] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_PGEN_RD        21, 21
/* [20:19] rw=R/W reset=0x2 */
#define DDRC_OCECCCFG1_OCECC_POISON_ECC_CORR_UNCORR 20, 19
/* [18:18] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EGEN_XPI_RD_0  18, 18
/* [17:13] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EGEN_MR_RD_1_BYTE 17, 13
/* [12:12] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EGEN_MR_RD_1   12, 12
/* [11:8] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_PORT_NUM       11,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EGEN_XPI_RD_OUT  7,  7
/* [6:2] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EGEN_MR_RD_0_BYTE  6,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EGEN_MR_RD_0    1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_OCECCCFG1_OCECC_POISON_EN              0,  0

/* ----- 0x0360  OCECCSTAT0 ----- */
/* On-Chip ECC Status Register 0 */
#define DDRC_OCECCSTAT0                 0x0360
/* [31:21] rw=- reset=0x0 */
#define DDRC_OCECCSTAT0_RESERVED_1                 31, 21
/* [20:20] rw=R reset=0x0 */
#define DDRC_OCECCSTAT0_PAR_ERR_RD                 20, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_OCECCSTAT0_PAR_ERR_MR_ECC             19, 19
/* [18:18] rw=- reset=0x0 */
#define DDRC_OCECCSTAT0_RESERVED_2                 18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_OCECCSTAT0_OCECC_ERR_DDRC_MR_RD       17, 17
/* [16:1] rw=- reset=0x0 */
#define DDRC_OCECCSTAT0_RESERVED_3                 16,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_OCECCSTAT0_OCECC_UNCORRECTED_ERR       0,  0

/* ----- 0x0364  OCECCSTAT1 ----- */
/* On-Chip ECC Status Register 1 */
#define DDRC_OCECCSTAT1                 0x0364
/* [31:31] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_15        31, 31
/* [30:30] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_14        30, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_13        29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_12        28, 28
/* [27:27] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_11        27, 27
/* [26:26] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_10        26, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_9         25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_8         24, 24
/* [23:23] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_7         23, 23
/* [22:22] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_6         22, 22
/* [21:21] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_5         21, 21
/* [20:20] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_4         20, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_3         19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_2         18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_1         17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_RD_0         16, 16
/* [15:15] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_15     15, 15
/* [14:14] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_14     14, 14
/* [13:13] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_13     13, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_12     12, 12
/* [11:11] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_11     11, 11
/* [10:10] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_10     10, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_9       9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_8       8,  8
/* [7:7] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_7       7,  7
/* [6:6] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_6       6,  6
/* [5:5] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_5       5,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_4       4,  4
/* [3:3] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_3       3,  3
/* [2:2] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_2       2,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_1       1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_OCECCSTAT1_OCECC_ERR_XPI_WR_IN_0       0,  0

/* ----- 0x0368  OCECCSTAT2 ----- */
/* On-Chip ECC Status Register 2 */
#define DDRC_OCECCSTAT2                 0x0368
/* [31:0] rw=R reset=0x0 */
#define DDRC_OCECCSTAT2_OCECC_ERR_DDRC_MR_RD_BYTE_NUM 31,  0

/* ----- 0x036c  POISONCFG ----- */
/* AXI Poison Configuration Register. Common for all AXI ports. */
#define DDRC_POISONCFG                  0x036c
/* [31:25] rw=- reset=0x0 */
#define DDRC_POISONCFG_RESERVED_1                  31, 25
/* [24:24] rw=R/W1C reset=0x0 */
#define DDRC_POISONCFG_RD_POISON_INTR_CLR          24, 24
/* [23:21] rw=- reset=0x0 */
#define DDRC_POISONCFG_RESERVED_2                  23, 21
/* [20:20] rw=R/W reset=0x1 */
#define DDRC_POISONCFG_RD_POISON_INTR_EN           20, 20
/* [19:17] rw=- reset=0x0 */
#define DDRC_POISONCFG_RESERVED_3                  19, 17
/* [16:16] rw=R/W reset=0x1 */
#define DDRC_POISONCFG_RD_POISON_SLVERR_EN         16, 16
/* [15:9] rw=- reset=0x0 */
#define DDRC_POISONCFG_RESERVED_4                  15,  9
/* [8:8] rw=R/W1C reset=0x0 */
#define DDRC_POISONCFG_WR_POISON_INTR_CLR           8,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_POISONCFG_RESERVED_5                   7,  5
/* [4:4] rw=R/W reset=0x1 */
#define DDRC_POISONCFG_WR_POISON_INTR_EN            4,  4
/* [3:1] rw=- reset=0x0 */
#define DDRC_POISONCFG_RESERVED_6                   3,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_POISONCFG_WR_POISON_SLVERR_EN          0,  0

/* ----- 0x0370  POISONSTAT ----- */
/* AXI Poison Status Register */
#define DDRC_POISONSTAT                 0x0370
/* [31:31] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_15          31, 31
/* [30:30] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_14          30, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_13          29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_12          28, 28
/* [27:27] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_11          27, 27
/* [26:26] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_10          26, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_9           25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_8           24, 24
/* [23:23] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_7           23, 23
/* [22:22] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_6           22, 22
/* [21:21] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_5           21, 21
/* [20:20] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_4           20, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_3           19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_2           18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_1           17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_POISONSTAT_RD_POISON_INTR_0           16, 16
/* [15:15] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_15          15, 15
/* [14:14] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_14          14, 14
/* [13:13] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_13          13, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_12          12, 12
/* [11:11] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_11          11, 11
/* [10:10] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_10          10, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_9            9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_8            8,  8
/* [7:7] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_7            7,  7
/* [6:6] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_6            6,  6
/* [5:5] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_5            5,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_4            4,  4
/* [3:3] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_3            3,  3
/* [2:2] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_2            2,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_1            1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_POISONSTAT_WR_POISON_INTR_0            0,  0

/* ----- 0x0374  ADVECCINDEX ----- */
/* Advanced ECC Index Register */
#define DDRC_ADVECCINDEX                0x0374
/* [31:9] rw=- reset=0x0 */
#define DDRC_ADVECCINDEX_RESERVED_1                31,  9
/* [8:5] rw=R/W reset=0x0 */
#define DDRC_ADVECCINDEX_ECC_POISON_BEATS_SEL       8,  5
/* [4:3] rw=R/W reset=0x0 */
#define DDRC_ADVECCINDEX_ECC_ERR_SYMBOL_SEL         4,  3
/* [2:0] rw=R/W reset=0x0 */
#define DDRC_ADVECCINDEX_ECC_SYNDROME_SEL           2,  0

/* ----- 0x0378  ADVECCSTAT ----- */
/* Advanced ECC Status Register */
#define DDRC_ADVECCSTAT                 0x0378
/* [31:24] rw=- reset=0x0 */
#define DDRC_ADVECCSTAT_RESERVED_1                 31, 24
/* [23:16] rw=R reset=0x0 */
#define DDRC_ADVECCSTAT_ADVECC_ERR_SYMBOL_BITS     23, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_ADVECCSTAT_RESERVED_2                 15, 11
/* [10:5] rw=R reset=0x0 */
#define DDRC_ADVECCSTAT_ADVECC_ERR_SYMBOL_POS      10,  5
/* [4:2] rw=R reset=0x0 */
#define DDRC_ADVECCSTAT_ADVECC_NUM_ERR_SYMBOL       4,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_ADVECCSTAT_ADVECC_UNCORRECTED_ERR      1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_ADVECCSTAT_ADVECC_CORRECTED_ERR        0,  0

/* ----- 0x037c  ECCPOISONPAT0 ----- */
/* ECC Poison Pattern 0 Register */
#define DDRC_ECCPOISONPAT0              0x037c
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONPAT0_ECC_POISON_DATA_31_0    31,  0

/* ----- 0x0380  ECCPOISONPAT1 ----- */
/* ECC Poison Pattern 1 Register */
#define DDRC_ECCPOISONPAT1              0x0380
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONPAT1_ECC_POISON_DATA_63_32   31,  0

/* ----- 0x0384  ECCPOISONPAT2 ----- */
/* ECC Poison Pattern 2 Register */
#define DDRC_ECCPOISONPAT2              0x0384
/* [31:16] rw=- reset=0x0 */
#define DDRC_ECCPOISONPAT2_RESERVED_1              31, 16
/* [15:8] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONPAT2_ECC_POISON_DATA_79_72   15,  8
/* [7:0] rw=R/W reset=0x0 */
#define DDRC_ECCPOISONPAT2_ECC_POISON_DATA_71_64    7,  0

/* ----- 0x0388  ECCAPSTAT ----- */
/* Address protection within ECC Status Register */
#define DDRC_ECCAPSTAT                  0x0388
/* [31:1] rw=- reset=0x0 */
#define DDRC_ECCAPSTAT_RESERVED_1                  31,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_ECCAPSTAT_ECC_AP_ERR                   0,  0

/* ----- 0x03a0  CAPARPOISONCTL ----- */
/* CA parity poison control Register */
#define DDRC_CAPARPOISONCTL             0x03a0
/* [31:10] rw=- reset=0x0 */
#define DDRC_CAPARPOISONCTL_RESERVED_1             31, 10
/* [9:8] rw=R/W reset=0x0 */
#define DDRC_CAPARPOISONCTL_CAPAR_POISON_CMDTYPE    9,  8
/* [7:1] rw=- reset=0x0 */
#define DDRC_CAPARPOISONCTL_RESERVED_2              7,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_CAPARPOISONCTL_CAPAR_POISON_INJECT_EN  0,  0

/* ----- 0x03a4  CAPARPOISONSTAT ----- */
/* CA parity poison status Register */
#define DDRC_CAPARPOISONSTAT            0x03a4
/* [31:1] rw=- reset=0x0 */
#define DDRC_CAPARPOISONSTAT_RESERVED_1            31,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_CAPARPOISONSTAT_CAPAR_POISON_COMPLETE  0,  0

/* ----- 0x03b0  DYNBSMSTAT ----- */
/* Dynamic BSM Status Register */
#define DDRC_DYNBSMSTAT                 0x03b0
/* 变量位域(按配置实例化): max_num_unalloc_entries [x:16] rw=R reset=0x0 */
/* 变量位域(按配置实例化): max_num_alloc_bsm [x:8] rw=R reset=0x0 */
/* 变量位域(按配置实例化): num_alloc_bsm [x:0] rw=R reset=0x0 */

/* ----- 0x03b8  CRCPARCTL3 ----- */
/* CRC Parity Control Register 3 */
#define DDRC_CRCPARCTL3                 0x03b8
/* [31:16] rw=- reset=0x0 */
#define DDRC_CRCPARCTL3_RESERVED_1                 31, 16
/* [15:0] rw=R/W reset=0xffff */
#define DDRC_CRCPARCTL3_DFI_ALERT_ERR_MAX_REACHED_TH 15,  0

/* ----- 0x03c0  REGPARCFG ----- */
/* Register Parity Configuration Register (Note that all fields must be programmed with */
#define DDRC_REGPARCFG                  0x03c0
/* [31:9] rw=- reset=0x0 */
#define DDRC_REGPARCFG_RESERVED_1                  31,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_REGPARCFG_REG_PAR_POISON_EN            8,  8
/* [7:4] rw=- reset=0x0 */
#define DDRC_REGPARCFG_RESERVED_2                   7,  4
/* [3:3] rw=R/W1C reset=0x0 */
#define DDRC_REGPARCFG_REG_PAR_ERR_INTR_FORCE       3,  3
/* [2:2] rw=R/W1C reset=0x0 */
#define DDRC_REGPARCFG_REG_PAR_ERR_INTR_CLR         2,  2
/* [1:1] rw=R/W reset=0x1 */
#define DDRC_REGPARCFG_REG_PAR_ERR_INTR_EN          1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_REGPARCFG_REG_PAR_EN                   0,  0

/* ----- 0x03c4  REGPARSTAT ----- */
/* Register Parity Status Register */
#define DDRC_REGPARSTAT                 0x03c4
/* [31:1] rw=- reset=0x0 */
#define DDRC_REGPARSTAT_RESERVED_1                 31,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_REGPARSTAT_REG_PAR_ERR_INTR            0,  0

/* ----- 0x03d0  RCDINIT1 ----- */
/* Control Word setting Register RCDINIT1 */
#define DDRC_RCDINIT1                   0x03d0
/* [31:29] rw=- reset=0x0 */
#define DDRC_RCDINIT1_RESERVED_1                   31, 29
/* [28:16] rw=R/W reset=0x0 */
#define DDRC_RCDINIT1_CTRL_WORD_2                  28, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_RCDINIT1_RESERVED_2                   15, 13
/* [12:0] rw=R/W reset=0x0 */
#define DDRC_RCDINIT1_CTRL_WORD_1                  12,  0

/* ----- 0x03d4  RCDINIT2 ----- */
/* Control Word setting Register RCDINIT2. */
#define DDRC_RCDINIT2                   0x03d4
/* [31:29] rw=- reset=0x0 */
#define DDRC_RCDINIT2_RESERVED_1                   31, 29
/* [28:16] rw=R/W reset=0x0 */
#define DDRC_RCDINIT2_CTRL_WORD_4                  28, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_RCDINIT2_RESERVED_2                   15, 13
/* [12:0] rw=R/W reset=0x0 */
#define DDRC_RCDINIT2_CTRL_WORD_3                  12,  0

/* ----- 0x03d8  RCDINIT3 ----- */
/* Control Word setting Register RCDINIT3 */
#define DDRC_RCDINIT3                   0x03d8
/* [31:29] rw=- reset=0x0 */
#define DDRC_RCDINIT3_RESERVED_1                   31, 29
/* [28:16] rw=R/W reset=0x0 */
#define DDRC_RCDINIT3_CTRL_WORD_6                  28, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_RCDINIT3_RESERVED_2                   15, 13
/* [12:0] rw=R/W reset=0x0 */
#define DDRC_RCDINIT3_CTRL_WORD_5                  12,  0

/* ----- 0x03dc  RCDINIT4 ----- */
/* Control Word setting Register RCDINIT4 */
#define DDRC_RCDINIT4                   0x03dc
/* [31:29] rw=- reset=0x0 */
#define DDRC_RCDINIT4_RESERVED_1                   31, 29
/* [28:16] rw=R/W reset=0x0 */
#define DDRC_RCDINIT4_CTRL_WORD_8                  28, 16
/* [15:13] rw=- reset=0x0 */
#define DDRC_RCDINIT4_RESERVED_2                   15, 13
/* [12:0] rw=R/W reset=0x0 */
#define DDRC_RCDINIT4_CTRL_WORD_7                  12,  0

/* ----- 0x03e0  OCCAPCFG ----- */
/* On-Chip command/Address Protection Configuration Register */
#define DDRC_OCCAPCFG                   0x03e0
/* [31:28] rw=- reset=0x0 */
#define DDRC_OCCAPCFG_RESERVED_1                   31, 28
/* [27:27] rw=R/W reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_ARB_RAQ_POISON_EN      27, 27
/* [26:26] rw=R/W reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_ARB_CMP_POISON_ERR_INJ 26, 26
/* [25:25] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_ARB_CMP_POISON_PARALLEL 25, 25
/* [24:24] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_ARB_CMP_POISON_SEQ     24, 24
/* [23:19] rw=- reset=0x0 */
#define DDRC_OCCAPCFG_RESERVED_2                   23, 19
/* [18:18] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_ARB_INTR_FORCE         18, 18
/* [17:17] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_ARB_INTR_CLR           17, 17
/* [16:16] rw=R/W reset=0x1 */
#define DDRC_OCCAPCFG_OCCAP_ARB_INTR_EN            16, 16
/* [15:1] rw=- reset=0x0 */
#define DDRC_OCCAPCFG_RESERVED_3                   15,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_OCCAPCFG_OCCAP_EN                      0,  0

/* ----- 0x03e4  OCCAPSTAT ----- */
/* On-Chip command/Address Protection Status Register */
#define DDRC_OCCAPSTAT                  0x03e4
/* [31:26] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT_RESERVED_1                  31, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT_OCCAP_ARB_CMP_POISON_PARALLEL_E 25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT_OCCAP_ARB_CMP_POISON_SEQ_ERR 24, 24
/* [23:18] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT_RESERVED_2                  23, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT_OCCAP_ARB_CMP_POISON_COMPLETE 17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT_OCCAP_ARB_ERR_INTR          16, 16
/* [15:0] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT_RESERVED_3                  15,  0

/* ----- 0x03e8  OCCAPCFG1 ----- */
/* On-Chip command/Address Protection Configuration Register 1 */
#define DDRC_OCCAPCFG1                  0x03e8
/* [31:27] rw=- reset=0x0 */
#define DDRC_OCCAPCFG1_RESERVED_1                  31, 27
/* [26:26] rw=R/W reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_CTRL_POISON_ERR_INJ 26, 26
/* [25:25] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_CTRL_POISON_PARALLEL 25, 25
/* [24:24] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_CTRL_POISON_SEQ  24, 24
/* [23:19] rw=- reset=0x0 */
#define DDRC_OCCAPCFG1_RESERVED_2                  23, 19
/* [18:18] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_CTRL_INTR_FORCE  18, 18
/* [17:17] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_CTRL_INTR_CLR    17, 17
/* [16:16] rw=R/W reset=0x1 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_CTRL_INTR_EN     16, 16
/* [15:11] rw=- reset=0x0 */
#define DDRC_OCCAPCFG1_RESERVED_3                  15, 11
/* [10:10] rw=R/W reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_DATA_POISON_ERR_INJ 10, 10
/* [9:9] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_DATA_POISON_PARALLEL  9,  9
/* [8:8] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_DATA_POISON_SEQ   8,  8
/* [7:3] rw=- reset=0x0 */
#define DDRC_OCCAPCFG1_RESERVED_4                   7,  3
/* [2:2] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_DATA_INTR_FORCE   2,  2
/* [1:1] rw=R/W1C reset=0x0 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_DATA_INTR_CLR     1,  1
/* [0:0] rw=R/W reset=0x1 */
#define DDRC_OCCAPCFG1_OCCAP_DDRC_DATA_INTR_EN      0,  0

/* ----- 0x03ec  OCCAPSTAT1 ----- */
/* On-Chip command/Address Protection Status Register 1 */
#define DDRC_OCCAPSTAT1                 0x03ec
/* [31:26] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT1_RESERVED_1                 31, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_CTRL_POISON_PARALLEL_E 25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_CTRL_POISON_SEQ_ERR 24, 24
/* [23:18] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT1_RESERVED_2                 23, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_CTRL_POISON_COMPLETE 17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_CTRL_ERR_INTR   16, 16
/* [15:10] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT1_RESERVED_3                 15, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_DATA_POISON_PARALLEL_  9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_DATA_POISON_SEQ_ERR  8,  8
/* [7:2] rw=- reset=0x0 */
#define DDRC_OCCAPSTAT1_RESERVED_4                  7,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_DATA_POISON_COMPLET  1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_OCCAPSTAT1_OCCAP_DDRC_DATA_ERR_INTR    0,  0

/* ----- 0x03f0  DERATESTAT ----- */
/* Temperature Derate Status Register */
#define DDRC_DERATESTAT                 0x03f0
/* [31:1] rw=- reset=0x0 */
#define DDRC_DERATESTAT_RESERVED_1                 31,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_DERATESTAT_DERATE_TEMP_LIMIT_INTR      0,  0

/* ----- 0x03fc  PSTAT ----- */
/* Port Status Register */
#define DDRC_PSTAT                      0x03fc
/* [31:31] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_15                 31, 31
/* [30:30] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_14                 30, 30
/* [29:29] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_13                 29, 29
/* [28:28] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_12                 28, 28
/* [27:27] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_11                 27, 27
/* [26:26] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_10                 26, 26
/* [25:25] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_9                  25, 25
/* [24:24] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_8                  24, 24
/* [23:23] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_7                  23, 23
/* [22:22] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_6                  22, 22
/* [21:21] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_5                  21, 21
/* [20:20] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_4                  20, 20
/* [19:19] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_3                  19, 19
/* [18:18] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_2                  18, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_1                  17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_PSTAT_WR_PORT_BUSY_0                  16, 16
/* [15:15] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_15                 15, 15
/* [14:14] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_14                 14, 14
/* [13:13] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_13                 13, 13
/* [12:12] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_12                 12, 12
/* [11:11] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_11                 11, 11
/* [10:10] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_10                 10, 10
/* [9:9] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_9                   9,  9
/* [8:8] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_8                   8,  8
/* [7:7] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_7                   7,  7
/* [6:6] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_6                   6,  6
/* [5:5] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_5                   5,  5
/* [4:4] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_4                   4,  4
/* [3:3] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_3                   3,  3
/* [2:2] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_2                   2,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_1                   1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_PSTAT_RD_PORT_BUSY_0                   0,  0

/* ----- 0x0400  PCCFG ----- */
/* Port Common Configuration Register */
#define DDRC_PCCFG                      0x0400
/* [31:14] rw=- reset=0x0 */
#define DDRC_PCCFG_RESERVED_1                      31, 14
/* [13:12] rw=R/W reset=0x0 */
#define DDRC_PCCFG_DCH_DENSITY_RATIO               13, 12
/* [11:9] rw=- reset=0x0 */
#define DDRC_PCCFG_RESERVED_2                      11,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_PCCFG_BL_EXP_MODE                      8,  8
/* [7:5] rw=- reset=0x0 */
#define DDRC_PCCFG_RESERVED_3                       7,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_PCCFG_PAGEMATCH_LIMIT                  4,  4
/* [3:1] rw=- reset=0x0 */
#define DDRC_PCCFG_RESERVED_4                       3,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_PCCFG_GO2CRITICAL_EN                   0,  0

/* ----- PCFGR_n（阵列寄存器） ----- */
/* offset = 0x404 + 0xb0*n，按实例展开 */

/* ----- PCFGW_n（阵列寄存器） ----- */
/* offset = 0x408 + 0xb0*n，按实例展开 */

/* ----- PCFGC_n（阵列寄存器） ----- */
/* offset = 0x40c + 0xb0*n，按实例展开 */

/* ----- PCFGIDMASKCH（阵列寄存器） ----- */
/* offset = 0x410 + 0xb0*n +0x8*m，按实例展开 */

/* ----- PCFGIDVALUECH（阵列寄存器） ----- */
/* offset = 0x414 + 0xb0*n +0x8*m，按实例展开 */

/* ----- PCTRL_n（阵列寄存器） ----- */
/* offset = 0x490 + 0xb0*n，按实例展开 */
#define DDRC_PCTRL(n)                   (0x0490 + 0xb0 * (n))
#define DDRC_PCTRL_PORT_EN              0, 0

/* ----- PCFGQOS0_n（阵列寄存器） ----- */
/* offset = 0x494 + 0xb0*n，按实例展开 */

/* ----- PCFGQOS1_n（阵列寄存器） ----- */
/* offset = 0x498 + 0xb0*n，按实例展开 */

/* ----- PCFGWQOS0_n（阵列寄存器） ----- */
/* offset = 0x49c + 0xb0*n，按实例展开 */

/* ----- PCFGWQOS1_n（阵列寄存器） ----- */
/* offset = 0x4a0 + 0xb0*n，按实例展开 */

/* ----- SARBASEn（阵列寄存器） ----- */
/* offset = 0xf04 + 0x8*n，按实例展开 */

/* ----- SARSIZEn（阵列寄存器） ----- */
/* offset = 0xf08 + 0x8*n，按实例展开 */

/* ----- 0x0f24  SBRCTL ----- */
/* Scrubber Control Register */
#define DDRC_SBRCTL                     0x0f24
/* 变量位域(按配置实例化): scrub_interval [x:8] rw=R/W reset=0xff */
/* [7:7] rw=- reset=0x0 */
#define DDRC_SBRCTL_RESERVED_1                      7,  7
/* [6:4] rw=R/W reset=0x1 */
#define DDRC_SBRCTL_SCRUB_BURST                     6,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_SBRCTL_SCRUB_EN_DCH1                   3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_SBRCTL_SCRUB_MODE                      2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_SBRCTL_SCRUB_DURING_LOWPOWER           1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_SBRCTL_SCRUB_EN                        0,  0

/* ----- 0x0f28  SBRSTAT ----- */
/* Scrubber Status Register */
#define DDRC_SBRSTAT                    0x0f28
/* [31:18] rw=- reset=0x0 */
#define DDRC_SBRSTAT_RESERVED_1                    31, 18
/* [17:17] rw=R reset=0x0 */
#define DDRC_SBRSTAT_SCRUB_DONE_DCH1               17, 17
/* [16:16] rw=R reset=0x0 */
#define DDRC_SBRSTAT_SCRUB_BUSY_DCH1               16, 16
/* [15:2] rw=- reset=0x0 */
#define DDRC_SBRSTAT_RESERVED_2                    15,  2
/* [1:1] rw=R reset=0x0 */
#define DDRC_SBRSTAT_SCRUB_DONE                     1,  1
/* [0:0] rw=R reset=0x0 */
#define DDRC_SBRSTAT_SCRUB_BUSY                     0,  0

/* ----- 0x0f2c  SBRWDATA0 ----- */
/* Scrubber Write Data Pattern 0 */
#define DDRC_SBRWDATA0                  0x0f2c
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_SBRWDATA0_SCRUB_PATTERN0              31,  0

/* ----- 0x0f30  SBRWDATA1 ----- */
/* Scrubber Write Data Pattern 1 */
#define DDRC_SBRWDATA1                  0x0f30
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_SBRWDATA1_SCRUB_PATTERN1              31,  0

/* ----- 0x0f34  PDCH ----- */
/* Port Data Channel */
#define DDRC_PDCH                       0x0f34
/* [31:16] rw=- reset=0x0 */
#define DDRC_PDCH_RESERVED_1                       31, 16
/* [15:15] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_15             15, 15
/* [14:14] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_14             14, 14
/* [13:13] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_13             13, 13
/* [12:12] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_12             12, 12
/* [11:11] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_11             11, 11
/* [10:10] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_10             10, 10
/* [9:9] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_9               9,  9
/* [8:8] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_8               8,  8
/* [7:7] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_7               7,  7
/* [6:6] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_6               6,  6
/* [5:5] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_5               5,  5
/* [4:4] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_4               4,  4
/* [3:3] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_3               3,  3
/* [2:2] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_2               2,  2
/* [1:1] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_1               1,  1
/* [0:0] rw=R/W reset=0x0 */
#define DDRC_PDCH_PORT_DATA_CHANNEL_0               0,  0

/* ----- 0x0f38  SBRSTART0 ----- */
/* Scrubber Start Address Mask Register 0 */
#define DDRC_SBRSTART0                  0x0f38
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_SBRSTART0_SBR_ADDRESS_START_MASK_0    31,  0

/* ----- 0x0f3c  SBRSTART1 ----- */
/* Scrubber Start Address Mask Register 1 */
#define DDRC_SBRSTART1                  0x0f3c
/* 变量位域(按配置实例化): sbr_address_start_mask_1 [x:0] rw=R/W reset=0x0 */

/* ----- 0x0f40  SBRRANGE0 ----- */
/* Scrubber Address Range Mask Register 0 */
#define DDRC_SBRRANGE0                  0x0f40
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_SBRRANGE0_SBR_ADDRESS_RANGE_MASK_0    31,  0

/* ----- 0x0f44  SBRRANGE1 ----- */
/* Scrubber Address Range Mask Register 1 */
#define DDRC_SBRRANGE1                  0x0f44
/* 变量位域(按配置实例化): sbr_address_range_mask_1 [x:0] rw=R/W reset=0x0 */

/* ----- 0x0f48  SBRSTART0DCH1 ----- */
/* Scrubber Start Address Mask Register 0 for Data Channel 1 */
#define DDRC_SBRSTART0DCH1              0x0f48
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_SBRSTART0DCH1_SBR_ADDRESS_START_MASK_DCH1_0 31,  0

/* ----- 0x0f4c  SBRSTART1DCH1 ----- */
/* Scrubber Start Address Mask Register 1 for Data Channel 1 */
#define DDRC_SBRSTART1DCH1              0x0f4c
/* 变量位域(按配置实例化): sbr_address_start_mask_dch1_1 [x:0] rw=R/W reset=0x0 */

/* ----- 0x0f50  SBRRANGE0DCH1 ----- */
/* Scrubber Address Range Mask Register 0 for Data Channel 1 */
#define DDRC_SBRRANGE0DCH1              0x0f50
/* [31:0] rw=R/W reset=0x0 */
#define DDRC_SBRRANGE0DCH1_SBR_ADDRESS_RANGE_MASK_DCH1_0 31,  0

/* ----- 0x0f54  SBRRANGE1DCH1 ----- */
/* Scrubber Address Range Mask Register 1 for Data Channel 1 */
#define DDRC_SBRRANGE1DCH1              0x0f54
/* 变量位域(按配置实例化): sbr_address_range_mask_dch1_1 [x:0] rw=R/W reset=0x0 */

/* ----- 0x0ff0  UMCTL2_VER_NUMBER ----- */
/* UMCTL2 Version Number Register */
#define DDRC_UMCTL2_VER_NUMBER          0x0ff0
/* [31:0] rw=R reset=0x3339302a */
#define DDRC_UMCTL2_VER_NUMBER_VER_NUMBER          31,  0

/* ----- 0x0ff4  UMCTL2_VER_TYPE ----- */
/* UMCTL2 Version Type Register */
#define DDRC_UMCTL2_VER_TYPE            0x0ff4
/* [31:0] rw=R reset=0x67612a2a */
#define DDRC_UMCTL2_VER_TYPE_VER_TYPE              31,  0

#endif /* __DDRC_REGS_H__ */
