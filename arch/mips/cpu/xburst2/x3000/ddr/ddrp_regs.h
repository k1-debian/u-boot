/*
 * DDR PHY register-field definitions
 *
 * The register specification lists no register names: every entry is one bit
 * field.  Each field is defined as  DDRP_<NAME>  =  <offset>, <bith>, <bitl>
 * so that the access helpers can derive the field mask and shift
 * automatically:
 *
 *     uint32_t v = ddrp_read_reg(DDRP_CHANNEL_EN);
 *     ddrp_write_reg(DDRP_CHANNEL_EN, 0x1ff);
 *
 * RTL width suffixes in the field names (e.g. pvt_comp_cs_to_reg[6:0]) are
 * stripped; the bit position comes from the Bits column.
 */
#ifndef __DDRP_REGS_H__
#define __DDRP_REGS_H__


/* ============================ register access helpers ============================ */
/*
 * DDRP_xxx expands to (offset, bith, bitl).  Pass it straight to these
 * helpers; the field mask and shift are applied automatically.
 *
 *   uint32_t v = ddrp_read_reg(DDRP_CHANNEL_EN);    // (reg32 & mask) >> bitl
 *   ddrp_write_reg(DDRP_CHANNEL_EN, 0x1ff);         // RMW field write
 *
 *   ddrp_read32(0x14)      /  ddrp_write32(0x14, v)   // whole-register access
 */

#ifndef DDR_PHY_BASE
#define DDR_PHY_BASE 0xb3011000   /* <-- adapt: base address of the DDR PHY */
#endif

static inline uint32_t ddrp_read32(uint32_t offset)
{
	return *(volatile uint32_t *)(DDR_PHY_BASE + offset);
}

static inline void ddrp_write32(uint32_t offset, uint32_t value)
{
	*(volatile uint32_t *)(DDR_PHY_BASE + offset) = value;
}

static inline uint32_t ddrp_mask(uint8_t bith, uint8_t bitl)
{
	uint8_t width = (uint8_t)(bith - bitl + 1);
	uint32_t mask = (width >= 32) ? 0xffffffffu : ((1u << width) - 1u);
	return mask << bitl;
}

static inline uint32_t ddrp_read_reg(uint32_t offset, uint8_t bith, uint8_t bitl)
{
	return (ddrp_read32(offset) & ddrp_mask(bith, bitl)) >> bitl;
}

static inline void ddrp_write_reg(uint32_t offset, uint8_t bith, uint8_t bitl, uint32_t value)
{
	uint32_t mask = ddrp_mask(bith, bitl);
	ddrp_write32(offset, (ddrp_read32(offset) & ~mask) | ((value << bitl) & mask));
}

/* 整寄存器访问（不做字段移位），写的时候直接覆盖整个 32 位 */
static inline uint32_t ddrp_read_reg32(uint32_t offset, uint8_t bith, uint8_t bitl)
{
	(void)bith;
	(void)bitl;
	return ddrp_read32(offset);
}

static inline void ddrp_write_reg32(uint32_t offset, uint8_t bith, uint8_t bitl,
				    uint32_t value)
{
	(void)bith;
	(void)bitl;
	ddrp_write32(offset, value);
}


/* ============================ Common ============================ */


/* ----- 0x000 ----- */
/* [18:18]  RW  reset 0x0  --  reg_dm_invalid_value
 * the invaild dm's value, for ddr4 is 1, other is 0;
 */
#define DDRP_DM_INVALID_VALUE                      0x000, 18, 18

/* [17:17]  RW  reset 0x0  --  reg_dq_invalid_value
 * DDR2:0/1;
 * DDR3:0/1;
 * DDR4:1;
 * LPDDR2:0/1
 * LPDDR3:1
 * LPDDR4:0
 */
#define DDRP_DQ_INVALID_VALUE                      0x000, 17, 17

/* [16:8]  RW  reset 0x1ff  --  reg_channel_en
 * The byte enable signal of the PHY.
 * Active State: High
 * The default mode will open four bytes at the same time:
 * For combo PHY without LPDDRn
 * [0]: The byte0 enable signal, corresponding to DQ0~DQ7.
 * [1]: The byte1 enable signal, corresponding to DQ8~DQ15.
 * [2]: The byte2 enable signal, corresponding to DQ16~DQ23.
 * [3]: The byte3 enable signal, corresponding to DQ24~DQ31.
 * …
 * [8]: The byte 8 enable signal, corresponding to DQ64~DQ71.
 * For combo PHY with LPDDRn
 * [0]: The byte0 enable signal, corresponding to A_DQ0~A_DQ7.
 * [1]: The byte1 enable signal, corresponding to A_DQ8~A_DQ15.
 * [2]: The byte2 enable signal, corresponding to B_DQ0~B_DQ7.
 * [3]: The byte3 enable signal, corresponding to B_DQ8~B_DQ15.
 * …
 * [8]: The byte8 enable signal, corresponding to E_DQ0~E_DQ7.
 * For 2-byte applications, it is suggested to choose the byte0 and byte1.
 */
#define DDRP_CHANNEL_EN                            0x000, 16,  8

/* [7:7]  RW  reset 0x1  --  reg_burst_sel
 * Choose the burst type to be supported according to the SDRAM type.
 * 1: BL8/BL16
 * 0: Reserved
 */
#define DDRP_BURST_SEL                             0x000,  7,  7

/* [6:4]  RW  reset 0x0  --  mem_select_t
 * Choose the SDRAM that the PHY needs to support.
 * 3’h0: DDR2 PHY mode
 * 3’h1: LPDDR2 PHY mode
 * 3’h2: DDR3 PHY mode
 * 3’h3: LPDDR3 PHY mode
 * 3’h4: DDR4 PHY mode
 * 3’h5: LPDDR4 PHY mode
 * Others: Reserved
 */
#define DDRP_MEM_SELECT_T                          0x000,  6,  4

/* [2:2]  RW  reset 0x1  --  soft_reset1
 * The reset signal of digital core.
 * Active State: Low
 */
#define DDRP_SOFT_RESET1                           0x000,  2,  2

/* [1:1]  RW  reset 0x1  --  soft_reset0
 * The reset signal of analog logic.
 * Active State: Low
 */
#define DDRP_SOFT_RESET0                           0x000,  1,  1

/* [0:0]  RW  reset 0x1  --  soft_reset
 * The reset signal of digital core and analog logic.
 * Active State: Low
 */
#define DDRP_SOFT_RESET                            0x000,  0,  0


/* ----- 0x004 ----- */
/* [31:16]  RW  reset 0x0  --  reg_wl_loadmode
 * Write-leveling load mode [15:0].
 * For the lower 8 bits:
 * - For DDR3/4, bit[7:0] should keep the same value with MR1[7:0].
 * - For LPDDR3/4(X), bit[7:0] should keep the same value with the MR2[7:0].
 * For the upper 8 bits:
 * - For DDR3/4, bit[13:8] should keep the same value with the MR1[13:8] and this register[15:14] should be set to 2’b01.
 * - For LPDDR3/4(X), bit[15:8] should be set to 8’h0.
 * Note: Not support for DDR2/LPDDR2.
 */
#define DDRP_WL_LOADMODE                           0x004, 31, 16

/* [15:8]  RW  reset 0x46  --  reg_wl_dqs_start_point
 * Control the start point of the Tx delay line of the DQS for the write-leveling. The difference between the Tx delay line of the command and this register will be the max unbalance range between the CK and DQS.
 * Generally, the Tx delay line value of the command is 8'b1000_0000 which is the 2UI. Then set the start point of the DQS Tx delay line to 8'b0010_0000 which is the 0.5UI. So the write-leveling can cover the 1.5UI gap between the CK and DQS.
 */
#define DDRP_WL_DQS_START_POINT                    0x004, 15,  8

/* [7:6]  RW  reset 0x2  --  reg_wlcs_sel
 * The rank select signal of the write-leveling function.
 * 00: The write-leveling result will auto switch between RANK0 and RANK1 according to the DFI interface command after the write leveling.
 * 01: Choose RANK1. The chip which connects to CS1 will be chosen to enable the write-leveling.
 * 10: Choose RANK0. The chip which connects to CS0 will be chosen to enable the write-leveling.
 * 11: Reserved.
 * Note: It can't be set to 2'b00 when the write-leveling function is enabled. If the PHY needs to support two ranks, this register should be set to 2'b00 after the write-leveling.
 */
#define DDRP_WLCS_SEL                              0x004,  7,  6

/* [5:5]  RW  reset 0x0  --  reg_wl_bypass
 * The Tx per-bit-skew bypass function enable signal.
 * 0: Use the write-leveling result to control the Tx per-bit skew delay.
 *  For combo PHY without LPDDRn
 *       - DQ0~DQ7/DM0 will use DQS0 training result.
 *       - DQ8~DQ15/DM1 will use DQS1 training result.
 *  For combo PHY with LPDDRn
 *       -A_DQ0~A_DQ7/A_DM0 will use A_DQS0 training result.
 *       -A_DQ8~A_DQ15/A_DM1 will use A_DQS1 training result.
 * 1: Use the register to control the Tx per-bit-skew delay. Each data pad has an independent controller register.
 */
#define DDRP_WL_BYPASS                             0x004,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_wl_enable
 * The write-leveling enable signal.
 * 0: Keep current state or exit the write-leveling state
 * 1: Enable the write-leveling function
 */
#define DDRP_WL_ENABLE                             0x004,  4,  4

/* [3:2]  RW  reset 0x2  --  reg_calcs_sel
 * The rank select signal of the Rx-DQS calibration function.
 * 00: The Rx-DQS calibration result will auto switch between the RANK0 and RANK1 according to the DFI interface command after the Rx-DQS calibration training.
 * 01: Choose the RANK1. The chip which connects to CS1 will be chosen to enable the auto Rx-DQS calibration training.
 * 10: Choose the RANK0. The chip which connects to CS0 will be chosen to enable the auto Rx-DQS calibration training.
 * 11: Reserved.
 * Note: It can't be set to 2'b00 when enabling the Rx-DQS calibration training function. If the PHY needs to support two ranks, set the register to 2'b00 after the Rx-DQS calibration training.
 */
#define DDRP_CALCS_SEL                             0x004,  3,  2

/* [1:1]  RW  reset 0x0  --  reg_calib_bypass
 * The Rx-DQS calibration control bypass enable signal.
 * 0: Use the auto Rx-DQS calibration training result to control the Rx-DQS gating signal.
 * 1: Use the bypass register to control the Rx-DQS gating signal. Each DQS and each rank have an independent bypass register.
 */
#define DDRP_CALIB_BYPASS                          0x004,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_start_calib
 * The auto Rx-DQS calibration training enable signal.
 * 0: Keep current state or exit the auto Rx-DQS calibration training.
 * 1: Enable the auto Rx-DQS calibration training function.
 */
#define DDRP_START_CALIB                           0x004,  0,  0


/* ----- 0x008 ----- */
/* [29:24]  RW  reset 0x0  --  AL_FRE_OP0
 * Set the AL value of the PHY for frequency point 0. When enabling the Fast Frequency Change Mode, the PHY will choose AL_FRE_OP0 as the AL setting of the PHY when the dfi_frequency is 2'b00.
 * For LPDDRn, this register should remian the default value.
 * For DDRn, this register should remain the same AL setting with the SDRAM.
 */
#define DDRP_AL_FRE_OP0                            0x008, 29, 24

/* [21:16]  RW  reset 0x0  --  AL_FRE_OP1
 * Set the AL value of the PHY for frequency point 1. When enabling the Fast Frequency Change Mode, the PHY will choose AL_FRE_OP1 as the AL setting of the PHY when the dfi_frequency is 2'b01.
 * For LPDDRn, this register should remain the default value.
 * For DDRn, this register should remain the same AL setting with the SDRAM.
 */
#define DDRP_AL_FRE_OP1                            0x008, 21, 16

/* [13:8]  RW  reset 0x0  --  AL_FRE_OP2
 * Set the AL value of the PHY for frequency point 2. When enabling the Fast Frequency Change Mode, the PHY will choose AL_FEE_OP2 as the AL setting of the PHY when the dfi_frequency is 2'b10.
 * For LPDDRn, this register should remain the default value.
 * For DDRn, this register should remain the same AL setting with the SDRAM.
 */
#define DDRP_AL_FRE_OP2                            0x008, 13,  8

/* [5:0]  RW  reset 0x0  --  AL_FRE_OP3
 * Set the AL value of the PHY for frequency point 3. When enabling the Fast Frequency Change Mode, the PHY will choose AL_FEE_OP3 as the AL setting of the PHY when the dfi_frequency is 2'b11.
 * For LPDDRn, this register should remain the default value.
 * For DDRn, this register should remain the same AL setting with the SDRAM.
 */
#define DDRP_AL_FRE_OP3                            0x008,  5,  0


/* ----- 0x00c ----- */
/* [29:24]  RW  reset 0x6  --  CL_FRE_OP0
 * Set the CL/RL value of the PHY for frequency point 0. When enabling the Fast Frequency Change Mode, the PHY will choose CL_FRE_OP0 as the CL/RL setting of the PHY when the dfi_frequency is 2'b00.
 * For LPDDRn, this register should remain the same RL setting with the SDRAM.
 * For DDRn, this register should remain the same CL setting with the SDRAM.
 */
#define DDRP_CL_FRE_OP0                            0x00c, 29, 24

/* [21:16]  RW  reset 0x6  --  CL_FRE_OP1
 * Set the CL/RL value of the PHY for frequency point 1. When enabling the Fast Frequency Change Mode, the PHY will choose CL_FRE_OP1 as the CL/RL setting of the PHY when the dfi_frequency is 2'b01.
 * For LPDDRn, this register should remain the same RL setting with the SDRAM.
 * For DDRn, this register should remain the same CL setting with the SDRAM.
 */
#define DDRP_CL_FRE_OP1                            0x00c, 21, 16

/* [13:8]  RW  reset 0x6  --  CL_FRE_OP2
 * Set the CL/RL value of the PHY for frequency point 2. When enabling the Fast Frequency Change Mode, the PHY will choose CL_FRE_OP2 as the CL/RL setting of the PHY when the dfi_frequency is 2'b10.
 * For LPDDRn, this register should remain the same RL setting with the SDRAM.
 * For DDRn, this register should remain the same CL setting with the SDRAM.
 */
#define DDRP_CL_FRE_OP2                            0x00c, 13,  8

/* [5:0]  RW  reset 0x6  --  CL_FRE_OP3
 * Set the CL/RL value of the PHY for frequency point 3. When enabling the Fast Frequency Change Mode, the PHY will choose CL_FRE_OP3 as the CL/RL setting of the PHY when the dfi_frequency is 2'b11.
 * For LPDDRn, this register should remain the same RL setting with the SDRAM.
 * For DDRn, this register should remain the same CL setting with the SDRAM.
 */
#define DDRP_CL_FRE_OP3                            0x00c,  5,  0


/* ----- 0x010 ----- */
/* [29:24]  RW  reset 0x0  --  CWL_FRE_OP0
 * Set the CWL/WL value of the PHY for frequency point 0. When enabling the Fast Frequency Change Mode, the PHY will choose CWL_FRE_OP0 as the CWL/WL setting of the PHY when the dfi_frequency is 2'b00.
 * For LPDDRn, this register should remain the same WL setting with the SDRAM.
 * For DDRn, this register should remain the same CWL setting with the SDRAM.
 */
#define DDRP_CWL_FRE_OP0                           0x010, 29, 24

/* [21:16]  RW  reset 0x0  --  CWL_FRE_OP1
 * Set the CWL/WL value of the PHY for frequency point 1. When enabling the Fast Frequency Change Mode, the PHY will choose CWL_FRE_OP1 as the CWL/WL setting of the PHY when the dfi_frequency is 2'b01.
 * For LPDDRn, this register should remain the same WL setting with the SDRAM.
 * For DDRn, this register should remain the same CWL setting with the SDRAM.
 */
#define DDRP_CWL_FRE_OP1                           0x010, 21, 16

/* [13:8]  RW  reset 0x0  --  CWL_FRE_OP2
 * Set the CWL/WL value of the PHY for frequency point 2. When enabling the Fast Frequency Change Mode, the PHY will choose CWL_FRE_OP2 as the CWL/WL setting of the PHY when the dfi_frequency is 2'b10.
 * For LPDDRn, this register should remain the same WL setting with the SDRAM.
 * For DDRn, this register should remain the same CWL setting with the SDRAM.
 */
#define DDRP_CWL_FRE_OP2                           0x010, 13,  8

/* [5:0]  RW  reset 0x0  --  CWL_FRE_OP3
 * Set the CWL/WL value of the PHY for frequency point 3. When enabling the Fast Frequency Change Mode, the PHY will choose CWL_FRE_OP3 as the CWL/WL setting of the PHY when the dfi_frequency is 2'b11.
 * For LPDDRn, this register should remain the same WL setting with the SDRAM.
 * For DDRn, this register should remain the same CWL setting with the SDRAM.
 */
#define DDRP_CWL_FRE_OP3                           0x010,  5,  0


/* ----- 0x014 ----- */
/* [28:24]  RW  reset 0x0  --  reg_fb1xclk_invdelaysel_dqcmd
 * Adjust the hold timing of the digital to analog interface.
 * When increasing this register value, the hold timing of the digital to analog interface increases.
 */
#define DDRP_FB1XCLK_INVDELAYSEL_DQCMD             0x014, 28, 24

/* [20:16]  RW  reset 0x0  --  reg_h4xclk_invdelaysel_dqcmd
 * Reserved.
 */
#define DDRP_H4XCLK_INVDELAYSEL_DQCMD              0x014, 20, 16

/* [12:8]  RW  reset 0x0  --  reg_h4xclkdqs_invdelaysel_dqcmd
 * Reserved.
 */
#define DDRP_H4XCLKDQS_INVDELAYSEL_DQCMD           0x014, 12,  8

/* [4:0]  RW  reset 0x0  --  reg_h1xclk_invdelaysel_dqcmd
 * Reserved.
 */
#define DDRP_H1XCLK_INVDELAYSEL_DQCMD              0x014,  4,  0


/* ----- 0x018 ----- */
/* [31:0]  RW  reset 0xffffffff  --  reg_cmd_ph90en_bp
 * The command 90 degree control signal.
 * Active State: High
 * [0]: A0
 * [1]: A1
 * [2]: A2
 * [3]: A3
 * [4]: A4
 * [5]: A5
 * [6]: A6
 * [7]: A7
 * [8]: A8
 * [9]: A9
 * [10]: A10
 * [11]: A11
 * [12]: A12
 * [13]: A13
 * [14]: A14
 * [15]: A15
 * [16]: A16
 * [17]: A17
 * [18]: ACTN
 * [19]: BA0
 * [20]: BA1
 * [21]: BG0
 * [22]: BG1
 * [23]: CK
 * [24]: CKB
 * [25]: CKE0
 * [26]: CSB0
 * [27]: CSB1
 * [28]: ODT0
 * [29]: ODT1
 * [30]: CKE1
 * [31]: RESETN
 */
#define DDRP_CMD_PH90EN_BP                         0x018, 31,  0


/* ----- 0x01c ----- */
/* [30:28]  RW  reset 0x0  --  reg_pllpostdiv_fsp3
 * Post-divider for FSP[3].
 */
#define DDRP_PLLPOSTDIV_FSP3                       0x01c, 30, 28

/* [27:27]  RW  reset 0x0  --  reg_pllpostdiven_fsp3
 * Post-divider enable for FSP[3].
 * Active State: High
 */
#define DDRP_PLLPOSTDIVEN_FSP3                     0x01c, 27, 27

/* [26:24]  RW  reset 0x3  --  reg_pllcpi_bias_fsp3
 * Reserved.
 */
#define DDRP_PLLCPI_BIAS_FSP3                      0x01c, 26, 24

/* [22:20]  RW  reset 0x0  --  reg_pllpostdiv_fsp2
 * Post-divider for FSP[2].
 */
#define DDRP_PLLPOSTDIV_FSP2                       0x01c, 22, 20

/* [19:19]  RW  reset 0x0  --  reg_pllpostdiven_fsp2
 * Post-divider enable for FSP[2].
 * Active State: High
 */
#define DDRP_PLLPOSTDIVEN_FSP2                     0x01c, 19, 19

/* [18:16]  RW  reset 0x3  --  reg_pllcpi_bias_fsp2
 * Reserved.
 */
#define DDRP_PLLCPI_BIAS_FSP2                      0x01c, 18, 16

/* [14:12]  RW  reset 0x0  --  reg_pllpostdiv_fsp1
 * Post-divider for FSP[1].
 */
#define DDRP_PLLPOSTDIV_FSP1                       0x01c, 14, 12

/* [11:11]  RW  reset 0x0  --  reg_pllpostdiven_fsp1
 * Post-divider enable for FSP[1].
 * Active State: High
 */
#define DDRP_PLLPOSTDIVEN_FSP1                     0x01c, 11, 11

/* [10:8]  RW  reset 0x3  --  reg_pllcpi_bias_fsp1
 * Reserved.
 */
#define DDRP_PLLCPI_BIAS_FSP1                      0x01c, 10,  8

/* [6:4]  RW  reset 0x0  --  reg_pllpostdiv_fsp0
 * Post-divider for FSP[0].
 */
#define DDRP_PLLPOSTDIV_FSP0                       0x01c,  6,  4

/* [3:3]  RW  reset 0x0  --  reg_pllpostdiven_fsp0
 * Post-divider enable for FSP[0].
 * Active State: High
 */
#define DDRP_PLLPOSTDIVEN_FSP0                     0x01c,  3,  3

/* [2:0]  RW  reset 0x3  --  reg_pllcpi_bias_fsp0
 * Reserved.
 */
#define DDRP_PLLCPI_BIAS_FSP0                      0x01c,  2,  0


/* ----- 0x020 ----- */
/* [31:31]  RW  reset 0x0  --  reg_bist_init_done
 * Reserved.
 */
#define DDRP_BIST_INIT_DONE                        0x020, 31, 31

/* [30:30]  RW  reset 0x0  --  reg_bist_init_done_bypass
 * Reserved.
 */
#define DDRP_BIST_INIT_DONE_BYPASS                 0x020, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_catrain_init_from_bist
 * Reserved.
 */
#define DDRP_CATRAIN_INIT_FROM_BIST                0x020, 29, 29

/* [27:27]  RW  reset 0x0  --  reg_cmd_bist_err_inject
 * Used to insert error to the PHY BIST. When this bit is set to high at PHY BIST, the read back value of A5 will be locked to “0”. So the BIST error will be high when the BIST complete is high. You can read the read-only register bist_error_cmd, the bit[5] is high, indicating that A5 has an error.
 */
#define DDRP_CMD_BIST_ERR_INJECT                   0x020, 27, 27

/* [26:26]  RW  reset 0x0  --  reg_cmd_2t_mode_t
 * The enable signal of the command bus 2T mode.
 * Active State: High
 */
#define DDRP_CMD_2T_MODE_T                         0x020, 26, 26

/* [25:25]  RW  reset 0x0  --  reg_oscen
 * The analog OSC clock enable signal.
 * Active State: High
 */
#define DDRP_OSCEN                                 0x020, 25, 25

/* [23:23]  RW  reset 0x1  --  reg_vt_comp_bp
 * The Rx VT compensation disable signal.
 * 1: Disable
 * 0: Not supported in this design
 */
#define DDRP_VT_COMP_BP                            0x020, 23, 23

/* [22:22]  RW  reset 0x1  --  reg_cmdout_mux
 * The timing control of the command path between the digital part and analog part.
 * 1: The command signal from the digital part to the analog part is aligned to the posedge of the dfi_clk1x.
 * 0: The command signal from the digital part to the analog part is aligned to the negedge of the dfi_clk1x.
 */
#define DDRP_CMDOUT_MUX                            0x020, 22, 22

/* [21:21]  RW  reset 0x0  --  ph90en_bp
 * The 90 degree control bypass enable signal of the command path.
 * In normal mode, the 90 degree control of the command path is set automatically based on the SDRAM type. You also can use this register to choose the bypass mode, then use the register reg_cmd_ph90en_bp to control the 90 degree of each command pad.
 * 1: Choose the bypass mode.
 * 0: Choose the auto mode.
 */
#define DDRP_PH90EN_BP                             0x020, 21, 21

/* [20:20]  RW  reset 0x0  --  ph90en_bp_dq
 * The 90 degree control bypass enable signal of the data path.
 * In normal mode, the 90 degree control of the data path is set automatically based on the SDRAM type.
 * You also can use this register to choose the bypass mode, then use the register reg_{a/b/...}_{l/h}dq_ph90en_bp to control the 90 degree of each DQ pad.
 * 1: Choose the bypass mode.
 * 0: Choose the auto mode.
 */
#define DDRP_PH90EN_BP_DQ                          0x020, 20, 20

/* [19:19]  RW  reset 0x0  --  reg_rden_bypass
 * The enable signal of the Rx FIFO read control bypass mode.
 * 0: Disable the bypass mode.
 * 1: Enable the bypass mode and use the reg_rden_delay to control the Rx FIFO read timing.
 */
#define DDRP_RDEN_BYPASS                           0x020, 19, 19

/* [18:16]  RW  reset 0x2  --  reg_rden_delay
 * This register is used to control read timing of the Rx FIFO when enabling the Rx FIFO read control bypass mode. When the dfi_rddata_en changes to high, the PHY will read out the Rx data from the Rx FIFO after the following delay.
 * 0: 9 + max(reg_{a/b}_{l/h}_cycsel)
 * 1: 10 + max(reg_{a/b}_{l/h}_cycsel)
 * 2: 11 + max(reg_{a/b}_{l/h}_cycsel)
 * 3: 12 + max(reg_{a/b}_{l/h}_cycsel)
 * 4: 13 + max(reg_{a/b}_{l/h}_cycsel)
 * 5: 14 + max(reg_{a/b}_{l/h}_cycsel)
 * 6: 15 + max(reg_{a/b}_{l/h}_cycsel)
 * 7: 16 + max (reg_{a/b}_{l/h}_cycsel)
 * For auto mode, the delay = 11 + max(reg_{a/b}_{l/h}_cycsel).
 */
#define DDRP_RDEN_DELAY                            0x020, 18, 16

/* [15:15]  RW  reset 0x1  --  reg_phy_long_txenb
 * Reserved
 */
#define DDRP_PHY_LONG_TXENB                        0x020, 15, 15

/* [14:14]  RW  reset 0x0  --  reg_rdodt_bypass
 * The read training ODT bypass enable signal.
 * Active State: High
 */
#define DDRP_RDODT_BYPASS                          0x020, 14, 14

/* [13:13]  RW  reset 0x0  --  reg_rxodt_st_bypass
 * The enable signal of the Rx ODT start point bypass mode, used to control the start point of the Rx ODT.
 * Active State: High
 */
#define DDRP_RXODT_ST_BYPASS                       0x020, 13, 13

/* [11:8]  RW  reset 0x0  --  reg_rxodt_stdelay
 * Control the Rx ODT start point when enabling the Rx ODT start point bypass mode.
 * Decreasing this value will left shift the Rx ODT window, increasing this value will right shift the Rx ODT window.
 * Unit: dfi_clk1x.
 * For auto mode, the start_point = (RL-1)>>1.
 */
#define DDRP_RXODT_STDELAY                         0x020, 11,  8

/* [7:4]  RW  reset 0x2  --  reg_rxodt_length
 * Control the length of the Rx ODT. The step is 1x clock cycle.
 * When the value +1, the length of Rx ODT will increase 1x clock cycle.
 * Related to register reg_rxodt_start_point.
 * It combines with the register reg_rxodt_start_point to decide the range of the Rx ODT.
 * The default Rx ODT length is 4 * dfi_clk1x cycles.
 */
#define DDRP_RXODT_LENGTH                          0x020,  7,  4

/* [3:0]  RW  reset 0x3  --  reg_rxodt_start_point
 * Control the start point of the Rx ODT.
 * When the value +1, the start point of Rx ODT will increase 1x clock cycle.
 * To keep the length of the Rx ODT unchanged, you can  increase 1x clock cycle of the length of the Rx ODT using the register reg_rxodt_length.
 * The default Rx ODT start point is RL-2 in regard to the read command.
 */
#define DDRP_RXODT_START_POINT                     0x020,  3,  0


/* ----- 0x024 ----- */
/* [31:24]  RW  reset 0x5f  --  reg_ddrc_tzqinit_phy
 * Control the timing of the tZQCAL for LPDDR4/4X initialization which should >= 1us.
 * The delay = reg_ddrc_tzqinit * Tclkinit. The Tclkinit means the period of the low frequency.
 */
#define DDRP_DDRC_TZQINIT_PHY                      0x024, 31, 24

/* [23:16]  RW  reset 0x1f  --  reg_ddrc_tzqlat_phy
 * Control the timing of the tZQLAT for LPDDR4/4X initialization which should >= 30ns.
 * The delay = reg_ddrc_tzqlat * Tclkinit. The Tclkinit means the period of the low frequency.
 */
#define DDRP_DDRC_TZQLAT_PHY                       0x024, 23, 16

/* [15:8]  RW  reset 0x1f  --  reg_ddrc_treset_l_x1024_phy
 * Control the timing of the tINIT1 for LPDDR4/4X initialization which should >= 200us.
 * The delay = reg_ddrc_treset_l_x1024 * 1024 * Tclkinit. The Tclkinit means the period of the low frequency.
 */
#define DDRP_DDRC_TRESET_L_X1024_PHY               0x024, 15,  8

/* [7:0]  RW  reset 0x64  --  reg_ddrc_treset_h_x1024_phy
 * Control the timing of the tINIT3 for LPDDR4/4X initialization which should >= 2ms.
 * The delay = reg_ddrc_treset_h_x1024 * 1024 * Tclkinit. The Tclkinit means the period of the low frequency.
 */
#define DDRP_DDRC_TRESET_H_X1024_PHY               0x024,  7,  0


/* ----- 0x028 ----- */
/* [31:24]  RW  reset 0x1f  --  reg_ddrc_tckeh_phy
 * Used to control the timing of the tINIT5 for LPDDR4 initialization which should >= 2us.
 * The delay = reg_ddrc_tckeh * Tclkinit. The Tclkinit means the period of the low frequency.
 */
#define DDRP_DDRC_TCKEH_PHY                        0x028, 31, 24

/* [23:23]  RW  reset 0x1  --  reg_cat_ca_then_cs
 * The command bus training mode select.
 * 0: Train CS firstly then train CA.
 * 1: Train CA firstly then train CS.
 */
#define DDRP_CAT_CA_THEN_CS                        0x028, 23, 23

/* [22:22]  RW  reset 0x0  --  reg_cat_clear
 * The clean enable signal of the command bus training module.
 * 1: Clean the command bus training module.
 * 0: Keep the current state.
 */
#define DDRP_CAT_CLEAR                             0x028, 22, 22

/* [19:18]  RW  reset 0x3  --  reg_cat_rank_num
 * Define which rank that the command bus training should support.
 * 2’b00: Two ranks
 * 2’b10: Rank0
 * 2’b01: Rank1
 * Others: Not support.
 */
#define DDRP_CAT_RANK_NUM                          0x028, 19, 18

/* [17:16]  RW  reset 0x3  --  reg_cat_channel_num
 * The command channel enable signal of command bus training.
 * 2’b00: Enable Channel A and Channel B.
 * 2’b10: Only enable Channel A.
 * 2’b01: Only enable Channel B.
 */
#define DDRP_CAT_CHANNEL_NUM                       0x028, 17, 16

/* [12:8]  RW  reset 0x4  --  reg_clk_div_cnt
 * Control the divider of the dfi_clk1x which is used for the initialization of the SDRAM and command bus training.
 * The dfi_clk1x will be clock divided to the low speed based on this register.
 * Tclkinit = Tdfi_clk1x / (2 * reg_clk_div_cnt).
 */
#define DDRP_CLK_DIV_CNT                           0x028, 12,  8

/* [7:6]  RW  reset 0x0  --  reg_cat_bp_rank_sel
 * Choose the rank when enabling the auto command bus training.
 * 2’b10: Rank0.
 * 2’b01: Rank1.
 */
#define DDRP_CAT_BP_RANK_SEL                       0x028,  7,  6

/* [5:5]  RW  reset 0x0  --  reg_cat_bp_cmd_send
 * Note: This register is not available for this design.
 * Send the command bus training command when enabling the command bus training bypass mode.
 * Posedge is valid.
 * When this signal changes from low to high, it will send the command bus training command to the SDRAM. The command bus value depends on the reg_cat_bp_mode, reg_cat_cs_train_value and reg_cat_ca_train_value.
 */
#define DDRP_CAT_BP_CMD_SEND                       0x028,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_cat_bp_mode
 * Note: This register is not available for this design.
 * Choose the command bus training stage for the bypass mode.
 * 0: Current is CS bypass command bus training.
 * 1: Current is CA bypass command bus training.
 */
#define DDRP_CAT_BP_MODE                           0x028,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_cat_bp_en
 * Note: This register is not available for this design.
 * The bypass command bus training enable signal.
 * Active State: High.
 * When this bit is set to high, the command path will choose the command bus training module and the Tx delay line of command will be controlled by the register directly through the logic in the command bus training.
 */
#define DDRP_CAT_BP_EN                             0x028,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_cat_bp_start
 * Note: This register is not available for this design.
 * The bypass command bus training start signal.
 * Active State: High.
 * When this bit is set to high, the command bus module will go into the bypass command bus training flow.
 */
#define DDRP_CAT_BP_START                          0x028,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_cat_start
 * The auto command bus training start signal.
 * Active State: High.
 * When this signal is enabled, it will begin the command bus training.
 */
#define DDRP_CAT_START                             0x028,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_cat_enable
 * The auto command bus training enable signal.
 * Active State: High.
 * When this signal is enabled, the command path will switch to the command bus training module.
 */
#define DDRP_CAT_ENABLE                            0x028,  0,  0


/* ----- 0x02c ----- */
/* [28:25]  RW  reset 0x4  --  reg_txcbt
 * Control the timing parameter tcs_VREF/tCKCKEH/tMRZ.
 * The delay should >= MAX(1.5ns,2nCK).
 */
#define DDRP_TXCBT                                 0x02c, 28, 25

/* [23:20]  RW  reset 0xf  --  reg_tadr
 * Control the check time of the read back data from the DQ.
 * The delay should >= 20ns.
 */
#define DDRP_TADR                                  0x02c, 23, 20

/* [18:15]  RW  reset 0x4  --  reg_tckelck
 * Control the clock and command valid after CKE low(tCKELCK).
 * The delay should >= MAX(7.5ns,3nCK)
 */
#define DDRP_TCKELCK                               0x02c, 18, 15

/* [13:10]  RW  reset 0x1  --  reg_tdstrain
 * Data Setup/Hold for Vref (CA) training mode.
 */
#define DDRP_TDSTRAIN                              0x02c, 13, 10

/* [8:5]  RW  reset 0xf  --  reg_tmrw
 * Control the delay between the MRW command and valid clock/CS requirement after CKE input LOW after MRW command.
 * The delay >= MAX(14ns, 10nCK).
 */
#define DDRP_TMRW                                  0x02c,  8,  5

/* [4:0]  RW  reset 0x15  --  reg_tcacd
 * Control the CA Bus Training Command to CA Bus Training Command Delay( tCACD).
 * The delay should >= RU(20ns/tCK).
 */
#define DDRP_TCACD                                 0x02c,  4,  0


/* ----- 0x030 ----- */
/* [28:25]  RW  reset 0x5  --  reg_phy_div_value
 * Oscillator count divider.
 */
#define DDRP_PHY_DIV_VALUE                         0x030, 28, 25

/* [24:16]  RW  reset 0x28  --  reg_tvrefca_long
 * Control the Vref(CA) step time.
 */
#define DDRP_TVREFCA_LONG                          0x030, 24, 16

/* [15:8]  RW  reset 0xf  --  reg_tcaent
 * Control the first CA Bus Training Command following CKE Low(tCAENT).
 * The delay should >= 250ns.
 */
#define DDRP_TCAENT                                0x030, 15,  8

/* [7:0]  RW  reset 0x1f  --  reg_tfc
 * Control the frequency set point switching time(tFC).
 * The delay should >= 250ns.
 */
#define DDRP_TFC                                   0x030,  7,  0


/* ----- 0x034 ----- */
/* [31:24]  RW  reset 0x0  --  reg_mr1
 * Control the LPDDR4 load mode value of MR1 during command bus training and the DDR4 load mode value of MR3 during read training (MPR).  When in DDR4 read training, set this register as current SDRAM MR3 (A10~A3) value. The read training operation will write new data to the MR3, so keep the other bits the same as the current MR3.
 */
#define DDRP_MR1                                   0x034, 31, 24

/* [23:16]  RW  reset 0x0  --  reg_mr2
 * Control the LPDDR4 load mode value of MR2 when doing the command bus training.
 */
#define DDRP_MR2                                   0x034, 23, 16

/* [15:8]  RW  reset 0x0  --  reg_mr3
 * Control the LPDDR4 load mode value of MR3 when doing the command bus training.
 */
#define DDRP_MR3                                   0x034, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_mr11
 * Control the LPDDR4 load mode value of MR11 when doing the command bus training.
 */
#define DDRP_MR11                                  0x034,  7,  0


/* ----- 0x038 ----- */
/* [31:24]  RW  reset 0x3f  --  reg_cat_cs_right_scan_steps
 * The right scan steps of the command bus training of CS pad.
 */
#define DDRP_CAT_CS_RIGHT_SCAN_STEPS               0x038, 31, 24

/* [23:16]  RW  reset 0x0  --  reg_mr13
 * Control the LPDDR4 load mode value of MR13 when doing the command bus training.
 */
#define DDRP_MR13                                  0x038, 23, 16

/* [15:8]  RW  reset 0x0  --  reg_mr14
 * Control the LPDDR4 load mode value of MR14 when doing the command bus training.
 */
#define DDRP_MR14                                  0x038, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_mr22_rank0
 * Control the LPDDR4 RANK0 load mode value of MR22 when doing the command bus training.
 */
#define DDRP_MR22_RANK0                            0x038,  7,  0


/* ----- 0x03c ----- */
/* [31:31]  RW  reset 0x0  --  reg_ca_vref_update
 * Reserved.
 */
#define DDRP_CA_VREF_UPDATE                        0x03c, 31, 31

/* [30:30]  RW  reset 0x0  --  reg_cat_vref_scan_disable
 * Disable the CA_VREF training when doing the auto command bus training.
 * Active State: High
 * 1: Only scan the timing window of the CA and CS based on the current CA_VREF setting.
 * 0: Scan the CA_VREF to find the best point.
 */
#define DDRP_CAT_VREF_SCAN_DISABLE                 0x03c, 30, 30

/* [29:24]  RW  reset 0x32  --  reg_cat_vref_scan_max
 * Set the max value that the Vref training.
 * When the Vref scan reaches the reg_cat_vref_scan_max, it will stop the Vref scan.
 */
#define DDRP_CAT_VREF_SCAN_MAX                     0x03c, 29, 24

/* [21:16]  RW  reset 0x0  --  reg_cat_vref_scan_min
 * Set the min value that the perbit skew training.
 * When the perbit skew scan reaches the reg_cat_ca_scan_max, it will stop the Tx delay line scan of the command.
 */
#define DDRP_CAT_VREF_SCAN_MIN                     0x03c, 21, 16

/* [14:8]  RW  reset 0x1f  --  reg_cha_cat_vref_bp_value
 * Reserved.
 */
#define DDRP_CHA_CAT_VREF_BP_VALUE                 0x03c, 14,  8

/* [6:0]  RW  reset 0x1f  --  reg_chb_cat_vref_bp_value
 * Reserved.
 */
#define DDRP_CHB_CAT_VREF_BP_VALUE                 0x03c,  6,  0


/* ----- 0x040 ----- */
/* [31:28]  RW  reset 0x8  --  reg_cs_perbit_skew_offest_fsp0
 * The FSP[0] compensation value of CSB Tx delay after the command bus training. When the PHY performs the command bus training to scan the CSB delay window, the CSB has 75%*2UI. So after the command bus training, you need to make a compensation for the 25%*2UI. The compensation value is the half of the lost pulse width = 1/4UI.
 */
#define DDRP_CS_PERBIT_SKEW_OFFEST_FSP0            0x040, 31, 28

/* [27:24]  RW  reset 0x8  --  reg_cs_perbit_skew_offest_fsp1
 * The FSP[1] compensation value of CSB Tx delay after the command bus training. When the PHY performs the command bus training to scan the CSB delay window, the CSB has 75%*2UI. So after the command bus training, you need to make a compensation for the 25%*2UI. The compensation value is the half of the lost pulse width = 1/4UI.
 */
#define DDRP_CS_PERBIT_SKEW_OFFEST_FSP1            0x040, 27, 24

/* [23:20]  RW  reset 0x8  --  reg_cs_perbit_skew_offest_fsp2
 * The FSP[2] compensation value of CSB Tx delay after the command bus training. When the PHY performs the command bus training to scan the CSB delay window, the CSB has 75%*2UI. So after the command bus training, you need to make a compensation for the 25%*2UI. The compensation value is the half of the lost pulse width = 1/4UI.
 */
#define DDRP_CS_PERBIT_SKEW_OFFEST_FSP2            0x040, 23, 20

/* [19:16]  RW  reset 0x8  --  reg_cs_perbit_skew_offest_fsp3
 * The FSP[3] compensation value of CSB Tx delay after the command bus training. When the PHY performs the command bus training to scan the CSB delay window, the CSB has 75%*2UI. So after the command bus training, you need to make a compensation for the 25%*2UI. The compensation value is the half of the lost pulse width = 1/4UI.
 */
#define DDRP_CS_PERBIT_SKEW_OFFEST_FSP3            0x040, 19, 16

/* [13:12]  RW  reset 0x1  --  reg_lpddr4_ca_odt
 * Select the rank to control the CA_ODT in LPDDR4 mode when reg_lpddr4_ca_odt_sel is set to 1‘b1 to use this register to control the CA_ODT.
 * 01: Two Ranks
 * 01: Rank0
 * 10: Rank1
 */
#define DDRP_LPDDR4_CA_ODT                         0x040, 13, 12

/* [11:11]  RW  reset 0x0  --  reg_lpddr4_ca_odt_sel
 * Choose the CA ODT control mode in LPDDR4 mode.
 * 1: Use the register reg_lpddr4_ca_odt to control.
 * 0: Use the dfi_odt to control.
 */
#define DDRP_LPDDR4_CA_ODT_SEL                     0x040, 11, 11

/* [10:8]  RW  reset 0x1  --  reg_cat_vref_scan_steps
 * Control the CA_VREF scan steps.
 * When the reg_cat_vref_scan_disable is set to 1’b0 to enable the CA_VREF scan mode, you can use this register to control the scan steps to decrease the scan time.
 */
#define DDRP_CAT_VREF_SCAN_STEPS                   0x040, 10,  8

/* [7:7]  RW  reset 0x1  --  reg_cs_pwc_disable
 * The disable signal of pulse width control of CS.
 * Active State: High.
 * When the PHY performs the command bus training of CS, if the pulse width control function is enabled, the pulse width will decrease 25% for command bus training. Before entering command bus training and after exiting command bus training, the pulse width of CS will keep the normal pulse width.
 */
#define DDRP_CS_PWC_DISABLE                        0x040,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_cat_skip_fspy
 * For the command bus training, you can set the FSP[Y] ( assume that the current is FSP[X]) at the low speed based on the setting in the register ( reg_mr1~reg_mr22_rank{0/1}). This register is used to enable this function.
 * 1: Enable the function to set the FSP[Y] before the command bus training.
 * 0: Disable the function to set the FSP[Y]] before the command bus training. When this mode is selected, you need to set the FSP[Y] before enabling the command bus traning.
 * Note: For the first time of high speed command bus training, the PHY will complete the SDRAM initialization and skip this operation automatically.
 */
#define DDRP_CAT_SKIP_FSPY                         0x040,  6,  6

/* [5:4]  RW  reset 0x0  --  reg_cat_fspy_rank
 * Choose the RANK which needs to be set after setting the FSP[Y] in the command bus training flow by setting the reg_cat_skip_fspy = 1’b0.
 * 00: Choose RANK0 and RANK1.
 * 01: Choose Rank1.
 * 10: Choose Rank0.
 * 11: Disable the setting.
 */
#define DDRP_CAT_FSPY_RANK                         0x040,  5,  4

/* [3:3]  RW  reset 0x1  --  reg_cat_fc_pd_en
 * Reserved.
 */
#define DDRP_CAT_FC_PD_EN                          0x040,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_cat_skip_cs_train
 * The disable signal to skip the CS training flow of the auto command bus training.
 * 1: Skip the CS training and go to the CA training directly.
 * 0: Begin the CS training firstly and go to the CA training.
 */
#define DDRP_CAT_SKIP_CS_TRAIN                     0x040,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_cmd_perbit_skew_bp
 * The Tx delay line of the command pad bypass enable signal for LPDDR4.
 * 1: Use registers to control the Tx delay line of the command pad.
 * 0: Use the command bus training module to control the Tx delay line of the command pad.
 * Note: For other SDRAM types, the Tx delay line is always controlled by the register directly. This register is unused.
 */
#define DDRP_CMD_PERBIT_SKEW_BP                    0x040,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_ca_perbit_skew_update
 * Update the perbit skew of the command in the command bus training module through the registers.
 * Posedge is valid.
 * When this signal changes from low to high, it will update the perbit skew value by setting the register to the command bus training module according to the current reg_freq_choose_op_t.
 */
#define DDRP_CA_PERBIT_SKEW_UPDATE                 0x040,  0,  0


/* ----- 0x044 ----- */
/* [25:25]  RW  reset 0x0  --  reg_cat_cke_mode
 * Reserved.
 */
#define DDRP_CAT_CKE_MODE                          0x044, 25, 25

/* [24:24]  RW  reset 0x1  --  reg_cat_ck_cke_odt_fix_perbit_skew
 * Reserved.
 */
#define DDRP_CAT_CK_CKE_ODT_FIX_PERBIT_SKEW        0x044, 24, 24

/* [23:16]  RW  reset 0xff  --  reg_cat_ca_scan_max
 * Set the perbit skew scan range of the command bus when enabling the command bus training in LPDDR4 mode.
 * The scan range is [0:reg_cat_ca_scan_max].
 */
#define DDRP_CAT_CA_SCAN_MAX                       0x044, 23, 16

/* [13:8]  RW  reset 0x3f  --  reg_cat_ca_train_value
 * Control the CA command bus training pattern.
 */
#define DDRP_CAT_CA_TRAIN_VALUE                    0x044, 13,  8

/* [5:0]  RW  reset 0x6  --  reg_cat_cs_train_value
 * Control the CS command bus training pattern.
 */
#define DDRP_CAT_CS_TRAIN_VALUE                    0x044,  5,  0


/* ----- 0x048 ----- */
/* [29:24]  RW  reset 0x6  --  reg_cha_cat_cs_check_value
 * The check pattern of CS training mode for Channel A.
 */
#define DDRP_CHA_CAT_CS_CHECK_VALUE                0x048, 29, 24

/* [21:16]  RW  reset 0x6  --  reg_chb_cat_cs_check_value
 * The check pattern of CS training mode for Channel B.
 */
#define DDRP_CHB_CAT_CS_CHECK_VALUE                0x048, 21, 16

/* [13:8]  RW  reset 0x3f  --  reg_cha_cat_ca_check_value
 * The check pattern of CA training mode for Channel A.
 */
#define DDRP_CHA_CAT_CA_CHECK_VALUE                0x048, 13,  8

/* [6:1]  RW  reset 0x3f  --  reg_chb_cat_ca_check_value
 * The check pattern of CA training mode for Channel B.
 */
#define DDRP_CHB_CAT_CA_CHECK_VALUE                0x048,  6,  1

/* [0:0]  RW  reset 0x1  --  reg_cat_channel_num_lp3
 * The command_channel enable signal of lpddr3 command bus train. High is valid.
 * [0]: set 1 then enable channel A.
 * [1]: set 1 then enable channel B.
 */
#define DDRP_CAT_CHANNEL_NUM_LP3                   0x048,  0,  0


/* ----- 0x04c ----- */
/* [31:30]  RW  reset 0x0  --  reg_freq_choose_op_t
 * Used for fast frequency switch. Valid only when reg_freq_choose_op_bypass is high.
 * 00: Freq Point 0.
 * 01: Freq Point 1.
 * 10: Freq Point 2.
 * 11: Freq Point 3.
 * In the normal mode, the current frequency point is set by the dfi_frequency at the initialization and frequency change. To change the current working frequency or update the registers which are used to control the other frequency points, use this register to set the frequency point first.
 * After choosing the frequency point using this register, you can use the update register to switch the training result to the corresponding frequency point fastly.
 */
#define DDRP_FREQ_CHOOSE_OP_T                      0x04c, 31, 30

/* [29:29]  RW  reset 0x0  --  reg_lpddr4_rd_preamble
 * Choose the read preamble mode when PHY is in LPDDR4 mode.
 * 0: Static.
 * 1: Toggle.
 * Note: This setting should keep the same setting with the MR#1[3] of the LPDDR4.
 */
#define DDRP_LPDDR4_RD_PREAMBLE                    0x04c, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_wl_freq_update
 * The data Tx delay line control value update signal.
 * Active State: High.
 * In the default mode, the Tx delay line is controlled by the write-leveling/write training result according to the frequency point. To change the value to adjust the delay line, you can use the bypass registers to set the value and set this signal to “1”. Then the bypass register value will be updated to the control register of the Tx perbit skew based on the current frequency point.
 * Refer to Write Leveling section in the Databook.
 * Note: Please recover it to “0” after the setting.
 */
#define DDRP_WL_FREQ_UPDATE                        0x04c, 28, 28

/* [27:27]  RW  reset 0x0  --  reg_calib_freq_update
 * The Rx DQS Gating control value update signal.
 * Active State: High.
 * In the default mode, after the Rx DQS Gating, the training value will control the Rx DQS Gating delay. To change the value and adjust the Rx DQS Gating window, you can use the bypass registers to set the control value. If this bit is set to “1”,  the setting value will be updated to the control register of the Rx DQS Gating based on the current frequency point.
 * Refer to Auto Rx DQS Gating Training section in the Databook.
 * Note: Please recover it to “0” after the setting.
 */
#define DDRP_CALIB_FREQ_UPDATE                     0x04c, 27, 27

/* [26:26]  RW  reset 0x0  --  reg_cmd_abutobsmodeen
 * Reserved.
 */
#define DDRP_CMD_ABUTOBSMODEEN                     0x04c, 26, 26

/* [25:25]  RW  reset 0x0  --  reg_sdram_vref_update
 * Reserved.
 */
#define DDRP_SDRAM_VREF_UPDATE                     0x04c, 25, 25

/* [24:24]  RW  reset 0x0  --  reg_calib_mode_sel
 * The DQS gating mode select.
 * 1: Read Preamble Training mode (only for DDR4).
 * 0: Normal Read mode (for DDR2/3/4 and LPDDR2/3/4).
 * When the DQS Gating mode chooses the Normal Read mode for DDR4, the Read DQS will be pulled down to 1’b0. So the register reg_{a/b…}_{l/h}_weakpub_reg should be set to 2’b00 to pull-down the Read DQS when it is in “high z” state.
 */
#define DDRP_CALIB_MODE_SEL                        0x04c, 24, 24

/* [22:20]  RW  reset 0x3  --  reg_lpddr4_cmd_gap_for_diff_rank
 * Reserved.
 */
#define DDRP_LPDDR4_CMD_GAP_FOR_DIFF_RANK          0x04c, 22, 20

/* [19:16]  RW  reset 0x0  --  reg_b0_cmdobsmuxsel
 * Reserved.
 */
#define DDRP_B0_CMDOBSMUXSEL                       0x04c, 19, 16

/* [15:15]  RW  reset 0x0  --  reg_b0_obsdataen
 * Reserved.
 */
#define DDRP_B0_OBSDATAEN                          0x04c, 15, 15

/* [14:14]  RW  reset 0x0  --  bist_calibst
 * Reserved.
 */
#define DDRP_BIST_CALIBST                          0x04c, 14, 14

/* [13:13]  RW  reset 0x0  --  reg_fiford_delay
 * Reserved.
 */
#define DDRP_FIFORD_DELAY                          0x04c, 13, 13

/* [12:12]  RW  reset 0x0  --  bist_ck_select
 * Choose the CLK which is used to sample the BIST command when the PHY normal BIST is enabled.
 * 1: Choose the feedback signal from the CK pad as the clock.
 * 0: Choose the feedback signal from the CKB pad as the clock.
 */
#define DDRP_BIST_CK_SELECT                        0x04c, 12, 12

/* [11:11]  RW  reset 0x0  --  reg_mch_odt
 * Reserved.
 */
#define DDRP_MCH_ODT                               0x04c, 11, 11

/* [10:10]  RW  reset 0x0  --  mux_sync_sel
 * Reserved.
 */
#define DDRP_MUX_SYNC_SEL                          0x04c, 10, 10

/* [9:9]  RW  reset 0x0  --  reg_sync_en
 * Reserved.
 */
#define DDRP_SYNC_EN                               0x04c,  9,  9

/* [8:8]  RW  reset 0x1  --  reg_rdptr_delay
 * Reserved.
 */
#define DDRP_RDPTR_DELAY                           0x04c,  8,  8

/* [7:7]  RW  reset 0x0  --  reg_scr_rcvmodsel
 * Reserved.
 */
#define DDRP_SCR_RCVMODSEL                         0x04c,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_scr_sckdimm0dis
 * Reserved.
 */
#define DDRP_SCR_SCKDIMM0DIS                       0x04c,  6,  6

/* [5:5]  RW  reset 0x1  --  reg_scr_sdhsclkb_pos1neg0sel
 * Reserved.
 */
#define DDRP_SCR_SDHSCLKB_POS1NEG0SEL              0x04c,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_scr_sdhsclk_pos1neg0sel
 * Reserved.
 */
#define DDRP_SCR_SDHSCLK_POS1NEG0SEL               0x04c,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_scr_odttrien
 * Reserved.
 */
#define DDRP_SCR_ODTTRIEN                          0x04c,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_scr_csbtrien
 * Reserved.
 */
#define DDRP_SCR_CSBTRIEN                          0x04c,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_scr_cmdtrien
 * Reserved.
 */
#define DDRP_SCR_CMDTRIEN                          0x04c,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_scr_cketrirnk0
 * Reserved.
 */
#define DDRP_SCR_CKETRIRNK0                        0x04c,  0,  0


/* ----- 0x050 ----- */
/* [31:24]  RW  reset 0x3f  --  reg_cat_cs_left_scan_steps
 * The left scan steps of the command bus training of CS pad.
 */
#define DDRP_CAT_CS_LEFT_SCAN_STEPS                0x050, 31, 24

/* [20:20]  RW  reset 0x0  --  reg_io_highz_dis
 * Reserved.
 */
#define DDRP_IO_HIGHZ_DIS                          0x050, 20, 20

/* [19:19]  RW  reset 0x0  --  reg_cmd_delay_one_ui
 * Reserved.
 */
#define DDRP_CMD_DELAY_ONE_UI                      0x050, 19, 19

/* [18:18]  RW  reset 0x0  --  cmd_bypassen
 * Reserved.
 */
#define DDRP_CMD_BYPASSEN                          0x050, 18, 18

/* [17:17]  RW  reset 0x1  --  reg_cmd_iobufact_bp
 * Reserved.
 */
#define DDRP_CMD_IOBUFACT_BP                       0x050, 17, 17

/* [16:8]  RW  reset 0x0  --  bypassen
 * Reserved.
 */
#define DDRP_BYPASSEN                              0x050, 16,  8

/* [7:0]  RW  reset 0x80  --  reg_mpr_cnt
 * Control the timing of the MPR command for DDR4 Rx DQS calibration.
 * Timing interval between MPR = reg_mpr_cnt * dfi_clk1x.
 */
#define DDRP_MPR_CNT                               0x050,  7,  0


/* ----- 0x054 ----- */
/* [31:30]  RW  reset 0x2  --  reg_lp3_odt1_mask
 * This is a register only for LPDDR3.
 * When 2 ranks are in a single package, this should be 0x0 (odt1=0);
 * When 2 ranks are in 2 packages, this should be 0x2 (odt1=odt1) .
 */
#define DDRP_LP3_ODT1_MASK                         0x054, 31, 30

/* [29:28]  RW  reset 0x1  --  reg_lp3_odt0_mask
 * This is a register only for LPDDR3.
 * When 2 ranks are in a single package, this should be 0x3 (odt0=odt0|odt1);
 * When 2 ranks are in 2 packages, this should be 0x1 (odt0=odt0).
 */
#define DDRP_LP3_ODT0_MASK                         0x054, 29, 28

/* [27:26]  RW  reset 0x0  --  reg_freq_choose_wr_t
 * Select the frequency point for the desired delay register configuration.
 * Valid only when the reg_freq_choose_wr_bypass is high.
 * 00: Frequency Point 0
 * 01: Frequency Point 1
 * 10: Frequency Point 2
 * 11: Frequency Point 3
 * This register configuration will take effect when the reg_freq_choose_op_t has been set to the same frequency point.
 */
#define DDRP_FREQ_CHOOSE_WR_T                      0x054, 27, 26

/* [25:25]  RW  reset 0x0  --  reg_cmd_invdelay_lp_en
 * Reserved.
 */
#define DDRP_CMD_INVDELAY_LP_EN                    0x054, 25, 25

/* [24:24]  RW  reset 0x0  --  reg_dfi_clk_gate_bp
 * The disable signal of the clock gate for the dfi_clk1x_in in the flow of the fast frequency change.
 * 1: Disable the clock gate of the dfi_clk1x_in when the fast frequency changes after the dfi_init_start high and dfi_init_complete low.
 * 0: Enable the clock gate of the dfi_clk1x_in when the fast frequency changes after the dfi_init_start high and dfi_init_complete low. It will skip the glitch of the frequency change for dfi_clk1x_in.
 */
#define DDRP_DFI_CLK_GATE_BP                       0x054, 24, 24

/* [23:16]  RW  reset 0x40  --  reg_max_rdvalue
 * Set the max number of the read commands when doing the DQS Gating. When the read commands are more than the max number, it indicates the DQS Gating Error and this state can be read by the register calib_error.
 */
#define DDRP_MAX_RDVALUE                           0x054, 23, 16

/* [15:0]  RW  reset 0x1000  --  reg_calib_timeout
 * The bit[7:0] of the timer used for DQS Gating. When the clock cycle exceeds the bit[15:0] of the timer, it indicates the DQS Gating Error and this state can be read by the register calib_error.
 */
#define DDRP_CALIB_TIMEOUT                         0x054, 15,  0


/* ----- 0x058 ----- */
/* [29:29]  RW  reset 0x1  --  reg_pvt_comp_dis
 * The disable signal of the PVT compensation.
 * Active State: High.
 * 1: Disable the PVT compensation function.
 * 0: When the PVT compensation is triggered because the DLL lock value exceeds the threshold, it will
 * enable the PVT compensation control flow to generate the handshake signal between the PHY and MC. Then the PHY will update the DLL lock value at the self fresh state of the SDRAM.
 */
#define DDRP_PVT_COMP_DIS                          0x058, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_tx_lock_code_bp_en
 * The bypass enable signal of the DLL lock value of the Tx path.
 * 1: Choose to use the register reg_tx_lock_code_bp_value as the DLL lock value.
 * 0: Choose to use the auto DLL lock code as the DLL lock value.
 */
#define DDRP_TX_LOCK_CODE_BP_EN                    0x058, 28, 28

/* [27:27]  RW  reset 0x0  --  reg_rx_lock_code_bp_en
 * The bypass enable signal of the DLL lock value of the Rx path.
 * 1: Choose to use the register reg_rx_lock_code_bp_value as the DLL lock value.
 * 0: Choose to use the auto DLL lock code as the DLL lock value.
 */
#define DDRP_RX_LOCK_CODE_BP_EN                    0x058, 27, 27

/* [26:26]  RW  reset 0x0  --  reg_mdll_update_cnt_clear
 * The clear signal to the counter of the PVT compensaton trigger times.
 * Active State: High.
 * 1: Clear the counter. Need to recover it to 1’b0 after the setting.
 * 0: Keep the current state.
 */
#define DDRP_MDLL_UPDATE_CNT_CLEAR                 0x058, 26, 26

/* [25:25]  RW  reset 0x0  --  reg_phy_sdram_initial
 * PHY triggers SDRAM initialization for LPDDR4 CA training
 * Active State: High.
 */
#define DDRP_PHY_SDRAM_INITIAL                     0x058, 25, 25

/* [15:15]  RW  reset 0x1  --  reg_cat_rxodt_en
 * Set to 1 to open Rx ODT when doing CA training or write leveling.
 */
#define DDRP_CAT_RXODT_EN                          0x058, 15, 15

/* [14:14]  RW  reset 0x0  --  reg_rx_calib_use_rdc_cmd
 * When doing LPDDR4 calibration, set this register to 1’b1, then PHY will send MPC RD during calibration. Set to 1’b0, PHY will send normal RD during calibration.
 */
#define DDRP_RX_CALIB_USE_RDC_CMD                  0x058, 14, 14

/* [13:11]  RW  reset 0x2  --  reg_wrank_dig_1xdly
 * Reserved
 */
#define DDRP_WRANK_DIG_1XDLY                       0x058, 13, 11

/* [10:10]  RW  reset 0x1  --  reg_rrankdly_4x_dec
 * The delay control signal of the read rank switch.
 * Unit: dfi_clk4x.
 * 1: Decrease 1 dfi_clk4x delay for the read rank switch.
 * 0: Keep current state.
 */
#define DDRP_RRANKDLY_4X_DEC                       0x058, 10, 10

/* [9:8]  RW  reset 0x0  --  reg_rdrank_4xdly
 * Reserved.
 */
#define DDRP_RDRANK_4XDLY                          0x058,  9,  8

/* [7:7]  RW  reset 0x0  --  reg_rdrank_delay_bp
 * The bypass mode of read rank switch point at 4x clock cycles precision. You can use the registers reg_{a/b/…}_{l/h}_rrankdly_4x_cs0/1to adjust the read rank switch point.
 */
#define DDRP_RDRANK_DELAY_BP                       0x058,  7,  7

/* [6:4]  RW  reset 0x1  --  reg_rdrank_1xdly
 * Reserved.
 */
#define DDRP_RDRANK_1XDLY                          0x058,  6,  4

/* [2:0]  RW  reset 0x1  --  reg_wrrank_1xdly
 * The delay control signal of the write rank switch.
 * Unit: dfi_clk1x.
 */
#define DDRP_WRRANK_1XDLY                          0x058,  2,  0


/* ----- 0x05c ----- */
/* [31:24]  RW  reset 0x4  --  reg_mdll_chg_margin
 * The threshold value of the changing for the DLL lock value.
 * When the PVT compensation module finds the DLL lock value changing range exceeding or equaling the threshold vlaue, it will trigger the PVT compensation operaiton.
 */
#define DDRP_MDLL_CHG_MARGIN                       0x05c, 31, 24

/* [23:16]  RW  reset 0x0  --  reg_pvt_comp_req_wait_cnt
 * The max wait time of the dfi_phymstr_ack after the assertion of the dfi_phymstr_req.
 * If the MC can't assert the dfi_phymstr_ack this time, it will report the error for the PVT compensation and give up the PVT compensation operation.
 */
#define DDRP_PVT_COMP_REQ_WAIT_CNT                 0x05c, 23, 16

/* [15:8]  RW  reset 0x0  --  reg_tx_lock_code_bp_value
 * The lock value when enabling the bypass signal of the DLL lock value of the Tx path. Related to the register reg_tx_lock_code_bp_en.
 */
#define DDRP_TX_LOCK_CODE_BP_VALUE                 0x05c, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_rx_lock_code_bp_value
 * The lock value when enabling the bypass signal of the DLL lock value of the Rx path. Related to the register reg_rx_lock_code_bp_en.
 */
#define DDRP_RX_LOCK_CODE_BP_VALUE                 0x05c,  7,  0


/* ----- 0x060 ----- */
/* [29:25]  RW  reset 0x0  --  reg_cmd0_wrap_sel
 * The pin wrap select of pad A0 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD0_WRAP_SEL                         0x060, 29, 25

/* [24:20]  RW  reset 0x1  --  reg_cmd1_wrap_sel
 * The pin wrap select of pad A1 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD1_WRAP_SEL                         0x060, 24, 20

/* [19:15]  RW  reset 0x2  --  reg_cmd2_wrap_sel
 * The pin wrap select of pad A2 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD2_WRAP_SEL                         0x060, 19, 15

/* [14:10]  RW  reset 0x3  --  reg_cmd3_wrap_sel
 * The pin wrap select of pad A3 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD3_WRAP_SEL                         0x060, 14, 10

/* [9:5]  RW  reset 0x4  --  reg_cmd4_wrap_sel
 * The pin wrap select of pad A4 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD4_WRAP_SEL                         0x060,  9,  5

/* [4:0]  RW  reset 0x5  --  reg_cmd5_wrap_sel
 * The pin wrap select of pad A5 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD5_WRAP_SEL                         0x060,  4,  0


/* ----- 0x064 ----- */
/* [29:25]  RW  reset 0x6  --  reg_cmd6_wrap_sel
 * The pin wrap select of pad A6 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD6_WRAP_SEL                         0x064, 29, 25

/* [24:20]  RW  reset 0x7  --  reg_cmd7_wrap_sel
 * The pin wrap select of pad A7 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD7_WRAP_SEL                         0x064, 24, 20

/* [19:15]  RW  reset 0x8  --  reg_cmd8_wrap_sel
 * The pin wrap select of pad A8 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD8_WRAP_SEL                         0x064, 19, 15

/* [14:10]  RW  reset 0x9  --  reg_cmd9_wrap_sel
 * The pin wrap select of pad A9 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD9_WRAP_SEL                         0x064, 14, 10

/* [9:5]  RW  reset 0xa  --  reg_cmd10_wrap_sel
 * The pin wrap select of pad A10 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD10_WRAP_SEL                        0x064,  9,  5

/* [4:0]  RW  reset 0xb  --  reg_cmd11_wrap_sel
 * The pin wrap select of pad A11 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD11_WRAP_SEL                        0x064,  4,  0


/* ----- 0x068 ----- */
/* [29:25]  RW  reset 0xc  --  reg_cmd12_wrap_sel
 * The pin wrap select of pad A12 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD12_WRAP_SEL                        0x068, 29, 25

/* [24:20]  RW  reset 0xd  --  reg_cmd13_wrap_sel
 * The pin wrap select of pad A13 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD13_WRAP_SEL                        0x068, 24, 20

/* [19:15]  RW  reset 0xe  --  reg_cmd14_wrap_sel
 * The pin wrap select of pad A14 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD14_WRAP_SEL                        0x068, 19, 15

/* [14:10]  RW  reset 0xf  --  reg_cmd15_wrap_sel
 * The pin wrap select of pad A15 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD15_WRAP_SEL                        0x068, 14, 10

/* [9:5]  RW  reset 0x10  --  reg_cmd16_wrap_sel
 * The pin wrap select of pad A16 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD16_WRAP_SEL                        0x068,  9,  5

/* [4:0]  RW  reset 0x11  --  reg_cmd17_wrap_sel
 * The pin wrap select of pad A17 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD17_WRAP_SEL                        0x068,  4,  0


/* ----- 0x06c ----- */
/* [29:25]  RW  reset 0x12  --  reg_cmd18_wrap_sel
 * The pin wrap select of pad ACTN for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD18_WRAP_SEL                        0x06c, 29, 25

/* [24:20]  RW  reset 0x13  --  reg_cmd19_wrap_sel
 * The pin wrap select of pad BA0 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD19_WRAP_SEL                        0x06c, 24, 20

/* [19:15]  RW  reset 0x14  --  reg_cmd20_wrap_sel
 * The pin wrap select of pad BA1 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD20_WRAP_SEL                        0x06c, 19, 15

/* [14:10]  RW  reset 0x15  --  reg_cmd21_wrap_sel
 * The pin wrap select of pad BG0 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD21_WRAP_SEL                        0x06c, 14, 10

/* [9:5]  RW  reset 0x16  --  reg_cmd22_wrap_sel
 * The pin wrap select of pad BG1 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD22_WRAP_SEL                        0x06c,  9,  5

/* [4:0]  RW  reset 0x17  --  reg_cmd23_wrap_sel
 * The pin wrap select of pad CK for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD23_WRAP_SEL                        0x06c,  4,  0


/* ----- 0x070 ----- */
/* [29:25]  RW  reset 0x18  --  reg_cmd24_wrap_sel
 * The pin wrap select of pad CKB for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD24_WRAP_SEL                        0x070, 29, 25

/* [24:20]  RW  reset 0x19  --  reg_cmd25_wrap_sel
 * The pin wrap select of pad CKE0 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD25_WRAP_SEL                        0x070, 24, 20

/* [19:15]  RW  reset 0x1a  --  reg_cmd26_wrap_sel
 * The pin wrap select of pad CSB0 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD26_WRAP_SEL                        0x070, 19, 15

/* [14:10]  RW  reset 0x1b  --  reg_cmd27_wrap_sel
 * The pin wrap select of pad CSB1 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD27_WRAP_SEL                        0x070, 14, 10

/* [9:5]  RW  reset 0x1c  --  reg_cmd28_wrap_sel
 * The pin wrap select of pad ODT0 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD28_WRAP_SEL                        0x070,  9,  5

/* [4:0]  RW  reset 0x1d  --  reg_cmd29_wrap_sel
 * The pin wrap select of pad ODT1 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD29_WRAP_SEL                        0x070,  4,  0


/* ----- 0x074 ----- */
/* [29:25]  RW  reset 0x1e  --  reg_cmd30_wrap_sel
 * The pin wrap select of pad CKE1 for DDRn/LPDDRn.
 * Refer to User-Defined CMD Map section in the Databook.
 */
#define DDRP_CMD30_WRAP_SEL                        0x074, 29, 25

/* [15:0]  RW  reset 0x3e8  --  reg_max_bist_cnt
 * The max cnt for prbs bist
 */
#define DDRP_MAX_BIST_CNT                          0x074, 15,  0


/* ----- 0x078 ----- */
/* [11:4]  RW  reset 0x0  --  reg_mr22_rank1
 * Control the LPDDR4 rank1 load mode value of MR22 when doing the command bus training.
 */
#define DDRP_MR22_RANK1                            0x078, 11,  4

/* [3:0]  RW  reset 0x8  --  reg_byte8_wrap_sel
 * The wrap select signal of BYTE8.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE8_WRAP_SEL                        0x078,  3,  0


/* ----- 0x07c ----- */
/* [31:28]  RW  reset 0x7  --  reg_byte7_wrap_sel
 * The wrap select signal of BYTE7.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE7_WRAP_SEL                        0x07c, 31, 28

/* [27:24]  RW  reset 0x6  --  reg_byte6_wrap_sel
 * The wrap select signal of BYTE6.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE6_WRAP_SEL                        0x07c, 27, 24

/* [23:20]  RW  reset 0x5  --  reg_byte5_wrap_sel
 * The wrap select signal of BYTE5.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE5_WRAP_SEL                        0x07c, 23, 20

/* [19:16]  RW  reset 0x4  --  reg_byte4_wrap_sel
 * The wrap select signal of BYTE4.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE4_WRAP_SEL                        0x07c, 19, 16

/* [15:12]  RW  reset 0x3  --  reg_byte3_wrap_sel
 * The wrap select signal of BYTE3.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE3_WRAP_SEL                        0x07c, 15, 12

/* [11:8]  RW  reset 0x2  --  reg_byte2_wrap_sel
 * The wrap select signal of BYTE2.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE2_WRAP_SEL                        0x07c, 11,  8

/* [7:4]  RW  reset 0x1  --  reg_byte1_wrap_sel
 * The wrap select signal of BYTE1.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE1_WRAP_SEL                        0x07c,  7,  4

/* [3:0]  RW  reset 0x0  --  reg_byte0_wrap_sel
 * The wrap select signal of BYTE0.
 * Refer to DQ Byte Map in the Databook.
 */
#define DDRP_BYTE0_WRAP_SEL                        0x07c,  3,  0


/* ----- 0x080 ----- */
/* [31:0]  RW  reset 0x8101  --  reg_cke_ck_cmd_pad_t
 * The PHY supports the command remap, CK/CKE may remap to the other pads, so you can use this register to tell the PHY which pad is CK/CKE. For the CK/CKE output pad, the bit should be set to 1’b1.
 * [0]: A0
 * [1]: A1
 * [2]: A2
 * [3]: A3
 * [4]: A4
 * [5]: A5
 * [6]: A6
 * [7]: A7
 * [8]: A8
 * [9]: A9
 * [10]: A10
 * [11]: A11
 * [12]: A12
 * [13]: A13
 * [14]: A14
 * [15]: A15
 * [16]: A16
 * [17]: A17
 * [18]: ACTN
 * [19]: BA0
 * [20]: BA1
 * [21]: BG0
 * [22]: BG1
 * [23]: CK
 * [24]: CKB
 * [25]: CKE0
 * [26]: CSB0
 * [27]: CSB1
 * [28]: ODT0
 * [29]: ODT1
 * [30]: CKE1
 * [31]: RESETN
 */
#define DDRP_CKE_CK_CMD_PAD_T                      0x080, 31,  0


/* ----- 0x084 ----- */
/* [24:16]  RW  reset 0x0  --  reg_pllfbdiv_dqcmd
 * The control signal of feedback divider signal.
 */
#define DDRP_PLLFBDIV_DQCMD                        0x084, 24, 16

/* [8:4]  RW  reset 0x1  --  reg_pllprediv_dqcmd
 * Internal PLL charge bump work frequency adjustment, used to adjust PLL bandwidth, and the maximum bandwidth is 5’h1.
 * Refer to PLL section in the Databook.
 */
#define DDRP_PLLPREDIV_DQCMD                       0x084,  8,  4

/* [3:3]  RW  reset 0x1  --  reg_pllpostdiven_ls
 * Post-divider enable for CAT low frequency.
 * Active State: High
 */
#define DDRP_PLLPOSTDIVEN_LS                       0x084,  3,  3

/* [2:0]  RW  reset 0x1  --  reg_pllpostdiv_ls
 * Internal PLL post-divider parameter setting register. Set this register for different work frequencies to make PLL VCO at proper range. This register can be adjusted under condition of reg_pllpostdiven_ls = 1’b1 (post-divider enable is high).
 * Refer to PLL section in the Databook.
 */
#define DDRP_PLLPOSTDIV_LS                         0x084,  2,  0


/* ----- 0x088 ----- */
/* [27:20]  RW  reset 0x0  --  reg_invdelaysel_osc
 * Reserved.
 */
#define DDRP_INVDELAYSEL_OSC                       0x088, 27, 20

/* [19:18]  RW  reset 0x0  --  reg_plltestsel_dqcmd
 * Reserved.
 */
#define DDRP_PLLTESTSEL_DQCMD                      0x088, 19, 18

/* [17:16]  RW  reset 0x1  --  reg_pllgvco_bias_dqcmd
 * Reserved.
 */
#define DDRP_PLLGVCO_BIAS_DQCMD                    0x088, 17, 16

/* [14:12]  RW  reset 0x2  --  reg_pllcpi_bias_ls
 * Reserved.
 */
#define DDRP_PLLCPI_BIAS_LS                        0x088, 14, 12

/* [10:8]  RW  reset 0x2  --  reg_pllcpp_bias_dqcmd
 * Reserved.
 */
#define DDRP_PLLCPP_BIAS_DQCMD                     0x088, 10,  8

/* [7:7]  RW  reset 0x0  --  reg_plltestouten_dqcmd
 * Reserved.
 */
#define DDRP_PLLTESTOUTEN_DQCMD                    0x088,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_lockenb_dqcmd
 * The enable signal of the PLL.
 * 0: Enable the PLL.
 * 1: Disable the PLL.
 */
#define DDRP_LOCKENB_DQCMD                         0x088,  6,  6

/* [5:5]  RW  reset 0x0  --  reg_pllrstbsel_dqcmd
 * Reserved.
 */
#define DDRP_PLLRSTBSEL_DQCMD                      0x088,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_pllref_clk_byp_dqcmd
 * Reserved.
 */
#define DDRP_PLLREF_CLK_BYP_DQCMD                  0x088,  4,  4

/* [3:3]  RW  reset 0x1  --  reg_ssc_rstn
 * Reserved.
 */
#define DDRP_SSC_RSTN                              0x088,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_pllincz_dqcmd
 * Reserved.
 */
#define DDRP_PLLINCZ_DQCMD                         0x088,  2,  2

/* [1:1]  RW  reset 0x1  --  reg_pllclkouten_dqcmd_t
 * The clock output enable signal of PLL.
 * Active State: High.
 */
#define DDRP_PLLCLKOUTEN_DQCMD_T                   0x088,  1,  1

/* [0:0]  RW  reset 0x1  --  reg_pllpd_dqcmd_t
 * Internal PLL power down. Set to “0” to power down the PLL.
 * Active State: Low.
 */
#define DDRP_PLLPD_DQCMD_T                         0x088,  0,  0


/* ----- 0x08c ----- */
/* [31:31]  RW  reset 0x0  --  reg_hclk_train_sel
 * Choose the clock state of training logic.
 * 1: Keep clock for training logic when it is in no-training state.
 * 0: Gate clock for training logic when it is in no-training state.
 */
#define DDRP_HCLK_TRAIN_SEL                        0x08c, 31, 31

/* [30:30]  RW  reset 0x0  --  reg_hclk_bist_sel
 * The clock gate control signal of the BIST module.
 * 1: Keep the clock of the BIST module even if the module is not in working state.
 * 0: Automatically disable the clock of the BIST module when the module is not in working state.
 */
#define DDRP_HCLK_BIST_SEL                         0x08c, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_hclk_zqcalib_sel
 * The clock gate control signal of the ZQ calibration module.
 * 1: Keep the clock of the ZQ calibration module even if the module is not in the training mode.
 * 0: Automatically disable the clock of the ZQ calibration module when the module is not in the training module.
 */
#define DDRP_HCLK_ZQCALIB_SEL                      0x08c, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_hclk_byte7_sel
 * The enable signal of the clock gate for the digital part of BYTE7.
 * 1: Keep the clock when the BYTE7 is in idle state.
 * 0: Automatically gate the clock when the BYTE7 is in idle state.
 */
#define DDRP_HCLK_BYTE7_SEL                        0x08c, 28, 28

/* [27:27]  RW  reset 0x0  --  reg_hclk_byte6_sel
 * The enable signal of the clock gate for the digital part of BYTE6.
 * 1: Keep the clock when the BYTE6 is in idle state.
 * 0: Automatically gate the clock when the BYTE6 is in idle state.
 */
#define DDRP_HCLK_BYTE6_SEL                        0x08c, 27, 27

/* [26:26]  RW  reset 0x0  --  reg_hclk_byte5_sel
 * The enable signal of the clock gate for the digital part of BYTE5.
 * 1: Keep the clock when the BYTE5 is in idle state.
 * 0: Automatically gate the clock when the BYTE5 is in idle state.
 */
#define DDRP_HCLK_BYTE5_SEL                        0x08c, 26, 26

/* [25:25]  RW  reset 0x0  --  reg_hclk_byte4_sel
 * The enable signal of the clock gate for the digital part of BYTE4.
 * 1: Keep the clock when the BYTE4 is in idle state.
 * 0: Automatically gate the clock when the BYTE4 is in idle state.
 */
#define DDRP_HCLK_BYTE4_SEL                        0x08c, 25, 25

/* [24:24]  RW  reset 0x0  --  reg_hclk_byte3_sel
 * The enable signal of the clock gate for the digital part of BYTE3.
 * 1: Keep the clock when the BYTE3 is in idle state.
 * 0: Automatically gate the clock when the BYTE3 is in idle state.
 */
#define DDRP_HCLK_BYTE3_SEL                        0x08c, 24, 24

/* [23:23]  RW  reset 0x0  --  reg_hclk_byte2_sel
 * The enable signal of the clock gate for the digital part of BYTE2.
 * 1: Keep the clock when the BYTE2 is in idle state.
 * 0: Automatically gate the clock when the BYTE2 is in idle state.
 */
#define DDRP_HCLK_BYTE2_SEL                        0x08c, 23, 23

/* [22:22]  RW  reset 0x0  --  reg_hclk_byte1_sel
 * The enable signal of the clock gate for the digital part of BYTE1.
 * 1: Keep the clock when the BYTE1 is in idle state.
 * 0: Automatically gate the clock when the BYTE1 is in idle state.
 */
#define DDRP_HCLK_BYTE1_SEL                        0x08c, 22, 22

/* [21:21]  RW  reset 0x0  --  reg_hclk_byte0_sel
 * The enable signal of the clock gate for the digital part of BYTE0.
 * 1: Keep the clock when the BYTE0 is in idle state.
 * 0: Automatically gate the clock when the BYTE0 is in idle state.
 */
#define DDRP_HCLK_BYTE0_SEL                        0x08c, 21, 21

/* [20:20]  RW  reset 0x0  --  reg_hclk_byte_sel
 * The clock gate control signal of the DQ path common logic.
 * 1: Keep the clock of the common logic at idle state.
 * 0: Automatically disable the common logic at idle state.
 */
#define DDRP_HCLK_BYTE_SEL                         0x08c, 20, 20

/* [19:19]  RW  reset 0x0  --  reg_hclk_ca_sel
 * Choose the clock state of command logic.
 * 1: Keep clock of command logic when it is in idle state.
 * 0: Gate clock of command logic when it is in idle state.
 */
#define DDRP_HCLK_CA_SEL                           0x08c, 19, 19

/* [18:18]  RW  reset 0x1  --  reg_train_reg_update_en
 * Enable the clock gate of the training logic.
 * 1: Enable.
 * 0: Automatically gate the clock when the training logic is in idle state.
 * If you want to use the register to update the training result through the bypass mode, set the register to 1’b1 to enable the clock of the training control logic, then the bypass value can be updated to the clock register.
 */
#define DDRP_TRAIN_REG_UPDATE_EN                   0x08c, 18, 18

/* [17:17]  RW  reset 0x1  --  reg_dqclken_t
 * The enable signal of clock for DQ path in the analog circuit. This signal will be used to disable the CLK of the DQ module to save the power when the DDR2/3/4 SDRAM goes to power down mode.
 * 1: Keep the clock of the DQ path in the analog circuit.
 * 0: Gate the clock of the DQ path in the analog circuit.
 */
#define DDRP_DQCLKEN_T                             0x08c, 17, 17

/* [16:16]  RW  reset 0x1  --  reg_outclken
 * The gating control signal of 1x clock of PHY.
 * 0: Gate the 1x clock of the PHY.
 * 1: Automatically gate the 1x clock of the PHY in DFI low power mode.
 */
#define DDRP_OUTCLKEN                              0x08c, 16, 16

/* [15:15]  RW  reset 0x0  --  reg_lp_bypass
 * The DFI low power disable signal.
 * Active State: High
 * When it is set to high, it will ignore the signals of DFI low power interface.
 */
#define DDRP_LP_BYPASS                             0x08c, 15, 15

/* [14:14]  RW  reset 0x1  --  reg_deep_lp_en
 * The deep low power mode enable signal.
 * 1: Enable the deep low power.
 * 0: Disable the deep low power.
 */
#define DDRP_DEEP_LP_EN                            0x08c, 14, 14

/* [13:13]  RW  reset 0x0  --  reg_lp_wakeup_sel
 * Reserved
 */
#define DDRP_LP_WAKEUP_SEL                         0x08c, 13, 13

/* [12:12]  RW  reset 0x1  --  reg_lp_vref_ctrl_en
 * Disable the bypass mode of the VREF control.
 * 1: Using auto mode to disable VREF at DFI low power mode.
 * 0: Using the register reg_{a/b/…}_{l/h}_vref1_pd_reg to disable the VREF.
 */
#define DDRP_LP_VREF_CTRL_EN                       0x08c, 12, 12

/* [11:8]  RW  reset 0x9  --  reg_lp_wakeup_threhold
 * Configure the low power mode threshold.
 * When the dfi_lp_wakeup <= threshold, it will go into the normal low power mode;
 * When dfi_lp_wakeup > threshold, it will go into the deep low power mode.
 */
#define DDRP_LP_WAKEUP_THREHOLD                    0x08c, 11,  8

/* [7:6]  RW  reset 0x2  --  reg_lp_io_dis_ctrl
 * Adjust the timing from disabling the IO to disabling the clock when the low power flow starts.
 */
#define DDRP_LP_IO_DIS_CTRL                        0x08c,  7,  6

/* [5:5]  RW  reset 0x1  --  reg_lp_dq_clk_ctrl_en
 * Choose the control mode of the DQ clock path when in low power mode.
 * 1: Auto mode.
 * 0: Bypass mode. Use the register reg_dqclken_t to control.
 */
#define DDRP_LP_DQ_CLK_CTRL_EN                     0x08c,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_lp_dig_rst_ctrl_en
 * Choose the control mode of the reset of the digital core.
 * 1: Auto mode.
 * 0: Fix mode.
 */
#define DDRP_LP_DIG_RST_CTRL_EN                    0x08c,  4,  4

/* [3:3]  RW  reset 0x1  --  reg_lp_spll_clktree_ctrl_en
 * Choose the control mode of the clock of the PLL.
 * 1: Auto mode.
 * 0: Fix mode.
 */
#define DDRP_LP_SPLL_CLKTREE_CTRL_EN               0x08c,  3,  3

/* [2:2]  RW  reset 0x1  --  reg_lp_dig_clk_ctrl_en
 * Choose the control mode of the clock of the digital core.
 * 1: Auto mode.
 * 0: Fix mode.
 */
#define DDRP_LP_DIG_CLK_CTRL_EN                    0x08c,  2,  2

/* [1:1]  RW  reset 0x1  --  reg_lp_io_ctrl_en
 * Choose the control mode of the IO.
 * 1: Auto mode.
 * 0: Fix mode.
 */
#define DDRP_LP_IO_CTRL_EN                         0x08c,  1,  1

/* [0:0]  RW  reset 0x1  --  reg_lp_pllpd_ctrl_en
 * Choose the power down mode of the PLL.
 * 1: Auto mode.
 * 0: Fix mode.
 */
#define DDRP_LP_PLLPD_CTRL_EN                      0x08c,  0,  0


/* ----- 0x090 ----- */
/* [31:31]  RW  reset 0x0  --  reg_hclk_byte8_sel
 * The enable signal of the clock gate for the digital part of  BYTE8.
 * 1: Keep the clock when the BYTE8 is in idle state.
 * 0: Automatically gate the clock when the BYTE8 is in idle state.
 */
#define DDRP_HCLK_BYTE8_SEL                        0x090, 31, 31

/* [23:20]  RW  reset 0x1  --  reg_lp_stvalue
 * The response timing when pulling up the dfi_lp_ack.
 * Unit: dfi_clk1x.
 */
#define DDRP_LP_STVALUE                            0x090, 23, 20

/* [19:16]  RW  reset 0x6  --  reg_lp_ackvalue
 * The response timing when detecting the dfi_lp_req is high.
 * Unit: dfi_clk1x.
 */
#define DDRP_LP_ACKVALUE                           0x090, 19, 16

/* [15:0]  RW  reset 0x1388  --  reg_wait_cnt
 * The bit[7:0] of PLL lock wait signal.
 * There are two ways to generate the PLL lock. The first way is using the pll_lock from the PLL module directly; the second way is using the counter to calcuate the cycle after enabling the PLL.
 * For the second way, after enabling the PLL, the PHY will start to count until the counter equals the reg_wait_cnt, and then the PLL is locked.
 */
#define DDRP_WAIT_CNT                              0x090, 15,  0


/* ----- 0x094 ----- */
/* [30:30]  RW  reset 0x0  --  reg_zqcali_type_sel
 * 0: DDR2345/LPDDR23
 * 1: LPDDR4/5/4x/5x
 */
#define DDRP_ZQCALI_TYPE_SEL                       0x094, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_lpddr4x_zqcal
 * When LPDDR4x is chosen, this bit should be set to 1’b1.
 */
#define DDRP_LPDDR4X_ZQCAL                         0x094, 29, 29

/* [28:20]  RW  reset 0xff  --  reg_zq_chg_interval
 * The timing interval of the ZQ calibration training value changing when doing the ZQ calibration.
 * Unit: dfi_clk1x
 */
#define DDRP_ZQ_CHG_INTERVAL                       0x094, 28, 20

/* [18:10]  RW  reset 0xff  --  reg_pu_interval
 * The timing interval of the ZQ calibration between the pull-down training state to pull-up training state.
 * Unit: dfi_clk1x.
 */
#define DDRP_PU_INTERVAL                           0x094, 18, 10

/* [5:5]  RW  reset 0x0  --  reg_zqcali_chopen1_sel
 * The select signal of zqcalib chopen mode. High active.
 */
#define DDRP_ZQCALI_CHOPEN1_SEL                    0x094,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_zqcali_chopen0_sel
 * The select signal of zqcalib chopen mode. High active.
 */
#define DDRP_ZQCALI_CHOPEN0_SEL                    0x094,  4,  4

/* [3:3]  RW  reset 0x1  --  reg_pd_zqcali
 * The power down signal of ZQ calibration module when the ZQ calibration function is enabled.
 * 1: Power down mode.
 * 0: Normal work mode.
 */
#define DDRP_PD_ZQCALI                             0x094,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_zqcali_clear
 * The clear signal of ZQ calibration module.
 * Active State: High
 */
#define DDRP_ZQCALI_CLEAR                          0x094,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_zqcali_bypass
 * The bypass enable signal of ZQ calibration function.
 * Active State: High.
 * 1: The PHY will use the register reg_drvlegpu_zqcali/reg_drvlegpd_zqcali/reg_odtlegpu_zqcali/reg_odtlegpd_zqcali to control ZQ calibration.
 * 0: It will use the ZQ calibration module to control.
 */
#define DDRP_ZQCALI_BYPASS                         0x094,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_zqcali_en
 * The enable signal of ZQ calibration function.
 * Active State: High.
 */
#define DDRP_ZQCALI_EN                             0x094,  0,  0


/* ----- 0x098 ----- */
/* [24:16]  RW  reset 0x1ff  --  reg_max_dm_tx_scan_range
 * The max scan range of dm tx invdelay.
 */
#define DDRP_MAX_DM_TX_SCAN_RANGE                  0x098, 24, 16

/* [15:8]  RW  reset 0x0  --  reg_drvpd_zqcali_vref_sel
 * Reserved.
 */
#define DDRP_DRVPD_ZQCALI_VREF_SEL                 0x098, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_drvpu_zqcali_vref_sel
 * Reserved.
 */
#define DDRP_DRVPU_ZQCALI_VREF_SEL                 0x098,  7,  0


/* ----- 0x09c ----- */
/* [28:24]  RW  reset 0x0  --  reg_odtlegpu_zqcali
 * The ODT pull-up value when reg_zqcali_bypass is set to 1’b1 to enable the ZQ calibration bypass function.
 */
#define DDRP_ODTLEGPU_ZQCALI                       0x09c, 28, 24

/* [20:16]  RW  reset 0x0  --  reg_odtlegpd_zqcali
 * The ODT pull-down value when reg_zqcali_bypass is set to 1’b1 to enable the ZQ calibration bypass function.
 */
#define DDRP_ODTLEGPD_ZQCALI                       0x09c, 20, 16

/* [15:8]  RW  reset 0x0  --  reg_odtpd_zqcali_vref_sel
 * Reserved.
 */
#define DDRP_ODTPD_ZQCALI_VREF_SEL                 0x09c, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_odtpu_zqcali_vref_sel
 * Reserved.
 */
#define DDRP_ODTPU_ZQCALI_VREF_SEL                 0x09c,  7,  0


/* ----- 0x0a0 ----- */
/* [28:24]  RW  reset 0x0  --  reg_drvlegpu_zqcali
 * The driver pull-up value when reg_zqcali_bypass is set to 1’b1 to enable the ZQ calibration bypass function.
 */
#define DDRP_DRVLEGPU_ZQCALI                       0x0a0, 28, 24

/* [20:16]  RW  reset 0x0  --  reg_drvlegpd_zqcali
 * The driver pull-down value when reg_zqcali_bypass is set to 1’b1 to enable the ZQ calibration bypass function.
 */
#define DDRP_DRVLEGPD_ZQCALI                       0x0a0, 20, 16

/* [8:0]  RW  reset 0x1ff  --  reg_max_dq_tx_scan_range
 * The max scan range of dq tx invdelay.
 */
#define DDRP_MAX_DQ_TX_SCAN_RANGE                  0x0a0,  8,  0


/* ----- 0x0a4 ----- */
/* [31:31]  RW  reset 0x0  --  reg_ddrphy_rtrain_en
 * Reserved.
 */
#define DDRP_DDRPHY_RTRAIN_EN                      0x0a4, 31, 31

/* [30:24]  RW  reset 0x3f  --  reg_rd_train_dq_scan_max
 * The DQ scan max for read training. The read training will stop the scan when the delay line reaches the max value.
 */
#define DDRP_RD_TRAIN_DQ_SCAN_MAX                  0x0a4, 30, 24

/* [22:16]  RW  reset 0x3f  --  reg_rd_train_dqs_scan_max
 * The DQS scan max for read training. The read training will stop the scan when the delay line reaches the max value.
 */
#define DDRP_RD_TRAIN_DQS_SCAN_MAX                 0x0a4, 22, 16

/* [12:12]  RW  reset 0x0  --  reg_rd_train_perdef_en
 * The enable signal of the read predefined training.
 * 1: Not supported in this design
 * 0: Disable
 */
#define DDRP_RD_TRAIN_PERDEF_EN                    0x0a4, 12, 12

/* [11:11]  RW  reset 0x0  --  reg_train_vref_en
 * The enable signal of the Vref training.
 * 1: Not supported in this design
 * 0: Disable
 */
#define DDRP_TRAIN_VREF_EN                         0x0a4, 11, 11

/* [10:10]  RW  reset 0x0  --  reg_ddr4_dbi
 * When the read DBI function is enabled, and you want to perform write training with DBI open, set this register to 1’b1 at first.
 * 1: Support the DBI function during write training
 * 0: Not support the DBI function during write training
 */
#define DDRP_DDR4_DBI                              0x0a4, 10, 10

/* [9:8]  RW  reset 0x0  --  reg_rdtrain_cs_sel
 * Choose the rank of the read training.
 * 10: Rank0.
 * 01: Rank1.
 */
#define DDRP_RDTRAIN_CS_SEL                        0x0a4,  9,  8

/* [7:7]  RW  reset 0x0  --  reg_rx_vref_value_update
 * Update the PHY’s Vref value.
 * Active State: High
 * At posedge of this register, the value of reg_{a/b}_{l/h}_vref1_margsel will be latched to PHY.
 * At negedge of this register, the Verf value will keep the system preset value.
 * Refer to Read Training in the databook.
 */
#define DDRP_RX_VREF_VALUE_UPDATE                  0x0a4,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_rd_train_dqs_range_bypass
 * The enable signal to set the DQS scan range using the register. (Read training)
 * 1: Enable to use the register to set the DQS scan range.
 * 0: Use the default DQS scan range[0~6’h3f].
 */
#define DDRP_RD_TRAIN_DQS_RANGE_BYPASS             0x0a4,  6,  6

/* [5:5]  RW  reset 0x0  --  reg_bypass_rd_train_cmd_start_en
 * Generate the read training command when the read training bypass mode function is enabled.
 * Active State: High.
 */
#define DDRP_BYPASS_RD_TRAIN_CMD_START_EN          0x0a4,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_bypass_rd_train_en
 * Enable the read training bypass mode.
 * 1: Enable.
 * 0: Disable.
 */
#define DDRP_BYPASS_RD_TRAIN_EN                    0x0a4,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_rd_train_check_value_en
 * Choose the check value of the read training.
 * 0: Use the fix mode.
 * DDR3 = 8’b10101010
 * LPDDR3 = 16’hcc55
 * LPDDR4 = {MR#40, MR#32}
 * DDR4 = 8’haa/8’hcc/8’hf0/8’h00
 * 1: Use registers to configure.
 */
#define DDRP_RD_TRAIN_CHECK_VALUE_EN               0x0a4,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_rd_train_freq_update
 * Change the PHY’s Rx per-bit skew value.
 * At posedge of this signal, the value of reg_*_invdelayselrx will be latched to PHY.
 */
#define DDRP_RD_TRAIN_FREQ_UPDATE                  0x0a4,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_dqs_rd_train_en
 * The enable signal of the DQS scan.
 * 1: The auto read training will complete the DQ per-bit skew training and DQS-DQ Eye training. (Not suggested)
 * 0: The auto read training will only complete the DQ per-bit skew training.
 */
#define DDRP_DQS_RD_TRAIN_EN                       0x0a4,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_dq_rd_train_en
 * The enable signal of the read training auto mode.
 * 1: Enable
 * 0: Exit
 */
#define DDRP_DQ_RD_TRAIN_EN                        0x0a4,  0,  0


/* ----- 0x0a8 ----- */
/* [31:24]  RW  reset 0x55  --  reg_lpddr4_mr15_value
 * The load mode value of MR15 when doing the read training to change the check pattern of LPDDR4.
 */
#define DDRP_LPDDR4_MR15_VALUE                     0x0a8, 31, 24

/* [23:16]  RW  reset 0x55  --  reg_lpddr4_mr20_value
 * The load mode value of MR20 when doing the read training to change the check pattern of LPDDR4.
 */
#define DDRP_LPDDR4_MR20_VALUE                     0x0a8, 23, 16

/* [15:8]  RW  reset 0x5a  --  reg_lpddr4_mr32_value
 * The load mode value of MR32 when doing the read training to change the check pattern of LPDDR4.
 */
#define DDRP_LPDDR4_MR32_VALUE                     0x0a8, 15,  8

/* [7:0]  RW  reset 0x3c  --  reg_lpddr4_mr40_value
 * The load mode value of MR40 when doing the read training to change the check pattern of LPDDR4.
 */
#define DDRP_LPDDR4_MR40_VALUE                     0x0a8,  7,  0


/* ----- 0x0ac ----- */
/* [31:16]  RW  reset 0x0  --  reg_ddr4_mr4_value
 * The load mode value of MR4 when doing the read training of DDR4.
 */
#define DDRP_DDR4_MR4_VALUE                        0x0ac, 31, 16

/* [15:8]  RW  reset 0x3f  --  reg_ddr4_preamble_cnt
 * The time counter between the last precharge of MPR DQS calibration and read preamble training disabling.
 */
#define DDRP_DDR4_PREAMBLE_CNT                     0x0ac, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_ddr4_mr3
 * The load mode value of MR3 when doing the read training of DDR4.
 */
#define DDRP_DDR4_MR3                              0x0ac,  7,  0


/* ----- 0x0b0 ----- */
/* [25:25]  RW  reset 0x0  --  reg_dm_wr_train_en
 * The enable signal of the DM write training after the DQ write training.
 * 1: Enable
 * 0: Disable
 */
#define DDRP_DM_WR_TRAIN_EN                        0x0b0, 25, 25

/* [24:24]  RW  reset 0x1  --  reg_wrtrain_lpddr4_vref_range
 * Choose the adjust range of SDRAM's Vref.
 * Refer to JESD-LPDDR4
 */
#define DDRP_WRTRAIN_LPDDR4_VREF_RANGE             0x0b0, 24, 24

/* [23:15]  RW  reset 0x0  --  reg_pbit_deskew_offset_for_lpddr4
 * The Tx delay line adjust signal of the DQS/DQSB after the write-leveling.
 * The Tx delay line value = write-leveling result + reg_pbit_deskew_offset_for_lpddr4 and apply to the DQ/DM.
 */
#define DDRP_PBIT_DESKEW_OFFSET_FOR_LPDDR4         0x0b0, 23, 15

/* [9:9]  RW  reset 0x0  --  reg_dqs_wr_train_en
 * The enable signal of the DQS Scan.
 * Active State: High
 * When this bit is set to “1”, it will enable adjusting the DQS to find the best window of the DQ. Only used for debug, because it will change the write-leveling result if the write leveling function is enabled.
 */
#define DDRP_DQS_WR_TRAIN_EN                       0x0b0,  9,  9

/* [8:8]  RW  reset 0x0  --  reg_wrtrain_check_data_value_random_gen
 * 1’b1: PHY generates random data and writes it to SDRAM during write training.
 * 1’b0: Configure dq{0..7}_train_check_data_value{0..9} and PHY writes it (10*BL8) to SDRAM during write training.
 */
#define DDRP_WRTRAIN_CHECK_DATA_VALUE_RANDOM_GEN   0x0b0,  8,  8

/* [7:6]  RW  reset 0x0  --  reg_wrtrain_cs_sel
 * Choose the rank of the write training.
 * 2’b10: Rank0.
 * 2’b01: Rank1.
 */
#define DDRP_WRTRAIN_CS_SEL                        0x0b0,  7,  6

/* [5:5]  RW  reset 0x0  --  reg_wr_train_rst
 * The write training FSM clear signal.
 * 1: Reset the write training FSM.
 * 0: Keep the current state.
 */
#define DDRP_WR_TRAIN_RST                          0x0b0,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_wr_train_dqs_default_bypass
 * Choose the DQS default value when enabling the write training (The DQS scan mode is disabled).
 * 0: Use the write-leveling value.
 * 1: Use the register to choose the default value.
 */
#define DDRP_WR_TRAIN_DQS_DEFAULT_BYPASS           0x0b0,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_wr_train_dqs_range_bypass
 * The enable signal to set the DQS scan range using the register. (write training)
 * 1: Enable to use the register to set the DQS scan range.
 * 0: Use the default DQS scan range[0~6’h3f].
 */
#define DDRP_WR_TRAIN_DQS_RANGE_BYPASS             0x0b0,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_wr_train_freq_update
 * Reserved
 */
#define DDRP_WR_TRAIN_FREQ_UPDATE                  0x0b0,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_dq_wr_train_en
 * The enable signal of the write training Auto Mode.
 * 1: Enable
 * 0: Exit
 */
#define DDRP_DQ_WR_TRAIN_EN                        0x0b0,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_dq_wr_train_auto
 * Choose the mode of the write training.
 * 1: Auto mode (set it to 1’b1 before write training)
 * 0: Disable the write training
 */
#define DDRP_DQ_WR_TRAIN_AUTO                      0x0b0,  0,  0


/* ----- 0x0b4 ----- */
/* [30:28]  RW  reset 0x0  --  reg_wr_train_ba_addr
 * The bit [2:0] of bank address for write/read operation in DDR3/4 and LPDDR3 write training mode. (Reused for predefined read training and write training)
 * Reserved for LPDDR4.
 */
#define DDRP_WR_TRAIN_BA_ADDR                      0x0b4, 30, 28

/* [25:16]  RW  reset 0x0  --  reg_wr_train_col_addr
 * The bit [9:0] of column address for write/read operation in DDR3/4 and LPDDR3 write training mode. (Reused for predefined read training and write training)
 * Reserved for LPDDR4.
 */
#define DDRP_WR_TRAIN_COL_ADDR                     0x0b4, 25, 16

/* [15:0]  RW  reset 0x0  --  reg_wr_train_row_addr
 * The bit [15:0] of row address for write/read operation in DDR3/4 and LPDDR3 write training mode. (Reused for predefined read training and write training)
 * Reserved for LPDDR4.
 */
#define DDRP_WR_TRAIN_ROW_ADDR                     0x0b4, 15,  0


/* ----- 0x0b8 ----- */
/* [31:18]  RW  reset 0x960  --  reg_phy_trefi
 * The value to control the timing of the Trefi when the PHY is in training mode.
 * Unit: dfi_clk1x.
 */
#define DDRP_PHY_TREFI                             0x0b8, 31, 18

/* [17:8]  RW  reset 0x8c  --  reg_phy_trfc
 * Control the auto-refresh interval when the PHY performs the training.
 * The auto-refresh interval = reg_phy_trfc * Tdfi_clk1x.
 */
#define DDRP_PHY_TRFC                              0x0b8, 17,  8

/* [7:4]  RW  reset 0x8  --  reg_max_refi_cnt
 * Indicate how many auto refresh commands can be accumulated at one time after reg_phy_refresh_en is set to 1’b1.
 * The maximum is 8, and the larger the value is, the less time training takes.
 */
#define DDRP_MAX_REFI_CNT                          0x0b8,  7,  4

/* [1:1]  RW  reset 0x1  --  reg_phy_max_refi_enable
 * Enable to use the reg_max_refi_cnt register to control the max refresh times.
 * Active State: Low
 */
#define DDRP_PHY_MAX_REFI_ENABLE                   0x0b8,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_phy_refresh_en
 * The enable signal of PHY auto refresh function in training.
 * 1: Enable
 * 0: Disable
 */
#define DDRP_PHY_REFRESH_EN                        0x0b8,  0,  0


/* ----- 0x0bc ----- */
/* [31:31]  RW  reset 0x0  --  reg_a0_lp4x_en
 * The LPDDR4X enable mode of the A0.
 * Active State: High
 */
#define DDRP_A0_LP4X_EN                            0x0bc, 31, 31

/* [30:30]  RW  reset 0x0  --  reg_a1_lp4x_en
 * The LPDDR4X enable mode of the A1.
 * Active State: High
 */
#define DDRP_A1_LP4X_EN                            0x0bc, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_a2_lp4x_en
 * The LPDDR4X enable mode of the A2.
 * Active State: High
 */
#define DDRP_A2_LP4X_EN                            0x0bc, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_a3_lp4x_en
 * The LPDDR4X enable mode of the A3.
 * Active State: High
 */
#define DDRP_A3_LP4X_EN                            0x0bc, 28, 28

/* [27:27]  RW  reset 0x0  --  reg_a4_lp4x_en
 * The LPDDR4X enable mode of the A4.
 * Active State: High
 */
#define DDRP_A4_LP4X_EN                            0x0bc, 27, 27

/* [26:26]  RW  reset 0x0  --  reg_a5_lp4x_en
 * The LPDDR4X enable mode of the A5.
 * Active State: High
 */
#define DDRP_A5_LP4X_EN                            0x0bc, 26, 26

/* [25:25]  RW  reset 0x0  --  reg_a6_lp4x_en
 * The LPDDR4X enable mode of the A6.
 * Active State: High
 */
#define DDRP_A6_LP4X_EN                            0x0bc, 25, 25

/* [24:24]  RW  reset 0x0  --  reg_a7_lp4x_en
 * The LPDDR4X enable mode of the A7.
 * Active State: High
 */
#define DDRP_A7_LP4X_EN                            0x0bc, 24, 24

/* [23:23]  RW  reset 0x0  --  reg_a8_lp4x_en
 * The LPDDR4X enable mode of the A8.
 * Active State: High
 */
#define DDRP_A8_LP4X_EN                            0x0bc, 23, 23

/* [22:22]  RW  reset 0x0  --  reg_a9_lp4x_en
 * The LPDDR4X enable mode of the A9.
 * Active State: High
 */
#define DDRP_A9_LP4X_EN                            0x0bc, 22, 22

/* [21:21]  RW  reset 0x0  --  reg_a10_lp4x_en
 * The LPDDR4X enable mode of the A10.
 * Active State: High
 */
#define DDRP_A10_LP4X_EN                           0x0bc, 21, 21

/* [20:20]  RW  reset 0x0  --  reg_a11_lp4x_en
 * The LPDDR4X enable mode of the A11.
 * Active State: High
 */
#define DDRP_A11_LP4X_EN                           0x0bc, 20, 20

/* [19:19]  RW  reset 0x0  --  reg_a12_lp4x_en
 * The LPDDR4X enable mode of the A12.
 * Active State: High
 */
#define DDRP_A12_LP4X_EN                           0x0bc, 19, 19

/* [18:18]  RW  reset 0x0  --  reg_a13_lp4x_en
 * The LPDDR4X enable mode of the A13.
 * Active State: High
 */
#define DDRP_A13_LP4X_EN                           0x0bc, 18, 18

/* [17:17]  RW  reset 0x0  --  reg_a14_lp4x_en
 * The LPDDR4X enable mode of the A14.
 * Active State: High
 */
#define DDRP_A14_LP4X_EN                           0x0bc, 17, 17

/* [16:16]  RW  reset 0x0  --  reg_a15_lp4x_en
 * The LPDDR4X enable mode of the A15.
 * Active State: High
 */
#define DDRP_A15_LP4X_EN                           0x0bc, 16, 16

/* [15:15]  RW  reset 0x0  --  reg_a16_lp4x_en
 * The LPDDR4X enable mode of the A16.
 * Active State: High
 */
#define DDRP_A16_LP4X_EN                           0x0bc, 15, 15

/* [14:14]  RW  reset 0x0  --  reg_a17_lp4x_en
 * The LPDDR4X enable mode of the A17.
 * Active State: High
 */
#define DDRP_A17_LP4X_EN                           0x0bc, 14, 14

/* [13:13]  RW  reset 0x0  --  reg_actn_lp4x_en
 * The LPDDR4X enable mode of the ACTN.
 * Active State: High
 */
#define DDRP_ACTN_LP4X_EN                          0x0bc, 13, 13

/* [12:12]  RW  reset 0x0  --  reg_ba0_lp4x_en
 * The LPDDR4X enable mode of the BA0.
 * Active State: High
 */
#define DDRP_BA0_LP4X_EN                           0x0bc, 12, 12

/* [11:11]  RW  reset 0x0  --  reg_ba1_lp4x_en
 * The LPDDR4X enable mode of the BA1.
 * Active State: High ||
 */
#define DDRP_BA1_LP4X_EN                           0x0bc, 11, 11

/* [10:10]  RW  reset 0x0  --  reg_bg0_lp4x_en
 * The LPDDR4X enable mode of the BG0.
 * Active State: High
 */
#define DDRP_BG0_LP4X_EN                           0x0bc, 10, 10

/* [9:9]  RW  reset 0x0  --  reg_bg1_lp4x_en
 * The LPDDR4X enable mode of the BG1.
 * Active State: High
 */
#define DDRP_BG1_LP4X_EN                           0x0bc,  9,  9

/* [8:8]  RW  reset 0x0  --  reg_ck_lp4x_en
 * The LPDDR4X enable mode of the CK.
 * Active State: High
 */
#define DDRP_CK_LP4X_EN                            0x0bc,  8,  8

/* [7:7]  RW  reset 0x0  --  reg_ckb_lp4x_en
 * The LPDDR4X enable mode of the CKB.
 * Active State: High
 */
#define DDRP_CKB_LP4X_EN                           0x0bc,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_cke0_lp4x_en
 * The LPDDR4X enable mode of the CKE0.
 * Active State: High
 */
#define DDRP_CKE0_LP4X_EN                          0x0bc,  6,  6

/* [5:5]  RW  reset 0x0  --  reg_csb0_lp4x_en
 * The LPDDR4X enable mode of the CSB0.
 * Active State: High
 */
#define DDRP_CSB0_LP4X_EN                          0x0bc,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_odt0_lp4x_en
 * The LPDDR4X enable mode of the ODT0.
 * Active State: High
 */
#define DDRP_ODT0_LP4X_EN                          0x0bc,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_csb1_lp4x_en
 * The LPDDR4X enable mode of the CSB1.
 * Active State: High
 */
#define DDRP_CSB1_LP4X_EN                          0x0bc,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_odt1_lp4x_en
 * The LPDDR4X enable mode of the ODT1.
 * Active State: High
 */
#define DDRP_ODT1_LP4X_EN                          0x0bc,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_resetn_lp4x_en
 * The LPDDR4X enable mode of the RESETN.
 * Active State: High
 */
#define DDRP_RESETN_LP4X_EN                        0x0bc,  1,  1

/* [0:0]  RW  reset 0x1  --  reg_cmd_ca_enb_lp4
 * Increase the slew rate of signal by opening the pull-up and pull-down at the same time for LPDDR4/4X.
 * Active State: High
 * Keep it to 1’b1 for other SDRAM types.
 */
#define DDRP_CMD_CA_ENB_LP4                        0x0bc,  0,  0


/* ----- 0x0c0 ----- */
/* [31:31]  RW  reset 0x1  --  reg_a17_pvt_comp_en
 * The PVT compensation enable signal of the A17.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A17.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A17 will keep the current value.
 */
#define DDRP_A17_PVT_COMP_EN                       0x0c0, 31, 31

/* [30:30]  RW  reset 0x1  --  reg_a16_pvt_comp_en
 * The PVT compensation enable signal of the A16.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A16.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A16 will keep the current value.
 */
#define DDRP_A16_PVT_COMP_EN                       0x0c0, 30, 30

/* [29:29]  RW  reset 0x1  --  reg_a15_pvt_comp_en
 * The PVT compensation enable signal of the A15.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A15.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A15 will keep the current value.
 */
#define DDRP_A15_PVT_COMP_EN                       0x0c0, 29, 29

/* [28:28]  RW  reset 0x1  --  reg_a14_pvt_comp_en
 * The PVT compensation enable signal of the A14.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A14.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A14 will keep the current value.
 */
#define DDRP_A14_PVT_COMP_EN                       0x0c0, 28, 28

/* [27:27]  RW  reset 0x1  --  reg_a13_pvt_comp_en
 * The PVT compensation enable signal of the A13.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A13.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A13 will keep the current value.
 */
#define DDRP_A13_PVT_COMP_EN                       0x0c0, 27, 27

/* [26:26]  RW  reset 0x1  --  reg_a12_pvt_comp_en
 * The PVT compensation enable signal of the A12.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A12.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A12 will keep the current value.
 */
#define DDRP_A12_PVT_COMP_EN                       0x0c0, 26, 26

/* [25:25]  RW  reset 0x1  --  reg_a11_pvt_comp_en
 * The PVT compensation enable signal of the A11.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A11.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A11 will keep the current value.
 */
#define DDRP_A11_PVT_COMP_EN                       0x0c0, 25, 25

/* [24:24]  RW  reset 0x1  --  reg_a10_pvt_comp_en
 * The PVT compensation enable signal of the A10.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A10.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A10 will keep the current value.
 */
#define DDRP_A10_PVT_COMP_EN                       0x0c0, 24, 24

/* [23:23]  RW  reset 0x1  --  reg_a9_pvt_comp_en
 * The PVT compensation enable signal of the A9.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A9.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A9 will keep the current value.
 */
#define DDRP_A9_PVT_COMP_EN                        0x0c0, 23, 23

/* [22:22]  RW  reset 0x1  --  reg_a8_pvt_comp_en
 * The PVT compensation enable signal of the A8.
 * Active State: High
 * 1: When the PVTcompensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A8.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A8 will keep the current value.
 */
#define DDRP_A8_PVT_COMP_EN                        0x0c0, 22, 22

/* [21:21]  RW  reset 0x1  --  reg_a7_pvt_comp_en
 * The PVT compensation enable signal of the A7.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A7.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A7 will keep the current value.
 */
#define DDRP_A7_PVT_COMP_EN                        0x0c0, 21, 21

/* [20:20]  RW  reset 0x1  --  reg_a6_pvt_comp_en
 * The PVT compensation enable signal of the A6.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A6.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A6 will keep the current value.
 */
#define DDRP_A6_PVT_COMP_EN                        0x0c0, 20, 20

/* [19:19]  RW  reset 0x1  --  reg_a5_pvt_comp_en
 * The PVT compensation enable signal of the A5.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A5.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A5 will keep the current value.
 */
#define DDRP_A5_PVT_COMP_EN                        0x0c0, 19, 19

/* [18:18]  RW  reset 0x1  --  reg_a4_pvt_comp_en
 * The PVT compensation enable signal of the A4.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A4.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A4 will keep the current value.
 */
#define DDRP_A4_PVT_COMP_EN                        0x0c0, 18, 18

/* [17:17]  RW  reset 0x1  --  reg_a3_pvt_comp_en
 * The PVT compensation enable signal of the A3.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A3.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A3 will keep the current value.
 */
#define DDRP_A3_PVT_COMP_EN                        0x0c0, 17, 17

/* [16:16]  RW  reset 0x1  --  reg_a2_pvt_comp_en
 * The PVT compensation enable signal of the A2.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A2.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A2 will keep the current value.
 */
#define DDRP_A2_PVT_COMP_EN                        0x0c0, 16, 16

/* [15:15]  RW  reset 0x1  --  reg_a1_pvt_comp_en
 * The PVT compensation enable signal of the A1.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A1.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A1 will keep the current value.
 */
#define DDRP_A1_PVT_COMP_EN                        0x0c0, 15, 15

/* [14:14]  RW  reset 0x1  --  reg_a0_pvt_comp_en
 * The PVT compensation enable signal of the A0.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of A0.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of A0 will keep the current value.
 */
#define DDRP_A0_PVT_COMP_EN                        0x0c0, 14, 14

/* [13:13]  RW  reset 0x1  --  reg_actn_pvt_comp_en
 * The PVT compensation enable signal of the ACTN.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of ACTN.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of ACTN will keep the current value.
 */
#define DDRP_ACTN_PVT_COMP_EN                      0x0c0, 13, 13

/* [12:12]  RW  reset 0x1  --  reg_ba0_pvt_comp_en
 * The PVT compensation enable signal of the BA0.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of BA0.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of BA0 will keep the current value.
 */
#define DDRP_BA0_PVT_COMP_EN                       0x0c0, 12, 12

/* [11:11]  RW  reset 0x1  --  reg_ba1_pvt_comp_en
 * The PVT compensation enable signal of the BA1.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of BA1.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of BA1 will keep the current value.
 */
#define DDRP_BA1_PVT_COMP_EN                       0x0c0, 11, 11

/* [10:10]  RW  reset 0x1  --  reg_bg0_pvt_comp_en
 * The PVT compensation enable signal of the BG0.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of BG0.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of BG0 will keep the current value.
 */
#define DDRP_BG0_PVT_COMP_EN                       0x0c0, 10, 10

/* [9:9]  RW  reset 0x1  --  reg_bg1_pvt_comp_en
 * The PVT compensation enable signal of the BG1.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of BG1.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of BG1 will keep the current value.
 */
#define DDRP_BG1_PVT_COMP_EN                       0x0c0,  9,  9

/* [8:8]  RW  reset 0x1  --  reg_ck_pvt_comp_en
 * The PVT compensation enable signal of the CK.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of CK.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of CK will keep the current value.
 */
#define DDRP_CK_PVT_COMP_EN                        0x0c0,  8,  8

/* [7:7]  RW  reset 0x1  --  reg_ckb_pvt_comp_en
 * The PVT compensation enable signal of the CKB.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of CKB.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of CKB will keep the current value.
 */
#define DDRP_CKB_PVT_COMP_EN                       0x0c0,  7,  7

/* [6:6]  RW  reset 0x1  --  reg_cke0_pvt_comp_en
 * The PVT compensation enable signal of the CKE0.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of CKE0.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of CKE0 will keep the current value.
 */
#define DDRP_CKE0_PVT_COMP_EN                      0x0c0,  6,  6

/* [5:5]  RW  reset 0x1  --  reg_csb0_pvt_comp_en
 * The PVT compensation enable signal of the CSB0.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of CSB0.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of CSB0 will keep the current value.
 */
#define DDRP_CSB0_PVT_COMP_EN                      0x0c0,  5,  5

/* [4:4]  RW  reset 0x1  --  reg_odt0_pvt_comp_en
 * The PVT compensation enable signal of the ODT0.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of ODT0.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of ODT0 will keep the current value.
 */
#define DDRP_ODT0_PVT_COMP_EN                      0x0c0,  4,  4

/* [3:3]  RW  reset 0x1  --  reg_cke1_pvt_comp_en
 * The PVT compensation enable signal of the CKE1.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of CKE1.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of CKE1 will keep the current value.
 */
#define DDRP_CKE1_PVT_COMP_EN                      0x0c0,  3,  3

/* [2:2]  RW  reset 0x1  --  reg_csb1_pvt_comp_en
 * The PVT compensation enable signal of the CSB1.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of CSB1.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of CSB1 will keep the current value.
 */
#define DDRP_CSB1_PVT_COMP_EN                      0x0c0,  2,  2

/* [1:1]  RW  reset 0x1  --  reg_odt1_pvt_comp_en
 * The PVT compensation enable signal of the ODT1.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of ODT1.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of ODT1 will keep the current value.
 */
#define DDRP_ODT1_PVT_COMP_EN                      0x0c0,  1,  1

/* [0:0]  RW  reset 0x1  --  reg_resetn_pvt_comp_en
 * The PVT compensation enable signal of the RESETN.
 * Active State: High
 * 1: When the PVT compensation function is enabled and triggered, the compensation value will be updated to the Tx delay line of RESETN.
 * 0: When the PVT compensation function is enabled and triggered, the Tx delay line of RESETN will keep the current value.
 */
#define DDRP_RESETN_PVT_COMP_EN                    0x0c0,  0,  0


/* ----- 0x0c4 ----- */
/* [17:17]  RW  reset 0x0  --  reg_cmd_drv_zqcalib_en
 * The command driver control signal.
 * 1: Choose the ZQ calibration result as the control signal.
 * 0: Choose the register as the control signal.
 */
#define DDRP_CMD_DRV_ZQCALIB_EN                    0x0c4, 17, 17

/* [16:16]  RW  reset 0x0  --  reg_cmd_fbsel_reg
 * The PHY BIST command path select signal.
 * 1: Choose the internal as the end.
 * 0: Choose to arrive the pad.
 */
#define DDRP_CMD_FBSEL_REG                         0x0c4, 16, 16

/* [15:15]  RW  reset 0x0  --  reg_cmd_fben_reg
 * The PHY BIST command feedback enable signal. Just used for debug.
 * Active State: High
 */
#define DDRP_CMD_FBEN_REG                          0x0c4, 15, 15

/* [13:13]  RW  reset 0x1  --  reg_cmd_abutweakpub_reg
 * The enable signal of CMD pad weak pull up.
 * Active State: Low
 */
#define DDRP_CMD_ABUTWEAKPUB_REG                   0x0c4, 13, 13

/* [12:8]  RW  reset 0x0  --  reg_cmd_abutslewpu_reg
 * CMD/CK  edge slew rate control, default 0 means maximum slew rate.
 */
#define DDRP_CMD_ABUTSLEWPU_REG                    0x0c4, 12,  8

/* [5:5]  RW  reset 0x0  --  reg_cmd_abutweakpd_reg
 * The enable signal of CMD pad weak pull down.
 * Active State: High
 */
#define DDRP_CMD_ABUTWEAKPD_REG                    0x0c4,  5,  5

/* [4:0]  RW  reset 0x0  --  reg_cmd_abutslewpd_reg
 * Reserved.
 */
#define DDRP_CMD_ABUTSLEWPD_REG                    0x0c4,  4,  0


/* ----- 0x0c8 ----- */
/* [28:24]  RW  reset 0xe  --  reg_cmd_abutnrcomp_reg
 * The pull-down resistance of CMD except CK.
 */
#define DDRP_CMD_ABUTNRCOMP_REG                    0x0c8, 28, 24

/* [20:16]  RW  reset 0xe  --  reg_cmd_abutprcomp_reg
 * The pull-up resistance of CMD except CK.
 */
#define DDRP_CMD_ABUTPRCOMP_REG                    0x0c8, 20, 16

/* [12:8]  RW  reset 0xe  --  reg_cmd_abutnrcomp_ck0_reg
 * The pull-down resistance of CK.
 */
#define DDRP_CMD_ABUTNRCOMP_CK0_REG                0x0c8, 12,  8

/* [4:0]  RW  reset 0xe  --  reg_cmd_abutprcomp_ck0_reg
 * The pull-up resistance of CK.
 */
#define DDRP_CMD_ABUTPRCOMP_CK0_REG                0x0c8,  4,  0


/* ----- 0x0cc ----- */
/* [24:16]  RW  reset 0x80  --  reg_ram_vref1_margsel_reg
 * Reserved.
 */
#define DDRP_RAM_VREF1_MARGSEL_REG                 0x0cc, 24, 16

/* [12:8]  RW  reset 0xe  --  reg_cmd_abutnrcomp_special_reg
 * Reserved.
 */
#define DDRP_CMD_ABUTNRCOMP_SPECIAL_REG            0x0cc, 12,  8

/* [4:0]  RW  reset 0xe  --  reg_cmd_abutprcomp_special_reg
 * Reserved.
 */
#define DDRP_CMD_ABUTPRCOMP_SPECIAL_REG            0x0cc,  4,  0


/* ----- 0x0d0 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a0_invdelaysel_bp
 * Control the Tx delay line of the A0. Each step is 4UI/256.
 */
#define DDRP_A0_INVDELAYSEL_BP                     0x0d0, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_a1_invdelaysel_bp
 * Control the Tx delay line of the A1. Each step is 4UI/256.
 */
#define DDRP_A1_INVDELAYSEL_BP                     0x0d0, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_a2_invdelaysel_bp
 * Control the Tx delay line of the A2. Each step is 4UI/256.
 */
#define DDRP_A2_INVDELAYSEL_BP                     0x0d0, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_a3_invdelaysel_bp
 * Control the Tx delay line of the A3. Each step is 4UI/256.
 */
#define DDRP_A3_INVDELAYSEL_BP                     0x0d0,  7,  0


/* ----- 0x0d4 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a4_invdelaysel_bp
 * Control the Tx delay line of the A4. Each step is 4UI/256.
 */
#define DDRP_A4_INVDELAYSEL_BP                     0x0d4, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_a5_invdelaysel_bp
 * Control the Tx delay line of the A5. Each step is 4UI/256.
 */
#define DDRP_A5_INVDELAYSEL_BP                     0x0d4, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_a6_invdelaysel_bp
 * Control the Tx delay line of the A6. Each step is 4UI/256.
 */
#define DDRP_A6_INVDELAYSEL_BP                     0x0d4, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_a7_invdelaysel_bp
 * Control the Tx delay line of the A7. Each step is 4UI/256.
 */
#define DDRP_A7_INVDELAYSEL_BP                     0x0d4,  7,  0


/* ----- 0x0d8 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a8_invdelaysel_bp
 * Control the Tx delay line of the A8. Each step is 4UI/256.
 */
#define DDRP_A8_INVDELAYSEL_BP                     0x0d8, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_a9_invdelaysel_bp
 * Control the Tx delay line of the A9. Each step is 4UI/256.
 */
#define DDRP_A9_INVDELAYSEL_BP                     0x0d8, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_a10_invdelaysel_bp
 * Control the Tx delay line of the A10. Each step is 4UI/256.
 */
#define DDRP_A10_INVDELAYSEL_BP                    0x0d8, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_a11_invdelaysel_bp
 * Control the Tx delay line of the A11. Each step is 4UI/256.
 */
#define DDRP_A11_INVDELAYSEL_BP                    0x0d8,  7,  0


/* ----- 0x0dc ----- */
/* [31:24]  RW  reset 0x80  --  reg_a12_invdelaysel_bp
 * Control the Tx delay line of the A12. Each step is 4UI/256.
 */
#define DDRP_A12_INVDELAYSEL_BP                    0x0dc, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_a13_invdelaysel_bp
 * Control the Tx delay line of the A13. Each step is 4UI/256.
 */
#define DDRP_A13_INVDELAYSEL_BP                    0x0dc, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_a14_invdelaysel_bp
 * Control the Tx delay line of the A14. Each step is 4UI/256.
 */
#define DDRP_A14_INVDELAYSEL_BP                    0x0dc, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_a15_invdelaysel_bp
 * Control the Tx delay line of the A15. Each step is 4UI/256.
 */
#define DDRP_A15_INVDELAYSEL_BP                    0x0dc,  7,  0


/* ----- 0x0e0 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a16_invdelaysel_bp
 * Control the Tx delay line of the A16. Each step is 4UI/256.
 */
#define DDRP_A16_INVDELAYSEL_BP                    0x0e0, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_a17_invdelaysel_bp
 * Control the Tx delay line of the A17. Each step is 4UI/256.
 */
#define DDRP_A17_INVDELAYSEL_BP                    0x0e0, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_ba0_invdelaysel_bp
 * Control the Tx delay line of the BA0. Each step is 4UI/256.
 */
#define DDRP_BA0_INVDELAYSEL_BP                    0x0e0, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_ba1_invdelaysel_bp
 * Control the Tx delay line of the BA1. Each step is 4UI/256.
 */
#define DDRP_BA1_INVDELAYSEL_BP                    0x0e0,  7,  0


/* ----- 0x0e4 ----- */
/* [31:24]  RW  reset 0x80  --  reg_bg0_invdelaysel_bp
 * Control the Tx delay line of the BG0. Each step is 4UI/256.
 */
#define DDRP_BG0_INVDELAYSEL_BP                    0x0e4, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_bg1_invdelaysel_bp
 * Control the Tx delay line of the BG1. Each step is 4UI/256.
 */
#define DDRP_BG1_INVDELAYSEL_BP                    0x0e4, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_cke0_invdelaysel_bp
 * Control the Tx delay line of the CKE0. Each step is 4UI/256.
 */
#define DDRP_CKE0_INVDELAYSEL_BP                   0x0e4, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_cke1_invdelaysel_bp
 * Control the Tx delay line of the CKE1. Each step is 4UI/256.
 */
#define DDRP_CKE1_INVDELAYSEL_BP                   0x0e4,  7,  0


/* ----- 0x0e8 ----- */
/* [31:24]  RW  reset 0x80  --  reg_ckb_invdelaysel_bp
 * Control the Tx delay line of the CKB. Each step is 4UI/256.
 */
#define DDRP_CKB_INVDELAYSEL_BP                    0x0e8, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_ck_invdelaysel_bp
 * Control the Tx delay line of the CK. Each step is 4UI/256.
 */
#define DDRP_CK_INVDELAYSEL_BP                     0x0e8, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_odt0_invdelaysel_bp
 * Control the Tx delay line of the ODT0. Each step is 4UI/256.
 */
#define DDRP_ODT0_INVDELAYSEL_BP                   0x0e8, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_odt1_invdelaysel_bp
 * Control the Tx delay line of the ODT1. Each step is 4UI/256.
 */
#define DDRP_ODT1_INVDELAYSEL_BP                   0x0e8,  7,  0


/* ----- 0x0ec ----- */
/* [31:24]  RW  reset 0x80  --  reg_csb0_invdelaysel_bp
 * Control the Tx delay line of the CSB0. Each step is 4UI/256.
 */
#define DDRP_CSB0_INVDELAYSEL_BP                   0x0ec, 31, 24

/* [23:16]  RW  reset 0x80  --  reg_csb1_invdelaysel_bp
 * Control the Tx delay line of the CSB1. Each step is 4UI/256.
 */
#define DDRP_CSB1_INVDELAYSEL_BP                   0x0ec, 23, 16

/* [15:8]  RW  reset 0x80  --  reg_resetn_invdelaysel_bp
 * Control the Tx delay line of the RESETN. Each step is 4UI/256.
 */
#define DDRP_RESETN_INVDELAYSEL_BP                 0x0ec, 15,  8

/* [7:0]  RW  reset 0x80  --  reg_actn_invdelaysel_bp
 * Control the Tx delay line of the ACTN. Each step is 4UI/256.
 */
#define DDRP_ACTN_INVDELAYSEL_BP                   0x0ec,  7,  0


/* ----- 0x0f0 ----- */
/* [31:24]  RW  reset 0x2b  --  group1_dq0_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE0    0x0f0, 31, 24

/* [23:16]  RW  reset 0x5f  --  group1_dq0_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE1    0x0f0, 23, 16

/* [15:8]  RW  reset 0x38  --  group1_dq0_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE2    0x0f0, 15,  8

/* [7:0]  RW  reset 0x92  --  group1_dq0_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE3    0x0f0,  7,  0


/* ----- 0x0f4 ----- */
/* [31:24]  RW  reset 0xad  --  group1_dq0_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE4    0x0f4, 31, 24

/* [23:16]  RW  reset 0xbd  --  group1_dq0_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE5    0x0f4, 23, 16

/* [15:8]  RW  reset 0xb1  --  group1_dq0_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE6    0x0f4, 15,  8

/* [7:0]  RW  reset 0x74  --  group1_dq0_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE7    0x0f4,  7,  0


/* ----- 0x0f8 ----- */
/* [15:8]  RW  reset 0x67  --  group1_dq0_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE8    0x0f8, 15,  8

/* [7:0]  RW  reset 0xaa  --  group1_dq0_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ0/A_DQ8/B_DQ0/B_DQ8.
 */
#define DDRP_GROUP1_DQ0_TRAIN_CHECK_DATA_VALUE9    0x0f8,  7,  0


/* ----- 0x0fc ----- */
/* [31:24]  RW  reset 0x3b  --  group1_dq1_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE0    0x0fc, 31, 24

/* [23:16]  RW  reset 0x53  --  group1_dq1_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE1    0x0fc, 23, 16

/* [15:8]  RW  reset 0xfd  --  group1_dq1_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE2    0x0fc, 15,  8

/* [7:0]  RW  reset 0x81  --  group1_dq1_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE3    0x0fc,  7,  0


/* ----- 0x100 ----- */
/* [31:24]  RW  reset 0x60  --  group1_dq1_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE4    0x100, 31, 24

/* [23:16]  RW  reset 0x28  --  group1_dq1_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE5    0x100, 23, 16

/* [15:8]  RW  reset 0x9e  --  group1_dq1_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE6    0x100, 15,  8

/* [7:0]  RW  reset 0x68  --  group1_dq1_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE7    0x100,  7,  0


/* ----- 0x104 ----- */
/* [15:8]  RW  reset 0xae  --  group1_dq1_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE8    0x104, 15,  8

/* [7:0]  RW  reset 0x7c  --  group1_dq1_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ1/A_DQ9/B_DQ1/B_DQ9.
 */
#define DDRP_GROUP1_DQ1_TRAIN_CHECK_DATA_VALUE9    0x104,  7,  0


/* ----- 0x108 ----- */
/* [31:24]  RW  reset 0x20  --  group1_dq2_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE0    0x108, 31, 24

/* [23:16]  RW  reset 0x18  --  group1_dq2_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE1    0x108, 23, 16

/* [15:8]  RW  reset 0x8a  --  group1_dq2_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE2    0x108, 15,  8

/* [7:0]  RW  reset 0x27  --  group1_dq2_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE3    0x108,  7,  0


/* ----- 0x10c ----- */
/* [31:24]  RW  reset 0x9a  --  group1_dq2_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE4    0x10c, 31, 24

/* [23:16]  RW  reset 0x2b  --  group1_dq2_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE5    0x10c, 23, 16

/* [15:8]  RW  reset 0x5f  --  group1_dq2_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE6    0x10c, 15,  8

/* [7:0]  RW  reset 0x38  --  group1_dq2_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE7    0x10c,  7,  0


/* ----- 0x110 ----- */
/* [15:8]  RW  reset 0x92  --  group1_dq2_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE8    0x110, 15,  8

/* [7:0]  RW  reset 0xad  --  group1_dq2_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ2/A_DQ10/B_DQ2/B_DQ10.
 */
#define DDRP_GROUP1_DQ2_TRAIN_CHECK_DATA_VALUE9    0x110,  7,  0


/* ----- 0x114 ----- */
/* [31:24]  RW  reset 0x77  --  group1_dq3_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE0    0x114, 31, 24

/* [23:16]  RW  reset 0xa6  --  group1_dq3_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE1    0x114, 23, 16

/* [15:8]  RW  reset 0xfa  --  group1_dq3_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE2    0x114, 15,  8

/* [7:0]  RW  reset 0x03  --  group1_dq3_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE3    0x114,  7,  0


/* ----- 0x118 ----- */
/* [31:24]  RW  reset 0xc1  --  group1_dq3_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE4    0x118, 31, 24

/* [23:16]  RW  reset 0x50  --  group1_dq3_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE5    0x118, 23, 16

/* [15:8]  RW  reset 0x3c  --  group1_dq3_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE6    0x118, 15,  8

/* [7:0]  RW  reset 0xd1  --  group1_dq3_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE7    0x118,  7,  0


/* ----- 0x11c ----- */
/* [15:8]  RW  reset 0x5c  --  group1_dq3_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE8    0x11c, 15,  8

/* [7:0]  RW  reset 0xf9  --  group1_dq3_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ3/A_DQ11/B_DQ3/B_DQ11.
 */
#define DDRP_GROUP1_DQ3_TRAIN_CHECK_DATA_VALUE9    0x11c,  7,  0


/* ----- 0x120 ----- */
/* [31:24]  RW  reset 0x5b  --  group1_dq4_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE0    0x120, 31, 24

/* [23:16]  RW  reset 0x7b  --  group1_dq4_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE1    0x120, 23, 16

/* [15:8]  RW  reset 0x63  --  group1_dq4_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE2    0x120, 15,  8

/* [7:0]  RW  reset 0xe9  --  group1_dq4_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE3    0x120,  7,  0


/* ----- 0x124 ----- */
/* [31:24]  RW  reset 0xce  --  group1_dq4_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through  the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE4    0x124, 31, 24

/* [23:16]  RW  reset 0x54  --  group1_dq4_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE5    0x124, 23, 16

/* [15:8]  RW  reset 0x7f  --  group1_dq4_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through  the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE6    0x124, 15,  8

/* [7:0]  RW  reset 0x20  --  group1_dq4_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE7    0x124,  7,  0


/* ----- 0x128 ----- */
/* [15:8]  RW  reset 0x18  --  group1_dq4_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE8    0x128, 15,  8

/* [7:0]  RW  reset 0x8a  --  group1_dq4_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ4/A_DQ12/B_DQ4/B_DQ12.
 */
#define DDRP_GROUP1_DQ4_TRAIN_CHECK_DATA_VALUE9    0x128,  7,  0


/* ----- 0x12c ----- */
/* [31:24]  RW  reset 0xce  --  group1_dq5_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE0    0x12c, 31, 24

/* [23:16]  RW  reset 0x54  --  group1_dq5_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE1    0x12c, 23, 16

/* [15:8]  RW  reset 0x7f  --  group1_dq5_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE2    0x12c, 15,  8

/* [7:0]  RW  reset 0x20  --  group1_dq5_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE3    0x12c,  7,  0


/* ----- 0x130 ----- */
/* [31:24]  RW  reset 0x18  --  group1_dq5_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE4    0x130, 31, 24

/* [23:16]  RW  reset 0x8a  --  group1_dq5_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE5    0x130, 23, 16

/* [15:8]  RW  reset 0x27  --  group1_dq5_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE6    0x130, 15,  8

/* [7:0]  RW  reset 0x9a  --  group1_dq5_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE7    0x130,  7,  0


/* ----- 0x134 ----- */
/* [15:8]  RW  reset 0x2b  --  group1_dq5_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE8    0x134, 15,  8

/* [7:0]  RW  reset 0x5f  --  group1_dq5_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ5/A_DQ13/B_DQ5/B_DQ13.
 */
#define DDRP_GROUP1_DQ5_TRAIN_CHECK_DATA_VALUE9    0x134,  7,  0


/* ----- 0x138 ----- */
/* [31:24]  RW  reset 0xd6  --  group1_dq6_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE0    0x138, 31, 24

/* [23:16]  RW  reset 0xde  --  group1_dq6_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE1    0x138, 23, 16

/* [15:8]  RW  reset 0x58  --  group1_dq6_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE2    0x138, 15,  8

/* [7:0]  RW  reset 0xba  --  group1_dq6_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE3    0x138,  7,  0


/* ----- 0x13c ----- */
/* [31:24]  RW  reset 0x33  --  group1_dq6_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of third burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE4    0x13c, 31, 24

/* [23:16]  RW  reset 0xd5  --  group1_dq6_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE5    0x13c, 23, 16

/* [15:8]  RW  reset 0x1f  --  group1_dq6_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE6    0x13c, 15,  8

/* [7:0]  RW  reset 0x08  --  group1_dq6_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE7    0x13c,  7,  0


/* ----- 0x140 ----- */
/* [15:8]  RW  reset 0x86  --  group1_dq6_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE8    0x140, 15,  8

/* [7:0]  RW  reset 0xe2  --  group1_dq6_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ6/A_DQ14/B_DQ6/B_DQ14.
 */
#define DDRP_GROUP1_DQ6_TRAIN_CHECK_DATA_VALUE9    0x140,  7,  0


/* ----- 0x144 ----- */
/* [31:24]  RW  reset 0x82  --  group1_dq7_train_check_data_value0
 * For the write training of DDR4, it is the data pattern of the first burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE0    0x144, 31, 24

/* [23:16]  RW  reset 0xa1  --  group1_dq7_train_check_data_value1
 * For the write training of DDR4, it is the data pattern of the second burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the first burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE1    0x144, 23, 16

/* [15:8]  RW  reset 0x78  --  group1_dq7_train_check_data_value2
 * For the write training of DDR4, it is the data pattern of the third burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE2    0x144, 15,  8

/* [7:0]  RW  reset 0xa2  --  group1_dq7_train_check_data_value3
 * For the write training of DDR4, it is the data pattern of the fourth burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the second burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE3    0x144,  7,  0


/* ----- 0x148 ----- */
/* [31:24]  RW  reset 0xb9  --  group1_dq7_train_check_data_value4
 * For the write training of DDR4, it is the data pattern of the fifth burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE4    0x148, 31, 24

/* [23:16]  RW  reset 0xf2  --  group1_dq7_train_check_data_value5
 * For the write training of DDR4, it is the data pattern of the sixth burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the third burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE5    0x148, 23, 16

/* [15:8]  RW  reset 0x85  --  group1_dq7_train_check_data_value6
 * For the write training of DDR4, it is the data pattern of the seventh burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE6    0x148, 15,  8

/* [7:0]  RW  reset 0x23  --  group1_dq7_train_check_data_value7
 * For the write training of DDR4, it is the data pattern of the eighth burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fourth burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE7    0x148,  7,  0


/* ----- 0x14c ----- */
/* [21:21]  RW  reset 0x0  --  reg_train_en_neg_reg
 * Reserved
 */
#define DDRP_TRAIN_EN_NEG_REG                      0x14c, 21, 21

/* [20:20]  RW  reset 0x0  --  reg_wrtrain_odt_advance
 * Used in the write training which makes the ODT open advance when doing the write operation.
 * You can keep the default value.
 */
#define DDRP_WRTRAIN_ODT_ADVANCE                   0x14c, 20, 20

/* [19:16]  RW  reset 0x1  --  reg_wrtrain_odt_keep
 * Used in the write training which makes the ODT open keep when doing the write operation.
 * You can keep the default value.
 */
#define DDRP_WRTRAIN_ODT_KEEP                      0x14c, 19, 16

/* [15:8]  RW  reset 0xd9  --  group1_dq7_train_check_data_value8
 * For the write training of DDR4, it is the data pattern of the ninth burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the previous 8 bits of fifth burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE8    0x14c, 15,  8

/* [7:0]  RW  reset 0xda  --  group1_dq7_train_check_data_value9
 * For the write training of DDR4, it is the data pattern of the tenth burst8 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 * For the write training of LPDDR4, it is the data pattern of the last 8 bits of the fifth burst16 which will be written to the SDRAM through the A_DQ7/A_DQ15/B_DQ7/B_DQ15.
 */
#define DDRP_GROUP1_DQ7_TRAIN_CHECK_DATA_VALUE9    0x14c,  7,  0


/* ----- 0x150 ----- */
/* [31:24]  RW  reset 0x14  --  reg_wrtrain_vref_wait_vref_cnt_50ns
 * Tell PHY how long 50ns is for a reference. Configure this register when Tx Vref scan function is enabled in the write training. When a new Tx Vref value to SDRAM is set, wait 100~250ns for it to take effect. The value of this register is 50ns/dfi_1xclk.
 */
#define DDRP_WRTRAIN_VREF_WAIT_VREF_CNT_50NS       0x150, 31, 24

/* [23:21]  RW  reset 0x1  --  reg_train_vref_step_min
 * Indicate how many steps needed at least at a time when doing a fine Vref scanning.
 * Configure this register when the Vref scan function is enabled in the predefined read training or write training.
 */
#define DDRP_TRAIN_VREF_STEP_MIN                   0x150, 23, 21

/* [20:16]  RW  reset 0x8  --  reg_train_vref_step_max
 * Indicate how many steps needed at most at a time when doing a rough Vref scanning. Configure this register when the Vref scan function is enabled in the predefined read training or write training.
 */
#define DDRP_TRAIN_VREF_STEP_MAX                   0x150, 20, 16

/* [15:10]  RW  reset 0x0  --  reg_cmd_invdelaysel_sel
 * Reserved
 */
#define DDRP_CMD_INVDELAYSEL_SEL                   0x150, 15, 10

/* [9:0]  RW  reset 0x140  --  reg_rdtrain_wait_vref_valid_cnt
 * Note: This register is not available for this design.
 * Configure this register when Rx Vref scan function is enabled in the predefined read training. When a new Rx Vref value is set, wait 800ns for it to take effect. The value of this register is 800ns/dfi_1xclk.
 */
#define DDRP_RDTRAIN_WAIT_VREF_VALID_CNT           0x150,  9,  0


/* ----- 0x154 ----- */
/* [31:27]  RW  reset 0x0  --  reg_twldqsen_cnt
 * tWLDQSEN=(33+reg_twldqsen_cnt)*tck
 */
#define DDRP_TWLDQSEN_CNT                          0x154, 31, 27

/* [26:19]  RW  reset 0xf  --  reg_ddrphy_trp
 * Control the timing parameter tRP which means the delay between the Precharge command to the Next command.
 */
#define DDRP_DDRPHY_TRP                            0x154, 26, 19

/* [18:13]  RW  reset 0x1b  --  reg_wl_dqs_lock_point
 * Control the write leveling check point corresponding to the tWLO.
 */
#define DDRP_WL_DQS_LOCK_POINT                     0x154, 18, 13

/* [12:12]  RW  reset 0x0  --  reg_freq_choose_wr_bypass
 * The bypass signal of the frequency point choose.
 * Active State: High
 */
#define DDRP_FREQ_CHOOSE_WR_BYPASS                 0x154, 12, 12

/* [11:6]  RW  reset 0x0  --  reg_data_path_clk_gate_dly
 * Reserved
 */
#define DDRP_DATA_PATH_CLK_GATE_DLY                0x154, 11,  6

/* [5:5]  RW  reset 0x0  --  reg_data_path_clk_gate_dly_bp
 * Reserved
 */
#define DDRP_DATA_PATH_CLK_GATE_DLY_BP             0x154,  5,  5

/* [4:4]  RW  reset 0x0  --  reg_freq_choose_op_bypass
 * The bypass signal of the frequency point choose.
 * Active State: High
 * 1: Choose the frequency point by using the reg_freq_choose_op_t register
 * 0: Choose the frequency point based on the dfi_frequency signal during the PHY initialization and fast frequency change
 */
#define DDRP_FREQ_CHOOSE_OP_BYPASS                 0x154,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_all_freq_train_finish
 * The complete signal of the Rx Vref switch in the fast frequency switch.
 * 1: Compelete Rx Vref switch
 * 0: Remain the Rx Vref unchanged
 * In this case, the Rx Vref switch takes many cycles, so it is recommended that the reg_all_freq_train_finish should be set to 1’b0 before training and set to 1’b1 after training.
 */
#define DDRP_ALL_FREQ_TRAIN_FINISH                 0x154,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_pll_lock_bypass
 * Choose the way to wait for the PLL lock after the PLL is enabled.
 * 0: Wait the PLL lock from the PLL module
 * 1: Wait the PLL lock by enabling the PLL lock counter
 */
#define DDRP_PLL_LOCK_BYPASS                       0x154,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_pllpd_bypass
 * Choose the way to control the power down signal of the PLL module.
 * 1: Controlled by the register only
 * 0: Auto controlled in the DFI low power mode
 */
#define DDRP_PLLPD_BYPASS                          0x154,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_lpddr4_write_postamble_sel
 * Choose the postamble mode for LPDDR4.
 * 1: 1.5tCK postamble
 * 0: 0.5tCK postamble
 */
#define DDRP_LPDDR4_WRITE_POSTAMBLE_SEL            0x154,  0,  0


/* ----- 0x158 ----- */
/* [7:7]  RO  reset 0x0  --  train_all_step_done
 * The complete signal of the write training.
 * Active State: High
 * (Reused for predefined read training and write training)
 */
#define DDRP_TRAIN_ALL_STEP_DONE                   0x158,  7,  7

/* [6:6]  RO  reset 0x0  --  train_step1_delay_done
 * The complete signal of the delay scan before the Vref scan when write training.
 * Active State: High
 * (Reused for predefined read training and write training)
 */
#define DDRP_TRAIN_STEP1_DELAY_DONE                0x158,  6,  6

/* [5:5]  RO  reset 0x0  --  train_step2_vref_done
 * The complete signal for the Vref scan when write training.
 * Active State: High
 * (Reused for predefined read training and write training)
 */
#define DDRP_TRAIN_STEP2_VREF_DONE                 0x158,  5,  5

/* [4:4]  RO  reset 0x0  --  train_step3_delay_done
 * The complete signal of the delay scan after the Vref scan when write training.
 * Active State: High
 * (Reused for predefined read training and write training)
 */
#define DDRP_TRAIN_STEP3_DELAY_DONE                0x158,  4,  4

/* [3:3]  RO  reset 0x0  --  train_step1_error
 * The error-check signal of step 1 in  write training. Check the value of this register after the training is done.
 * If the value = 1’b1: Error in step 1.
 * Refer to read training and write training section for the steps of training.
 */
#define DDRP_TRAIN_STEP1_ERROR                     0x158,  3,  3

/* [2:2]  RO  reset 0x0  --  train_step2_error
 * The error-check signal of step 2 in write training. Check the value of this register after the training is done.
 * If the value = 1’b1: Error in step 2.
 * (Reused for predefined read training and write training)
 */
#define DDRP_TRAIN_STEP2_ERROR                     0x158,  2,  2

/* [1:1]  RO  reset 0x0  --  train_step3_error
 * The error-check signal of step 3 in write training. Check the value of this register after training is done.
 * If the value = 1’b1: Error in step 3.
 * (Reused for predefined read training and write training)
 */
#define DDRP_TRAIN_STEP3_ERROR                     0x158,  1,  1

/* [0:0]  RO  reset 0x0  --  train_true_done
 * The complete signal of the read training (MPR/MPC mode).
 * Active State: High
 */
#define DDRP_TRAIN_TRUE_DONE                       0x158,  0,  0


/* ----- 0x15c ----- */
/* [14:8]  RO  reset 0x1  --  pvt_comp_cs_to_reg
 * The observation signal of the PVT compensation state machine.
 */
#define DDRP_PVT_COMP_CS_TO_REG                    0x15c, 14,  8

/* [7:7]  RO  reset 0x1  --  pvt_comp_req_wait_time_out_to_reg
 * The flag of timeout that the PHY did not receive the dfi_phymstr_ack signal after the dfi_phymstr_req signal is asserted in the specified time.
 * Active State: High
 */
#define DDRP_PVT_COMP_REQ_WAIT_TIME_OUT_TO_REG     0x15c,  7,  7

/* [6:6]  RO  reset 0x0  --  cat_low_freq_sel
 * Reserved.
 */
#define DDRP_CAT_LOW_FREQ_SEL                      0x15c,  6,  6

/* [5:5]  RO  reset 0x0  --  user_load_mode_busy
 * Reserved.
 */
#define DDRP_USER_LOAD_MODE_BUSY                   0x15c,  5,  5

/* [3:3]  RO  reset 0x1  --  pwrokcore
 * Reserved.
 */
#define DDRP_PWROKCORE                             0x15c,  3,  3

/* [2:2]  RO  reset 0x0  --  dll_lock_to_reg
 * The lock flag of the mdll.
 * Active State: High
 */
#define DDRP_DLL_LOCK_TO_REG                       0x15c,  2,  2

/* [1:1]  RO  reset 0x0  --  lock_mpll
 * Reserved.
 */
#define DDRP_LOCK_MPLL                             0x15c,  1,  1

/* [0:0]  RO  reset 0x0  --  lock_pll_dqcmd
 * The lock signal of the PLL.
 * Active State: High
 * The PLL will be locked in 5000 reference clock cycles after the pll starts running.
 */
#define DDRP_LOCK_PLL_DQCMD                        0x15c,  0,  0


/* ----- 0x160 ----- */
/* [28:24]  RO  reset 0x0  --  drvlegpd_zqcali_2reg
 * The ZQ calibration result of driver pull down after the ZQ calibration training.
 */
#define DDRP_DRVLEGPD_ZQCALI_2REG                  0x160, 28, 24

/* [20:16]  RO  reset 0x0  --  drvlegpu_zqcali_2reg
 * The ZQ calibration result of driver pull up after the ZQ calibration training.
 */
#define DDRP_DRVLEGPU_ZQCALI_2REG                  0x160, 20, 16

/* [12:8]  RO  reset 0x0  --  odtlegpd_zqcali_2reg
 * The ZQ calibration result of ODT pull down after the ZQ calibration training.
 */
#define DDRP_ODTLEGPD_ZQCALI_2REG                  0x160, 12,  8

/* [4:0]  RO  reset 0x0  --  odtlegpu_zqcali_2reg
 * The ZQ calibration result of ODT pull up after the ZQ calibration training.
 */
#define DDRP_ODTLEGPU_ZQCALI_2REG                  0x160,  4,  0


/* ----- 0x164 ----- */
/* [28:28]  RO  reset 0x0  --  reg_zqcali_done
 * The complete signal of the ZQ calibration training.
 * Active State: High
 */
#define DDRP_ZQCALI_DONE                           0x164, 28, 28

/* [27:24]  RO  reset 0x0  --  rtrain_cnt_to_reg
 * Read training counter.
 */
#define DDRP_RTRAIN_CNT_TO_REG                     0x164, 27, 24

/* [23:20]  RO  reset 0x0  --  pvt_compensation_cnt_to_reg
 * PVT compensation counter.
 */
#define DDRP_PVT_COMPENSATION_CNT_TO_REG           0x164, 23, 20

/* [19:4]  RO  reset 0x0  --  osc_update_value
 * Oscillator counter.
 */
#define DDRP_OSC_UPDATE_VALUE                      0x164, 19,  4

/* [3:3]  RO  reset 0x0  --  reg_drvpd_overflow
 * The failure flag of driver pull-down ZQ calibration training.
 * Active State: High
 */
#define DDRP_DRVPD_OVERFLOW                        0x164,  3,  3

/* [2:2]  RO  reset 0x0  --  reg_drvpu_overflow
 * The failure flag of driver pull-up ZQ calibration training.
 * Active State: High
 */
#define DDRP_DRVPU_OVERFLOW                        0x164,  2,  2

/* [1:1]  RO  reset 0x0  --  reg_odtpd_overflow
 * The failure flag of ODT pull-down ZQ calibration training.
 * Active State: High
 */
#define DDRP_ODTPD_OVERFLOW                        0x164,  1,  1

/* [0:0]  RO  reset 0x0  --  reg_odtpu_overflow
 * The failure flag of ODT pull-up ZQ calibration training.
 * Active State: High
 */
#define DDRP_ODTPU_OVERFLOW                        0x164,  0,  0


/* ----- 0x168 ----- */
/* [16:8]  RO  reset 0x0  --  bist_error_dm
 * The DM error flag of each pad when PHY BIST.
 * [0]: A_DM0.
 * [1]: A_DM1.
 * [2]: B_DM0.
 * [3]: B_DM1.
 * [4]: C_DM0.
 * [5]: C_DM1.
 * [6]: D_DM0.
 * [7]: D_DM1.
 * [8]: E_DM0.
 */
#define DDRP_BIST_ERROR_DM                         0x168, 16,  8

/* [3:3]  RO  reset 0x0  --  cmd_error_flag
 * The error flag of CMD pads for PHY BIST.
 * Active State: High
 * If the bist_complete and cmd_error_flag are high at the same time, it means that errors occur on the CMD pads when PHY BIST.
 * You can read the register bist_error_cmd to find which pad has an error.
 */
#define DDRP_CMD_ERROR_FLAG                        0x168,  3,  3

/* [2:2]  RO  reset 0x0  --  dm_error_flag
 * The error flag of DM pads for PHY BIST.
 * Active State: High
 * If the bist_complet  and dm_error_flag are high at the same time, it means that errors occur on the DM pads when PHY BIST.
 * You can read the register bist_error_dm to find which pad has an error.
 */
#define DDRP_DM_ERROR_FLAG                         0x168,  2,  2

/* [1:1]  RO  reset 0x0  --  dq_error_flag
 * The error flag of DQ pads for PHY BIST.
 * Active State: High
 * If the bist_complete and dq_error_flag are high at the same time, it means that errors occur on the DQ pads when PHY BIST.
 * You can read the register bist_error_dq to find which pad has an error.
 */
#define DDRP_DQ_ERROR_FLAG                         0x168,  1,  1

/* [0:0]  RO  reset 0x0  --  bist_complete
 * The complete signal of the PHY BIST.
 * Active State: High
 */
#define DDRP_BIST_COMPLETE                         0x168,  0,  0


/* ----- 0x16c ----- */
/* [31:0]  RO  reset 0x0  --  bist_error_dq
 * The bist error flag of DQ pad.
 * Active State: High
 */
#define DDRP_BIST_ERROR_DQ                         0x16c, 31,  0


/* ----- 0x170 ----- */
/* [29:0]  RO  reset 0x0  --  bist_error_cmd
 * The BIST error flag of the CMD pad.
 * [0]: A0
 * [1]: A1
 * [2]: A2
 * [3]: A3
 * [4]: A4
 * [5]: A5
 * [6]: A6
 * [7]: A7
 * [8]: A8
 * [9]: A9
 * [10]: A10
 * [11]: A11
 * [12]: A12
 * [13]: A13
 * [14]: A14
 * [15]: A15
 * [16]: A16
 * [17]: A17
 * [18]: BA0
 * [19]: BA1
 * [20]: BG0
 * [21]: BG1
 * [22]: ACTN
 * [23]: CKE0
 * [24]: ODT0
 * [25]: RESETN
 * [26]: ODT1
 * [27]: CSB1
 * [28]: CKE1
 * [29]: CSB0
 */
#define DDRP_BIST_ERROR_CMD                        0x170, 29,  0


/* ----- 0x174 ----- */
/* [24:16]  RO  reset 0x0  --  wl_done_byte
 * The write leveling complete signal of each channel.
 * Active State: High
 * [0]: The write leveling complete of byte0.
 * [1]: The write leveling complete of byte1.
 * [2]: The write leveling complete of byte2.
 * …
 * [8]: The write leveling complete of byte8.
 */
#define DDRP_WL_DONE_BYTE                          0x174, 24, 16

/* [11:11]  RO  reset 0x0  --  reg_wl_end
 * The write leveling complete signal.
 * Active State: High
 * Changed to 1’b1 only when all open channels have completed the write leveling.
 */
#define DDRP_WL_END                                0x174, 11, 11

/* [10:10]  RO  reset 0x0  --  calib_end
 * The Rx-DQS calibration complete signal.
 * Active State: High
 * Changed to 1’b1 only when all open channels have completed the Rx DQS calibration.
 */
#define DDRP_CALIB_END                             0x174, 10, 10

/* [9:9]  RO  reset 0x0  --  calib_error
 * The error flag of the Rx DQS calibration.
 * Active State: High
 */
#define DDRP_CALIB_ERROR                           0x174,  9,  9

/* [8:0]  RO  reset 0x0  --  calib_done_byte
 * The Rx-DQS calibration complete signal of each channel.
 * Active State: High
 * [0]: byte0.
 * [1]: byte1.
 * [2]: byte2
 * …
 * [n-1]: byte8
 */
#define DDRP_CALIB_DONE_BYTE                       0x174,  8,  0


/* ----- 0x178 ----- */
/* [19:18]  RO  reset 0x0  --  cha_rank_cat_bp_cmd_send_rdy
 * Note: This register is not available for this design.
 * The ready signal of the command bus training bypass mode of Channel A.
 * Active State: High
 * After the command bus training bypass mode is enabled, wait for it to be high, which indicates that the SDRAM has already entered the command bus training mode. Then, you can use the register to send the CBT command to do the command bus training.
 * [1]: Rank 1
 * [0]: Rank 0
 */
#define DDRP_CHA_RANK_CAT_BP_CMD_SEND_RDY          0x178, 19, 18

/* [15:14]  RO  reset 0x0  --  cha_rank_cat_bp_done
 * Note: This register is not available for this design.
 * The complete signal of the command bus training of Channel A.
 * Active State: High
 * After the command bus training bypass mode is enabled and the registers are used to complete the training, set reg_cat_bp_start to low to stop the bypass mode. Then, the PHY will exit the command bus training mode and set this register to 1. Set reg_cat_bp_ento low to exit the command bus training mode.
 * [1]: Rank 1
 * [0]: Rank 0
 */
#define DDRP_CHA_RANK_CAT_BP_DONE                  0x178, 15, 14

/* [11:0]  RO  reset 0x0  --  ca_check_value
 * Note: This register is not available for this design.
 * The command bus training read back data from the A_DQ[5:0] of Channel A and B_DQ[5:0] of Channel B when the command bus training bypass mode is enabled.
 * When the command bus training bypass mode is enabled and the PHY goes into the comand bus training mode, if the PHY sends the CBT command, the DQ will read back the data on the command bus and store the data in this register.
 * Read this register to get the read back data to judge whether the curren Tx delay line is correct or not.
 */
#define DDRP_CA_CHECK_VALUE                        0x178, 11,  0


/* ----- 0x17c ----- */
/* [17:8]  RO  reset 0x0  --  cha_cat_auto_cs_train_err
 * The error flag of the auto command bus CS training of Channel A.
 * [0]: The training result of CSB for RANK0 is abnormal
 * [1]: The training result of CSB for RANK1 is abnormal
 * [2]: Can't find the vref pass window of Channel A CSB for RANK0.
 * [3]: Can't find the vref pass window of Channel A CSB for RANK1.
 * [4]: Can't find the Tx delay line pass window of CSB for RANK0.
 * [5]: Can't find the Tx delay line pass window of CSB for RANK1.
 * [6]: The start point of CSB0 training is abnormal. The training will fail.
 * [7]: The start point of CSB1 training is abnormal. The training will fail.
 */
#define DDRP_CHA_CAT_AUTO_CS_TRAIN_ERR             0x17c, 17,  8

/* [2:2]  RO  reset 0x0  --  cha_cat_done
 * The complete signal of the auto command bus training of Channel A.
 * Active State: High
 */
#define DDRP_CHA_CAT_DONE                          0x17c,  2,  2

/* [0:0]  RO  reset 0x0  --  cha_cat_bp_cmd_send_done
 * Note: This register is not available for this design.
 * The complete signal of Channel A that the PHY has sent the CBT command when the command bus training bypass mode is enabled.
 * Active State: High
 * You need to wait this register to change to 1’b1 to send the next CBT command.
 */
#define DDRP_CHA_CAT_BP_CMD_SEND_DONE              0x17c,  0,  0


/* ----- 0x180 ----- */
/* [23:16]  RO  reset 0x0  --  reg_cmd_invdelaysel
 * The command delay line value of the current observation signal choosing of reg_cmd_invdelaysel_sel.
 */
#define DDRP_CMD_INVDELAYSEL                       0x180, 23, 16

/* [14:8]  RO  reset 0x0  --  reg_wrtrain_vref_max_value
 * The max pass point of the Tx SDRAM's DQ Vref.
 */
#define DDRP_WRTRAIN_VREF_MAX_VALUE                0x180, 14,  8

/* [6:0]  RO  reset 0x4f  --  reg_wrtrain_vref_min_value
 * The min pass point of the Tx SDRAM's DQ Vref.
 */
#define DDRP_WRTRAIN_VREF_MIN_VALUE                0x180,  6,  0


/* ----- 0x184 ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank0_ca0_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CA0.
 */
#define DDRP_CHA_RANK0_CA0_MIN_PERBIT_SKEW_PASS    0x184, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank0_ca1_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CA1.
 */
#define DDRP_CHA_RANK0_CA1_MIN_PERBIT_SKEW_PASS    0x184, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank0_ca2_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CA2.
 */
#define DDRP_CHA_RANK0_CA2_MIN_PERBIT_SKEW_PASS    0x184, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank0_ca3_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CA3.
 */
#define DDRP_CHA_RANK0_CA3_MIN_PERBIT_SKEW_PASS    0x184,  7,  0


/* ----- 0x188 ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank0_ca4_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CA4.
 */
#define DDRP_CHA_RANK0_CA4_MIN_PERBIT_SKEW_PASS    0x188, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank0_ca5_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CA5.
 */
#define DDRP_CHA_RANK0_CA5_MIN_PERBIT_SKEW_PASS    0x188, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank1_ca0_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CA0.
 */
#define DDRP_CHA_RANK1_CA0_MIN_PERBIT_SKEW_PASS    0x188, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank1_ca1_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CA1.
 */
#define DDRP_CHA_RANK1_CA1_MIN_PERBIT_SKEW_PASS    0x188,  7,  0


/* ----- 0x18c ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank1_ca2_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CA2.
 */
#define DDRP_CHA_RANK1_CA2_MIN_PERBIT_SKEW_PASS    0x18c, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank1_ca3_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CA3.
 */
#define DDRP_CHA_RANK1_CA3_MIN_PERBIT_SKEW_PASS    0x18c, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank1_ca4_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CA4.
 */
#define DDRP_CHA_RANK1_CA4_MIN_PERBIT_SKEW_PASS    0x18c, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank1_ca5_min_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CA5.
 */
#define DDRP_CHA_RANK1_CA5_MIN_PERBIT_SKEW_PASS    0x18c,  7,  0


/* ----- 0x190 ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank0_min_cs_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK0 Channel A CS.
 */
#define DDRP_CHA_RANK0_MIN_CS_PERBIT_SKEW_PASS     0x190, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank1_min_cs_perbit_skew_pass
 * The min pass point of the Tx delay line for RANK1 Channel A CS.
 */
#define DDRP_CHA_RANK1_MIN_CS_PERBIT_SKEW_PASS     0x190, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank0_max_cs_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CS.
 */
#define DDRP_CHA_RANK0_MAX_CS_PERBIT_SKEW_PASS     0x190, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank1_max_cs_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CS.
 */
#define DDRP_CHA_RANK1_MAX_CS_PERBIT_SKEW_PASS     0x190,  7,  0


/* ----- 0x194 ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank0_ca0_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CA0.
 */
#define DDRP_CHA_RANK0_CA0_MAX_PERBIT_SKEW_PASS    0x194, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank0_ca1_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CA1.
 */
#define DDRP_CHA_RANK0_CA1_MAX_PERBIT_SKEW_PASS    0x194, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank0_ca2_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CA2.
 */
#define DDRP_CHA_RANK0_CA2_MAX_PERBIT_SKEW_PASS    0x194, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank0_ca3_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CA3.
 */
#define DDRP_CHA_RANK0_CA3_MAX_PERBIT_SKEW_PASS    0x194,  7,  0


/* ----- 0x198 ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank0_ca4_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CA4.
 */
#define DDRP_CHA_RANK0_CA4_MAX_PERBIT_SKEW_PASS    0x198, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank0_ca5_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK0 Channel A CA5.
 */
#define DDRP_CHA_RANK0_CA5_MAX_PERBIT_SKEW_PASS    0x198, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank1_ca0_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CA0.
 */
#define DDRP_CHA_RANK1_CA0_MAX_PERBIT_SKEW_PASS    0x198, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank1_ca1_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CA1.
 */
#define DDRP_CHA_RANK1_CA1_MAX_PERBIT_SKEW_PASS    0x198,  7,  0


/* ----- 0x19c ----- */
/* [31:24]  RO  reset 0x0  --  cha_rank1_ca2_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CA2.
 */
#define DDRP_CHA_RANK1_CA2_MAX_PERBIT_SKEW_PASS    0x19c, 31, 24

/* [23:16]  RO  reset 0x0  --  cha_rank1_ca3_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CA3.
 */
#define DDRP_CHA_RANK1_CA3_MAX_PERBIT_SKEW_PASS    0x19c, 23, 16

/* [15:8]  RO  reset 0x0  --  cha_rank1_ca4_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CA4.
 */
#define DDRP_CHA_RANK1_CA4_MAX_PERBIT_SKEW_PASS    0x19c, 15,  8

/* [7:0]  RO  reset 0x0  --  cha_rank1_ca5_max_perbit_skew_pass
 * The max pass point of the Tx delay line for RANK1 Channel A CA5.
 */
#define DDRP_CHA_RANK1_CA5_MAX_PERBIT_SKEW_PASS    0x19c,  7,  0


/* ----- 0x1bc ----- */
/* [29:24]  RO  reset 0x0  --  cha_rank0_min_cs_vref_pass
 * The min pass point of the Tx SDRAM's Vref for RANK0 Channel A CS.
 */
#define DDRP_CHA_RANK0_MIN_CS_VREF_PASS            0x1bc, 29, 24

/* [21:16]  RO  reset 0x0  --  cha_rank0_max_cs_vref_pass
 * The max pass point of the Tx SDRAM's Vref for RANK0 Channel A CS.
 */
#define DDRP_CHA_RANK0_MAX_CS_VREF_PASS            0x1bc, 21, 16

/* [13:8]  RO  reset 0x0  --  cha_rank1_min_cs_vref_pass
 * The min pass point of the Tx SDRAM's Vref for RANK1 Channel A CS.
 */
#define DDRP_CHA_RANK1_MIN_CS_VREF_PASS            0x1bc, 13,  8

/* [5:0]  RO  reset 0x0  --  cha_rank1_max_cs_vref_pass
 * The max pass point of the Tx SDRAM's Vref for RANK1 Channel A CS.
 */
#define DDRP_CHA_RANK1_MAX_CS_VREF_PASS            0x1bc,  5,  0


/* ----- 0x1c4 ----- */
/* [26:18]  RO  reset 0x0  --  reg_train_error_for_rd_byte
 * The error signal of read training.
 * Active State: High
 * [0]: The error signal of the read training for byte0.
 * [1]: The error signal of the read training for byte1.
 * [2]: The error signal of the read training for byte2.
 * ...
 * [8]: The error signal of the read training for byte8
 */
#define DDRP_TRAIN_ERROR_FOR_RD_BYTE               0x1c4, 26, 18

/* [17:9]  RO  reset 0x0  --  reg_wr_train_done_byte
 * The done signal of write training.
 * Active State: High
 * [0]: The done signal of the write training for byte0.
 * [1]: The done signal of the write training for byte1.
 * [2]: The done signal of the write training for byte2.
 * ...
 * [8]: The done signal of the write training for byte8
 */
#define DDRP_WR_TRAIN_DONE_BYTE                    0x1c4, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_wr_train_error_byte
 * The error signal of write training.
 * Active State: High
 * [0]: The error signal of the write training for byte0.
 * [1]: The error signal of the write training for byte1.
 * [2]: The error signal of the write training for byte2.
 * ...
 * [8: The error signal of the write training for byte8
 */
#define DDRP_WR_TRAIN_ERROR_BYTE                   0x1c4,  8,  0


/* ----- 0x1c8 ----- */
/* [31:23]  RO  reset 0x0  --  reg_train_done_for_rd_to_reg_byte
 * The done signal of read training.
 * Active State: High
 * [0]: The done signal of the read training for byte0.
 * [1]: The done signal of the read training for byte1.
 * [2]: The done signal of the read training for byte2.
 * ...
 * [8]: The done signal of the read training for byte8
 */
#define DDRP_TRAIN_DONE_FOR_RD_TO_REG_BYTE         0x1c8, 31, 23

/* [16:16]  RO  reset 0x0  --  mdll_timeout_to_reg
 * The flag of timeout for the master DLL lock.
 * Active State: High
 */
#define DDRP_MDLL_TIMEOUT_TO_REG                   0x1c8, 16, 16

/* [14:8]  RO  reset 0x0  --  mdll_update_cnt
 * Used to store the times that the PVT compensation occurs.
 */
#define DDRP_MDLL_UPDATE_CNT                       0x1c8, 14,  8

/* [7:0]  RO  reset 0x10  --  halfui_lock_code_to_reg
 * The lock value of the master DLL.
 */
#define DDRP_HALFUI_LOCK_CODE_TO_REG               0x1c8,  7,  0


/* ----- 0x1cc ----- */
/* [31:24]  RW  reset 0x2b  --  group1_dm_train_check_data_value0
 * For the write training of DDR4, it is the DM pattern of the first burst8.
 * For the write training of LPDDR4, it is the DM pattern of the previous 8 bits of the first burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE0     0x1cc, 31, 24

/* [23:16]  RW  reset 0x5f  --  group1_dm_train_check_data_value1
 * For the write training of DDR4, it is the DM pattern of the second burst8.
 * For the write training of LPDDR4, it is the DM pattern of the last 8 bits of the first burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE1     0x1cc, 23, 16

/* [15:8]  RW  reset 0x38  --  group1_dm_train_check_data_value2
 * For the write training of DDR4, it is the DM pattern of the third burst8.
 * For the write training of LPDDR4, it is the DM pattern of the previous 8 bits of the second burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE2     0x1cc, 15,  8

/* [7:0]  RW  reset 0x92  --  group1_dm_train_check_data_value3
 * For the write training of DDR4, it is the DM pattern of the fourth burst8.
 * For the write training of LPDDR4, it is the DM pattern of the last 8 bits of the second burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE3     0x1cc,  7,  0


/* ----- 0x1d0 ----- */
/* [28:24]  RO  reset 0x0  --  drvleg_zqcali_s01_low_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S01_LOW_TO_REG          0x1d0, 28, 24

/* [20:16]  RO  reset 0x0  --  drvleg_zqcali_s01_high_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S01_HIGH_TO_REG         0x1d0, 20, 16

/* [12:8]  RO  reset 0x0  --  drvleg_zqcali_s00_low_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S00_LOW_TO_REG          0x1d0, 12,  8

/* [4:0]  RO  reset 0x0  --  drvleg_zqcali_s00_high_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S00_HIGH_TO_REG         0x1d0,  4,  0


/* ----- 0x1d4 ----- */
/* [28:24]  RO  reset 0x0  --  drvleg_zqcali_s11_low_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S11_LOW_TO_REG          0x1d4, 28, 24

/* [20:16]  RO  reset 0x0  --  drvleg_zqcali_s11_high_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S11_HIGH_TO_REG         0x1d4, 20, 16

/* [12:8]  RO  reset 0x0  --  drvleg_zqcali_s10_low_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S10_LOW_TO_REG          0x1d4, 12,  8

/* [4:0]  RO  reset 0x0  --  drvleg_zqcali_s10_high_to_reg
 * the observed singnal of zqcalib of phy
 */
#define DDRP_DRVLEG_ZQCALI_S10_HIGH_TO_REG         0x1d4,  4,  0


/* ----- 0x1e0 ----- */
/* [31:24]  RW  reset 0xad  --  group1_dm_train_check_data_value4
 * For the write training of DDR4, it is the DM pattern of the fifth burst8.
 * For the write training of LPDDR4, it is the DM pattern of the previous 8 bits of the third burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE4     0x1e0, 31, 24

/* [23:16]  RW  reset 0xbd  --  group1_dm_train_check_data_value5
 * For the write training of DDR4, it is the DM pattern of the sixth burst8.
 * For the write training of LPDDR4, it is the DM pattern of the last 8 bits of the third burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE5     0x1e0, 23, 16

/* [15:8]  RW  reset 0xb1  --  group1_dm_train_check_data_value6
 * For the write training of DDR4, it is the DM pattern of the seventh burst8.
 * For the write training of LPDDR4, it is the DM pattern of the previous 8 bits of the fourth burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE6     0x1e0, 15,  8

/* [7:0]  RW  reset 0x74  --  group1_dm_train_check_data_value7
 * For the write training of DDR4, it is the DM pattern of the eighth burst8.
 * For the write training of LPDDR4, it is the DM pattern of the last 8 bits of the fourth burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE7     0x1e0,  7,  0


/* ----- 0x1e4 ----- */
/* [31:28]  RW  reset 0x0  --  reg_lp4_odt_bypass_value
 * LPDDR4 ODT bypass configuration value.
 * [0]: ODT0
 * [1]: ODT1
 * [2]: ODT2
 * [3]: ODT3
 */
#define DDRP_LP4_ODT_BYPASS_VALUE                  0x1e4, 31, 28

/* [27:27]  RW  reset 0x0  --  reg_lp4_odt_bypass_en
 * The LPDDR4 ODT bypass enable signal which will configure reg_lp4_odt_bypass_value for ODT0/ODT1/ODT2/ODT3.
 * Active State: High
 */
#define DDRP_LP4_ODT_BYPASS_EN                     0x1e4, 27, 27

/* [26:20]  RW  reset 0x00  --  reg_wrtrain_vref_scan_min
 * The minimum scan value of write training Vref.
 */
#define DDRP_WRTRAIN_VREF_SCAN_MIN                 0x1e4, 26, 20

/* [15:8]  RW  reset 0x67  --  group1_dm_train_check_data_value8
 * For the write training of DDR4, it is the DM pattern of the ninth burst8.
 * For the write training of LPDDR4, it is the DM pattern of the previous 8 bits of the fifth burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE8     0x1e4, 15,  8

/* [7:0]  RW  reset 0xaa  --  group1_dm_train_check_data_value9
 * For the write training of DDR4, it is the DM pattern of the tenth burst8.
 * For the write training of LPDDR4, it is the DM pattern of the last 8 bits of the fifth burst16.
 */
#define DDRP_GROUP1_DM_TRAIN_CHECK_DATA_VALUE9     0x1e4,  7,  0


/* ----- 0x1e8 ----- */
/* [24:17]  RO  reset 0x10  --  halfui_lock_code
 * Reserved
 */
#define DDRP_HALFUI_LOCK_CODE                      0x1e8, 24, 17

/* [16:9]  RO  reset 0x0  --  bist_error_dq_byte8
 * Reserved
 */
#define DDRP_BIST_ERROR_DQ_BYTE8                   0x1e8, 16,  9

/* [8:0]  RO  reset 0x0  --  reg_rd_train_readback_data_valid_byte
 * The read back data valid signal when the read training bypass mode is enabled.
 * Active State: High.
 * When the read training bypass mode is enabled, after sending out the read command and the read back data will be received by the PHY and stored in the following registers. When this bit is high, it means the read back data is valid.
 * [0]: The read back data valid signal of byte0.
 * [1]: The read back data valid signal of byte1.
 * [2]: The read back data valid signal of byte2.
 * ...
 * [8]: The read back data valid signal of byte8.
 */
#define DDRP_RD_TRAIN_READBACK_DATA_VALID_BYTE     0x1e8,  8,  0


/* ----- 0x1ec ----- */
/* [31:0]  RO  reset 0x0  --  bist_error_dq_byte4_7
 * Reserved
 */
#define DDRP_BIST_ERROR_DQ_BYTE4_7                 0x1ec, 31,  0


/* ----- 0x1f0 ----- */
/* [31:0]  RO  reset 0x0  --  reg_dfx_pvt_cmp_read_back_data
 * Reserved.
 */
#define DDRP_DFX_PVT_CMP_READ_BACK_DATA            0x1f0, 31,  0


/* ----- 0x1f4 ----- */
/* [31:0]  RW  reset 0x7fffff  --  reg_cmd_2t_mode_value
 * Reserved.
 */
#define DDRP_CMD_2T_MODE_VALUE                     0x1f4, 31,  0


/* ----- 0x1f8 ----- */
/* [31:31]  RW  reset 0x0  --  reg_voltage_current_test_mode_rx
 * IO voltage and current_test_mode of rx. Active High.
 */
#define DDRP_VOLTAGE_CURRENT_TEST_MODE_RX          0x1f8, 31, 31

/* [30:30]  RW  reset 0x1  --  reg_bg_mirror
 * 1: bg mirror
 * 0: bg not mirror
 */
#define DDRP_BG_MIRROR                             0x1f8, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_mirror_cs_inv
 * 0: CS0 stands for rank0
 * 1: CS1 stands for rank1
 */
#define DDRP_MIRROR_CS_INV                         0x1f8, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_dfi_mirror_by_ctrl
 * The control signal of dfi mirror.
 * 0:when mirror enable and this signal is equal0, dfi will send address/ba/bg that mirrored.
 * 1:dfi will send address/ba/bg as normal.
 */
#define DDRP_DFI_MIRROR_BY_CTRL                    0x1f8, 28, 28

/* [27:27]  RW  reset 0x0  --  reg_ddr3_mirror
 * The enable of ddr3 mirror
 */
#define DDRP_DDR3_MIRROR                           0x1f8, 27, 27

/* [26:26]  RW  reset 0x0  --  reg_ddr4_mirror
 * The enable of ddr4 mirror
 */
#define DDRP_DDR4_MIRROR                           0x1f8, 26, 26

/* [25:25]  RW  reset 0x0  --  reg_dfx_pvt_cmp_cnt_clr
 * Reserved.
 */
#define DDRP_DFX_PVT_CMP_CNT_CLR                   0x1f8, 25, 25

/* [24:17]  RW  reset 0x0  --  reg_dfx_pvt_data_sel
 * Reserved.
 */
#define DDRP_DFX_PVT_DATA_SEL                      0x1f8, 24, 17

/* [16:13]  RW  reset 0x1  --  reg_rtrain_rank_num
 * Reserved.
 */
#define DDRP_RTRAIN_RANK_NUM                       0x1f8, 16, 13

/* [12:5]  RW  reset 0x20  --  reg_rtrain_chk_done_prd
 * Reserved.
 */
#define DDRP_RTRAIN_CHK_DONE_PRD                   0x1f8, 12,  5

/* [4:4]  RW  reset 0x0  --  reg_rtrain_cnt_clear
 * Reserved.
 */
#define DDRP_RTRAIN_CNT_CLEAR                      0x1f8,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_pvt_compensation_cnt_clear
 * Reserved.
 */
#define DDRP_PVT_COMPENSATION_CNT_CLEAR            0x1f8,  3,  3

/* [1:1]  RW  reset 0x0  --  reg_ddrphy_pvt_rtrain_en
 * Reserved.
 */
#define DDRP_DDRPHY_PVT_RTRAIN_EN                  0x1f8,  1,  1


/* ----- 0x1fc ----- */
/* [31:24]  RW  reset 0xf0  --  reg_rdqs_retrain_start_read_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_START_READ_CNT           0x1fc, 31, 24

/* [23:13]  RW  reset 0x0  --  reg_rdqs_retrain_pi_adj_step
 *
 */
#define DDRP_RDQS_RETRAIN_PI_ADJ_STEP              0x1fc, 23, 13

/* [12:7]  RW  reset 0x0  --  reg_ddrphy_wdqsoff_length
 * The lpddr4 wdqsoff parameter,the unit is tCK
 */
#define DDRP_DDRPHY_WDQSOFF_LENGTH                 0x1fc, 12,  7

/* [6:1]  RW  reset 0x0  --  reg_ddrphy_wdqson_length
 * The lpddr4 wdqson parameter,the unit is tCK
 */
#define DDRP_DDRPHY_WDQSON_LENGTH                  0x1fc,  6,  1

/* [0:0]  RW  reset 0x0  --  reg_ddrphy_wdqs_crtl_mode2_en
 * The lpddr4 wdqs mode2 enable signal
 */
#define DDRP_DDRPHY_WDQS_CRTL_MODE2_EN             0x1fc,  0,  0


/* ----- 0x1c20 ----- */
/* [31:29]  RW  reset 0x0  --  reg_ctrlupd_retrain_dfx_cnt_loop_sel
 * The total retrain counters select signal. 3'd0: total counters, 3'd1: wl retrain counters, 3'd2: dqs gate retrain counters,  3'd3: read retrain counters, 3'd4: write retrain counters,
 */
#define DDRP_CTRLUPD_RETRAIN_DFX_CNT_LOOP_SEL      0x1c20, 31, 29

/* [28:27]  RW  reset 0x0  --  reg_dfx_retrain_byte_loop_sel
 * The retrain result counters select signal of BYTE. 2'b00: BYTE0, 2'b01: BYTE1, 2'b10: BYTE2, 2'b11: BYTE3.
 */
#define DDRP_DFX_RETRAIN_BYTE_LOOP_SEL             0x1c20, 28, 27

/* [26:23]  RW  reset 0x0  --  reg_dfx_retrain_dq_loop_sel
 * The retrain result counters select signal of DQ. 4'd0~4'd7 according to DQ0~DQ7, 4'd8 according to DM
 */
#define DDRP_DFX_RETRAIN_DQ_LOOP_SEL               0x1c20, 26, 23

/* [22:19]  RW  reset 0x0  --  reg_dfx_retrain_type_loop_sel
 * The retrain result counters select signal of retrain type (exclude wl).
 */
#define DDRP_DFX_RETRAIN_TYPE_LOOP_SEL             0x1c20, 22, 19

/* [18:17]  RW  reset 0x0  --  reg_dfx_retrain_wl_loop_sel
 * The wl retrain result counters select signal.
 */
#define DDRP_DFX_RETRAIN_WL_LOOP_SEL               0x1c20, 18, 17

/* [16:15]  RW  reset 0x0  --  reg_ctrlupd_retrain_rank_enable
 *
 */
#define DDRP_CTRLUPD_RETRAIN_RANK_ENABLE           0x1c20, 16, 15

/* [14:9]  RW  reset 0x28  --  reg_wl_wait_dq_time
 *
 */
#define DDRP_WL_WAIT_DQ_TIME                       0x1c20, 14,  9

/* [8:5]  RW  reset 0x3  --  reg_wl_retrain_range
 *
 */
#define DDRP_WL_RETRAIN_RANGE                      0x1c20,  8,  5

/* [4:4]  RW  reset 0x0  --  reg_ctrlupd_retrain_en
 *
 */
#define DDRP_CTRLUPD_RETRAIN_EN                    0x1c20,  4,  4

/* [3:3]  RW  reset 0x0  --  reg_ctrlupd_wl_en
 *
 */
#define DDRP_CTRLUPD_WL_EN                         0x1c20,  3,  3

/* [2:2]  RW  reset 0x0  --  reg_ctrlupd_dqs_gate_en
 *
 */
#define DDRP_CTRLUPD_DQS_GATE_EN                   0x1c20,  2,  2

/* [1:1]  RW  reset 0x0  --  reg_ctrlupd_read_en
 *
 */
#define DDRP_CTRLUPD_READ_EN                       0x1c20,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_ctrlupd_write_en
 *
 */
#define DDRP_CTRLUPD_WRITE_EN                      0x1c20,  0,  0


/* ----- 0x1c24 ----- */
/* [31:30]  RW  reset 0x0  --  reg_ctrlupd_req_type
 * 0:no used
 * 1:ctrlupd type1
 * 2:ctrlupd type2
 */
#define DDRP_CTRLUPD_REQ_TYPE                      0x1c24, 31, 30

/* [29:28]  RW  reset 0x3  --  reg_dfx_signal_rank_sel
 * reg_ctrlupd_fast_retrain_dfx_cnt rank select.
 * 11:select all ranks to be counted;
 * 01:select rank0 to be counted;
 * 10:select rank1 to be counted;
 * 00:no ranks to be counted;
 */
#define DDRP_DFX_SIGNAL_RANK_SEL                   0x1c24, 29, 28

/* [27:22]  RW  reset 0x3f  --  reg_rdqs_retrain_read_interval
 *
 */
#define DDRP_RDQS_RETRAIN_READ_INTERVAL            0x1c24, 27, 22

/* [21:11]  RW  reset 0x0  --  reg_rdqs_retrain_pi_offset
 *
 */
#define DDRP_RDQS_RETRAIN_PI_OFFSET                0x1c24, 21, 11

/* [10:0]  RW  reset 0x0  --  reg_rd_retrain_pi_adj_step
 *
 */
#define DDRP_RD_RETRAIN_PI_ADJ_STEP                0x1c24, 10,  0


/* ----- 0x1c28 ----- */
/* [31:0]  RO  reset 0x0  --  reg_ctrlupd_fast_retrain_dfx
 * The fast retrain result counters.
 */
#define DDRP_CTRLUPD_FAST_RETRAIN_DFX              0x1c28, 31,  0


/* ----- 0x1c2c ----- */
/* [31:31]  RW  reset 0x0  --  reg_ctrlupd_req_type_bypass
 * 0: use dfi ctrlupd_type
 * 1: use register ctrlupd_type
 */
#define DDRP_CTRLUPD_REQ_TYPE_BYPASS               0x1c2c, 31, 31

/* [30:22]  RW  reset 0xe0  --  reg_rdqs_retrain_active1_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_ACTIVE1_CNT              0x1c2c, 30, 22

/* [21:11]  RW  reset 0x0  --  reg_wr_retrain_pi_offset
 *
 */
#define DDRP_WR_RETRAIN_PI_OFFSET                  0x1c2c, 21, 11

/* [10:0]  RW  reset 0x0  --  reg_wr_retrain_pi_adj_step
 *
 */
#define DDRP_WR_RETRAIN_PI_ADJ_STEP                0x1c2c, 10,  0


/* ----- 0x1c30 ----- */
/* [26:18]  RW  reset 0xf0  --  reg_rdqs_retrain_read_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_READ_CNT                 0x1c30, 26, 18

/* [17:9]  RW  reset 0xc0  --  reg_rdqs_retrain_active0_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_ACTIVE0_CNT              0x1c30, 17,  9

/* [8:0]  RW  reset 0xa0  --  reg_rdqs_retrain_precharge_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_PRECHARGE_CNT            0x1c30,  8,  0


/* ----- 0x1c34 ----- */
/* [28:18]  RW  reset 0x0  --  reg_rd_retrain_pi_offset
 *
 */
#define DDRP_RD_RETRAIN_PI_OFFSET                  0x1c34, 28, 18

/* [17:9]  RW  reset 0x30  --  reg_rdqs_retrain_mpr_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_MPR_CNT                  0x1c34, 17,  9

/* [8:0]  RW  reset 0x64  --  reg_rdqs_retrain_preamble_cnt
 *
 */
#define DDRP_RDQS_RETRAIN_PREAMBLE_CNT             0x1c34,  8,  0


/* ----- 0x1c38 ----- */
/* [31:0]  RO  reset 0x0  --  reg_ctrlupd_retrain_dfx_cnt
 * The total retrain counters.
 */
#define DDRP_CTRLUPD_RETRAIN_DFX_CNT               0x1c38, 31,  0


/* ----- 0x1c3c ----- */
/* [31:0]  RO  reset 0x0  --  reg_ctrlupd_wl_retrain_dfx
 * The wl retrain result counters.
 */
#define DDRP_CTRLUPD_WL_RETRAIN_DFX                0x1c3c, 31,  0


/* ----- 0x1c40 ----- */
/* [31:31]  RW  reset 0x0  --  reg_voltage_current_test_mode
 * IO voltage and current_test_mode of tx. Active High.
 */
#define DDRP_VOLTAGE_CURRENT_TEST_MODE             0x1c40, 31, 31

/* [30:0]  RW  reset 0x0  --  reg_cmd_pad_test_pattern
 * CMD IO voltage and current_test pattern of tx.  When set to 1, CMD IO  keep 1.Refer to Section 4.2.1 CMD Pad Map for the default corresponding relationship to ddr4 and the the corresponding relationship depends on the pin wrap sel.
 */
#define DDRP_CMD_PAD_TEST_PATTERN                  0x1c40, 30,  0


/* ----- 0x1c44 ----- */
/* [31:31]  RW  reset 0x0  --  reg_cmd_io_highz_mode
 * cmd io highz enable signal.
 */
#define DDRP_CMD_IO_HIGHZ_MODE                     0x1c44, 31, 31

/* [30:0]  RW  reset 0x0  --  reg_cmd_io_highz
 * set 1 to highz io of CMD.
 */
#define DDRP_CMD_IO_HIGHZ                          0x1c44, 30,  0


/* ----- 0x1c48 ----- */
/* [31:0]  RO  reset 0x0  --  wl_rxdq_to_wrap
 * The check pattern of rx in IO voltage and current_test_mode, depends on DQ IO value.
 * [7:0]: byte0
 * [15:8]: byte1
 * [23:16]: byte2
 * [31:24]: byte3
 */
#define DDRP_WL_RXDQ_TO_WRAP                       0x1c48, 31,  0


/* ----- 0x1c4c ----- */
/* [8:8]  RO  reset 0x0  --  cat_done_lpddr3
 * The lpddr3 cat done signal
 */
#define DDRP_CAT_DONE_LPDDR3                       0x1c4c,  8,  8

/* [7:4]  RO  reset 0x0  --  wl_rxdqs_to_wrap
 * The check pattern of rx in IO voltage and current_test_mode, depends on DQS IO value.
 * [0]: A_DQS0
 * [1]: A_DQS1
 * [2]: B_DQS0
 * [3]: B_DQS1
 */
#define DDRP_WL_RXDQS_TO_WRAP                      0x1c4c,  7,  4

/* [3:0]  RO  reset 0x0  --  wl_rxdm_to_wrap
 * The check pattern of rx in IO voltage and current_test_mode, depends on DM IO value.
 * [0]: A_DM0
 * [1]: A_DM1
 * [2]: B_DM0
 * [3]: B_DM1
 */
#define DDRP_WL_RXDM_TO_WRAP                       0x1c4c,  3,  0


/* ----- 0x1c50 ----- */
/* [31:16]  RW  reset 0x0  --  reg_lpddr3_load_mode00
 * lpddr3 load mode 0 value
 */
#define DDRP_LPDDR3_LOAD_MODE00                    0x1c50, 31, 16

/* [15:0]  RW  reset 0x0  --  reg_lpddr3_load_mode02
 * lpddr3 load mode 1 value
 */
#define DDRP_LPDDR3_LOAD_MODE02                    0x1c50, 15,  0


/* ----- 0x1c54 ----- */
/* [31:16]  RW  reset 0x0  --  reg_lpddr3_load_mode01
 * lpddr3 load mode 2 value
 */
#define DDRP_LPDDR3_LOAD_MODE01                    0x1c54, 31, 16

/* [15:0]  RW  reset 0x0  --  reg_lpddr3_load_mode48
 * lpddr3 load mode 48 value
 */
#define DDRP_LPDDR3_LOAD_MODE48                    0x1c54, 15,  0


/* ----- 0x1c58 ----- */
/* [31:16]  RW  reset 0x0  --  reg_lpddr3_load_mode42
 * lpddr3 load mode 42 value
 */
#define DDRP_LPDDR3_LOAD_MODE42                    0x1c58, 31, 16

/* [15:0]  RW  reset 0x0  --  reg_lpddr3_load_mode41
 * lpddr3 load mode 41 value
 */
#define DDRP_LPDDR3_LOAD_MODE41                    0x1c58, 15,  0


/* ----- 0x1c5c ----- */
/* [31:22]  RW  reset 0x101  --  reg_cat_ca_check_value_lpddr3
 * lpddr3 ca train check pattern of cha
 */
#define DDRP_CAT_CA_CHECK_VALUE_LPDDR3             0x1c5c, 31, 22

/* [21:12]  RW  reset 0x101  --  reg_cat_ca_train_value_lpddr3
 * lpddr3 ca train value of cha
 */
#define DDRP_CAT_CA_TRAIN_VALUE_LPDDR3             0x1c5c, 21, 12

/* [11:8]  RW  reset 0xf  --  reg_lpddr3_tmrw
 * lpddr3 tmrw . Unit dfi_1xclk
 */
#define DDRP_LPDDR3_TMRW                           0x1c5c, 11,  8

/* [7:4]  RW  reset 0xa  --  reg_tcackel_lpddr3
 * used for lpddr3 ca train tcackel timing. Unit dfi_c1xclk
 */
#define DDRP_TCACKEL_LPDDR3                        0x1c5c,  7,  4

/* [3:0]  RW  reset 0xa  --  reg_tcaent_lpddr3
 * used for lpddr3 ca train tcaent timing . Unit dfi_1xclk
 */
#define DDRP_TCAENT_LPDDR3                         0x1c5c,  3,  0


/* ----- 0x1c60 ----- */
/* [31:31]  RW  reset 0x0  --  reg_init_lpddr3_pd_en
 * pd enable signal fot lpddr3 initial
 */
#define DDRP_INIT_LPDDR3_PD_EN                     0x1c60, 31, 31

/* [30:30]  RW  reset 0x0  --  reg_lpddr3_preact_before_mrwreset
 * set this signal for preact before mrwreset during lpddr3 initial
 */
#define DDRP_LPDDR3_PREACT_BEFORE_MRWRESET         0x1c60, 30, 30

/* [29:28]  RW  reset 0x0  --  timer_lpddr3_pd_entry_select
 * timer scale choose
 * 2'b00: timer_x1. Unit dfi_1xclk
 * 2'b01: timer_x32. Unit dfi_1xclk
 * 2'b10: timer_x1024. Unit dfi_1xclk
 */
#define DDRP_TIMER_LPDDR3_PD_ENTRY_SELECT          0x1c60, 29, 28

/* [27:26]  RW  reset 0x0  --  timer_lpddr3_pd_exit_select
 * timer scale choose
 * 2'b00: timer_x1. Unit dfi_1xclk
 * 2'b01: timer_x32. Unit dfi_1xclk
 * 2'b10: timer_x1024. Unit dfi_1xclk
 */
#define DDRP_TIMER_LPDDR3_PD_EXIT_SELECT           0x1c60, 27, 26

/* [25:24]  RW  reset 0x0  --  timer_lpddr3_pd_wait_select
 * timer scale choose
 * 2'b00: timer_x1. Unit dfi_1xclk
 * 2'b01: timer_x32. Unit dfi_1xclk
 * 2'b10: timer_x1024. Unit dfi_1xclk
 */
#define DDRP_TIMER_LPDDR3_PD_WAIT_SELECT           0x1c60, 25, 24

/* [23:16]  RW  reset 0x00  --  reg_init_lpddr3_pd_exit
 * lpddr3 initial flow
 * when enter pd mode ,set this time for exit
 */
#define DDRP_INIT_LPDDR3_PD_EXIT                   0x1c60, 23, 16

/* [15:8]  RW  reset 0x00  --  reg_init_lpddr3_pd_wait
 * lpddr3 initial flow
 * when enter pd mode ,set this time for wait
 */
#define DDRP_INIT_LPDDR3_PD_WAIT                   0x1c60, 15,  8

/* [7:4]  RW  reset 0xa  --  reg_tcaext_lpddr3
 * used for lpddr3 ca train tcaext timing.  Unit dfi_1xclk
 */
#define DDRP_TCAEXT_LPDDR3                         0x1c60,  7,  4

/* [3:3]  RW  reset 0x1  --  reg_dfi_idle_en_phy
 * not use
 */
#define DDRP_DFI_IDLE_EN_PHY                       0x1c60,  3,  3

/* [2:2]  RW  reset 0x0  --  mrr_dai_phy
 * not use
 */
#define DDRP_MRR_DAI_PHY                           0x1c60,  2,  2

/* [1:1]  RW  reset 0x0  --  cat_skip_freq_change
 * Default 1'b0. Used to choose whether frequency reduction for lpddr3 ca train
 * 1'b0: During lpddr3 ca train, set load mode by reducing frequency
 * 1'b1: During lpddr3 ca train, set load mode through normal frequency
 */
#define DDRP_CAT_SKIP_FREQ_CHANGE                  0x1c60,  1,  1

/* [0:0]  RW  reset 0x0  --  cat_mrw_init_after_cbt
 * Default 1'b0,when set 1'b1 means lpddr3 initial mrw0/1/2 set after ca train done.
 */
#define DDRP_CAT_MRW_INIT_AFTER_CBT                0x1c60,  0,  0


/* ----- 0x1c64 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank0_min_a9_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A9 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A9_PERBIT_SKEW_PASS  0x1c64, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank0_min_a8_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A8 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A8_PERBIT_SKEW_PASS  0x1c64, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank0_min_a7_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A7 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A7_PERBIT_SKEW_PASS  0x1c64, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank0_min_a6_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A6 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A6_PERBIT_SKEW_PASS  0x1c64,  7,  0


/* ----- 0x1c68 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank0_min_a5_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A5 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A5_PERBIT_SKEW_PASS  0x1c68, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank0_min_a4_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A4 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A4_PERBIT_SKEW_PASS  0x1c68, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank0_min_a3_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A3 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A3_PERBIT_SKEW_PASS  0x1c68, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank0_min_a2_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A2 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A2_PERBIT_SKEW_PASS  0x1c68,  7,  0


/* ----- 0x1c6c ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank0_min_a1_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A1 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A1_PERBIT_SKEW_PASS  0x1c6c, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank0_min_a0_perbit_skew_pass
 * The min pass point of the tx delay line for rank0 A0 .
 */
#define DDRP_LPDDR3_RANK0_MIN_A0_PERBIT_SKEW_PASS  0x1c6c, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank0_max_a9_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A9 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A9_PERBIT_SKEW_PASS  0x1c6c, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank0_max_a8_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A8 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A8_PERBIT_SKEW_PASS  0x1c6c,  7,  0


/* ----- 0x1c70 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank0_max_a7_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A7 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A7_PERBIT_SKEW_PASS  0x1c70, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank0_max_a6_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A6 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A6_PERBIT_SKEW_PASS  0x1c70, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank0_max_a5_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A5 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A5_PERBIT_SKEW_PASS  0x1c70, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank0_max_a4_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A4 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A4_PERBIT_SKEW_PASS  0x1c70,  7,  0


/* ----- 0x1c74 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank0_max_a3_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A3 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A3_PERBIT_SKEW_PASS  0x1c74, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank0_max_a2_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A2 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A2_PERBIT_SKEW_PASS  0x1c74, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank0_max_a1_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A1 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A1_PERBIT_SKEW_PASS  0x1c74, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank0_max_a0_perbit_skew_pass
 * The max pass point of the tx delay line for rank0 A0 .
 */
#define DDRP_LPDDR3_RANK0_MAX_A0_PERBIT_SKEW_PASS  0x1c74,  7,  0


/* ----- 0x1c78 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank1_min_a9_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A9 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A9_PERBIT_SKEW_PASS  0x1c78, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank1_min_a8_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A8 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A8_PERBIT_SKEW_PASS  0x1c78, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank1_min_a7_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A7 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A7_PERBIT_SKEW_PASS  0x1c78, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank1_min_a6_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A6 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A6_PERBIT_SKEW_PASS  0x1c78,  7,  0


/* ----- 0x1c7c ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank1_min_a5_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A5 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A5_PERBIT_SKEW_PASS  0x1c7c, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank1_min_a4_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A4 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A4_PERBIT_SKEW_PASS  0x1c7c, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank1_min_a3_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A3 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A3_PERBIT_SKEW_PASS  0x1c7c, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank1_min_a2_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A2 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A2_PERBIT_SKEW_PASS  0x1c7c,  7,  0


/* ----- 0x1c80 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank1_min_a1_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A1 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A1_PERBIT_SKEW_PASS  0x1c80, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank1_min_a0_perbit_skew_pass
 * The min pass point of the tx delay line for rank1 A0 .
 */
#define DDRP_LPDDR3_RANK1_MIN_A0_PERBIT_SKEW_PASS  0x1c80, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank1_max_a9_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A9 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A9_PERBIT_SKEW_PASS  0x1c80, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank1_max_a8_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A8 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A8_PERBIT_SKEW_PASS  0x1c80,  7,  0


/* ----- 0x1c84 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank1_max_a7_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A7 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A7_PERBIT_SKEW_PASS  0x1c84, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank1_max_a6_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A6 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A6_PERBIT_SKEW_PASS  0x1c84, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank1_max_a5_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A5 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A5_PERBIT_SKEW_PASS  0x1c84, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank1_max_a4_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A4 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A4_PERBIT_SKEW_PASS  0x1c84,  7,  0


/* ----- 0x1c88 ----- */
/* [31:24]  RO  reset 0x0  --  lpddr3_rank1_max_a3_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A3 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A3_PERBIT_SKEW_PASS  0x1c88, 31, 24

/* [23:16]  RO  reset 0x0  --  lpddr3_rank1_max_a2_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A2 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A2_PERBIT_SKEW_PASS  0x1c88, 23, 16

/* [15:8]  RO  reset 0x0  --  lpddr3_rank1_max_a1_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A1 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A1_PERBIT_SKEW_PASS  0x1c88, 15,  8

/* [7:0]  RO  reset 0x0  --  lpddr3_rank1_max_a0_perbit_skew_pass
 * The max pass point of the tx delay line for rank1 A0 .
 */
#define DDRP_LPDDR3_RANK1_MAX_A0_PERBIT_SKEW_PASS  0x1c88,  7,  0


/* ----- 0x1c8c ----- */
/* [31:24]  RW  reset 0x1  --  reg_ddrc_trp_phy
 * Reserved.
 */
#define DDRP_DDRC_TRP_PHY                          0x1c8c, 31, 24

/* [23:16]  RW  reset 0x0  --  reg_ddrc_tmrr_phy
 * Reserved.
 */
#define DDRP_DDRC_TMRR_PHY                         0x1c8c, 23, 16

/* [15:8]  RW  reset 0x0  --  reg_ddrc_tmrd_phy
 * tMRD- Indicates the number of cycles to wait after a mode register write or read
 * tMRD = reg_drrc_tmrd *Tclk. For DDR4/DDR5
 */
#define DDRP_DDRC_TMRD_PHY                         0x1c8c, 15,  8

/* [7:0]  RW  reset 0x0  --  reg_ddrc_trpa_phy
 * Reserved.
 */
#define DDRP_DDRC_TRPA_PHY                         0x1c8c,  7,  0


/* ----- 0x1c90 ----- */
/* [31:16]  RW  reset 0xaab  --  reg_lpddr3_load_mode_mrwzqcl
 * lpddr3 load mode mrwzqcl value
 */
#define DDRP_LPDDR3_LOAD_MODE_MRWZQCL              0x1c90, 31, 16

/* [15:0]  RW  reset 0x3ffc  --  reg_lpddr3_load_mode_mrwreset
 * lpddr3 load mode mrwreset value
 */
#define DDRP_LPDDR3_LOAD_MODE_MRWRESET             0x1c90, 15,  0


/* ----- 0x1c94 ----- */
/* [31:24]  RW  reset 0x0  --  reg_lpddr3_tinit5
 * lpddr3 Tinit5. Unit dfi_1xclk x 32
 */
#define DDRP_LPDDR3_TINIT5                         0x1c94, 31, 24

/* [23:8]  RW  reset 0x0  --  reg_lpddr3_load_mode_read
 * lpddr3 mrr value
 */
#define DDRP_LPDDR3_LOAD_MODE_READ                 0x1c94, 23,  8

/* [7:0]  RW  reset 0x00  --  reg_init_lpddr3_pd_entry
 * lpddr3 initial flow
 * when enter pd mode ,set this time for entry
 */
#define DDRP_INIT_LPDDR3_PD_ENTRY                  0x1c94,  7,  0


/* ----- 0x1c98 ----- */
/* [31:24]  RW  reset 0x0  --  reg_mrwreset_1us_phy
 * Reserved.
 */
#define DDRP_MRWRESET_1US_PHY                      0x1c98, 31, 24

/* [23:16]  RW  reset 0x40  --  reg_tinit1_100ns_phy
 * Used to contol the timing parameter of tinit1 100ns for lpddr23. For DDR5 it means tINIT2
 */
#define DDRP_TINIT1_100NS_PHY                      0x1c98, 23, 16

/* [1:1]  RW  reset 0x0  --  reg_init_lpddr3_mrr_en
 * mrr enable signal for lpddr3 initial
 */
#define DDRP_INIT_LPDDR3_MRR_EN                    0x1c98,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_lpddr3_preall_before_mrwreset
 * set this signal for preall before mrwreset during lpddr3 initial
 */
#define DDRP_LPDDR3_PREALL_BEFORE_MRWRESET         0x1c98,  0,  0


/* ============================ Byte0 ============================ */


/* ----- 0x200 ----- */
/* [31:23]  RW  reset 0x80  --  reg_a_l_vref1_margsel_reg
 * Used to configure the Vref value of the PHY.
 */
#define DDRP_A_L_VREF1_MARGSEL_REG                 0x200, 31, 23

/* [22:22]  RW  reset 0x0  --  reg_a_l_dq_odt_zqcali_en
 * The DQ ODT control signal.
 * 1: ZQ calibration result will be written into analog circuit.
 * 0: Values in I/O drive strength registers register will be written into the analog circuit.
 * Refer to ZQ calibration section in the Databook
 */
#define DDRP_A_L_DQ_ODT_ZQCALI_EN                  0x200, 22, 22

/* [21:21]  RW  reset 0x0  --  reg_a_l_dq_drv_zqcali_en
 * The DQ driver control signal.
 * 1: ZQ calibration result will be written into analog circuit.
 * 0: Values in I/O drive strength registers register will be written into the analog circuit.
 * Refer to ZQ calibration section in the Databook
 */
#define DDRP_A_L_DQ_DRV_ZQCALI_EN                  0x200, 21, 21

/* [20:20]  RW  reset 0x0  --  reg_a_l_abutweakpddq_reg
 * Weak pull-down of A_DQ0~A_DQ7.
 * 0: Disable
 * 1: Enable, about 2.2kohm pull-down strength
 */
#define DDRP_A_L_ABUTWEAKPDDQ_REG                  0x200, 20, 20

/* [19:19]  RW  reset 0x1  --  reg_a_l_abutweakpubdq_reg
 * Weak pull-up of A_DQ0~A_DQ7.
 * 0: Enable, about 2.2kohm pull-up strength
 * 1: Disable
 */
#define DDRP_A_L_ABUTWEAKPUBDQ_REG                 0x200, 19, 19

/* [18:18]  RW  reset 0x0  --  reg_a_l_abutdiffampseen_reg
 * Reserved.
 */
#define DDRP_A_L_ABUTDIFFAMPSEEN_REG               0x200, 18, 18

/* [17:17]  RW  reset 0x0  --  reg_a_l_vref1_pd_reg
 * Byte0 internal power down signal.
 * Active State: High
 */
#define DDRP_A_L_VREF1_PD_REG                      0x200, 17, 17

/* [16:15]  RW  reset 0x0  --  reg_a_l_weakpd_reg
 * DQS pull-down and DQSB pull-up control.
 * 00: Disable
 * 01/10: About 400ohm
 * 11: About 200ohm
 */
#define DDRP_A_L_WEAKPD_REG                        0x200, 16, 15

/* [14:13]  RW  reset 0x3  --  reg_a_l_weakpub_reg
 * DQS pull-up and DQSB pull-down control.
 * 11: Disable
 * 01/10: About 400ohm
 * 00: About 200ohm
 */
#define DDRP_A_L_WEAKPUB_REG                       0x200, 14, 13

/* [12:8]  RW  reset 0x0  --  reg_a_l_abutslewpu_reg
 * Byte0 edge slew rate control , default 0 means maximum slew rate.
 */
#define DDRP_A_L_ABUTSLEWPU_REG                    0x200, 12,  8

/* [7:7]  RW  reset 0x1  --  reg_a_l_enb_lp4mode_reg
 * Reserved.
 */
#define DDRP_A_L_ENB_LP4MODE_REG                   0x200,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_a_l_dqsweakpd_reg
 * Weak pull-down of A_DQS0.
 * Active State: High
 * 0: Disable
 * 1: About 3.2kohm weak pull-down
 */
#define DDRP_A_L_DQSWEAKPD_REG                     0x200,  6,  6

/* [5:5]  RW  reset 0x1  --  reg_a_l_dqsbweakpub_reg
 * Weak pull-up of A_DQSB0.
 * Active State: Low
 * 0: About 3.2kohm weak pull-up
 * 1: Disable
 */
#define DDRP_A_L_DQSBWEAKPUB_REG                   0x200,  5,  5

/* [4:0]  RW  reset 0x0  --  reg_a_l_abutslewpd_reg
 * Reserved.
 */
#define DDRP_A_L_ABUTSLEWPD_REG                    0x200,  4,  0


/* ----- 0x204 ----- */
/* [30:30]  RW  reset 0x0  --  reg_a_l_dqfben_reg
 * The PHY BIST byte0 feedback enable signal.
 * Active State: High
 * Only for debug purpose.
 */
#define DDRP_A_L_DQFBEN_REG                        0x204, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_a_l_dqfbsel_reg
 * The select signal of PHY BIST byte0.
 * 1: Choose the internal Tx driver output as the end
 * 0: Choose the pad as the end
 */
#define DDRP_A_L_DQFBSEL_REG                       0x204, 29, 29

/* [28:24]  RW  reset 0xe  --  reg_a_l_abutnrcompdq_reg
 * The driver pull-down resistance of byte0.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_L_ABUTNRCOMPDQ_REG                  0x204, 28, 24

/* [20:16]  RW  reset 0xe  --  reg_a_l_abutprcompdq_reg
 * The driver pull-up resistance of byte0.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_L_ABUTPRCOMPDQ_REG                  0x204, 20, 16

/* [12:8]  RW  reset 0x5  --  reg_a_l_abutodtpddq_reg
 * The ODT pull-down resistance of byte0.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_L_ABUTODTPDDQ_REG                   0x204, 12,  8

/* [4:0]  RW  reset 0x5  --  reg_a_l_abutodtpudq_reg
 * The ODT pull-up resistance of byte0.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_L_ABUTODTPUDQ_REG                   0x204,  4,  0


/* ----- 0x208 ----- */
/* [26:24]  RW  reset 0x1  --  reg_a_l_rxmen0_delay_bp
 * Control the 1x delay of the Rx DQS calibration delay for byte0 of RANK0 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_L_RXMEN0_DELAY_BP                   0x208, 26, 24

/* [23:21]  RW  reset 0x7  --  reg_a_l_rxmen0_ophsel_bp
 * Control the 0.5*4x delay of the Rx DQS calibration delay for byte0 of RANK0 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_L_RXMEN0_OPHSEL_BP                  0x208, 23, 21

/* [20:16]  RW  reset 0x4  --  reg_a_l_rxmen0_sdlltap_bp
 * Control the delay line of the Rx DQS calibration delay for byte0 of RANK0 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_L_RXMEN0_SDLLTAP_BP                 0x208, 20, 16

/* [10:8]  RW  reset 0x1  --  reg_a_l_rxmen1_delay_bp
 * Control the 1x delay of the Rx DQS calibration delay  for byte0 of RANK1 when enabling  the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_L_RXMEN1_DELAY_BP                   0x208, 10,  8

/* [7:5]  RW  reset 0x7  --  reg_a_l_rxmen1_ophsel_bp
 * Control the 0.5*4x delay of the Rx DQS calibration delay for byte0 of RANK1 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_L_RXMEN1_OPHSEL_BP                  0x208,  7,  5

/* [4:0]  RW  reset 0x4  --  reg_a_l_rxmen1_sdlltap_bp
 * Control the delay line of the Rx DQS calibration delay for byte0 of RANK1 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_L_RXMEN1_SDLLTAP_BP                 0x208,  4,  0


/* ----- 0x20c ----- */
/* [26:24]  RW  reset 0x2  --  reg_a_l_rdodt0_delay
 * Control the 0.5*4x delay of the Rx ODT delay for byte0 of RANK0 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_L_RDODT0_DELAY                      0x20c, 26, 24

/* [23:21]  RW  reset 0x2  --  reg_a_l_rdodt0_ophsel
 * Control the delay line of the Rx ODT delay for byte0 of RANK0 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_L_RDODT0_OPHSEL                     0x20c, 23, 21

/* [20:16]  RW  reset 0x4  --  reg_a_l_rdodt0_dllsel
 * Control the 1x delay of the Rx ODT delay for byte0 of RANK0 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_L_RDODT0_DLLSEL                     0x20c, 20, 16

/* [10:8]  RW  reset 0x2  --  reg_a_l_rdodt1_delay
 * Control the 0.5*4x delay of the Rx ODT delay for byte0 of RANK1 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_L_RDODT1_DELAY                      0x20c, 10,  8

/* [7:5]  RW  reset 0x2  --  reg_a_l_rdodt1_ophsel
 * Control the delay line of the Rx ODT delay for byte0 of RANK1 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_L_RDODT1_OPHSEL                     0x20c,  7,  5

/* [4:0]  RW  reset 0x4  --  reg_a_l_rdodt1_dllsel
 * Control the 1x delay of the Rx ODT delay  for byte0 of RANK1 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_L_RDODT1_DLLSEL                     0x20c,  4,  0


/* ----- 0x210 ----- */
/* [29:29]  RW  reset 0x0  --  reg_a_l_dq_invdelay_lp_en
 * Reserved.
 */
#define DDRP_A_L_DQ_INVDELAY_LP_EN                 0x210, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_a_l_dm_obsdataen
 * Reserved.
 */
#define DDRP_A_L_DM_OBSDATAEN                      0x210, 28, 28

/* [27:24]  RW  reset 0x0  --  reg_a_l_dqobsmuxsel
 * Reserved.
 */
#define DDRP_A_L_DQOBSMUXSEL                       0x210, 27, 24

/* [23:23]  RW  reset 0x0  --  reg_a_l_rxen_lp4
 * Enable the Rx receiver of the LPDDR4 mode for byte0.
 * Active State: High
 */
#define DDRP_A_L_RXEN_LP4                          0x210, 23, 23

/* [22:22]  RW  reset 0x0  --  reg_a_l_lp4x_en
 * Enable the Rx receiver of the LPDDR4X mode for byte0.
 * Active State: High
 */
#define DDRP_A_L_LP4X_EN                           0x210, 22, 22

/* [21:21]  RW  reset 0x1  --  reg_a_l_pvt_comp_en
 * Enable the PVT compensation update function for byte0.
 * Active State: High
 */
#define DDRP_A_L_PVT_COMP_EN                       0x210, 21, 21

/* [20:20]  RW  reset 0x1  --  reg_a_l_dqout_mux
 * Used to control the timing of the DQ for byte0 from the digital part to the analog part.
 * 1: Align to the posedge of the dfi_clk1x
 * 0: Align to the negedge of the dfi_clk1x
 */
#define DDRP_A_L_DQOUT_MUX                         0x210, 20, 20

/* [19:19]  RW  reset 0x1  --  reg_a_l_dmout_mux
 * Used to control the timing of the DM for byte0 from the digital part to the analog part.
 * 1: Align to the posedge of the dfi_clk1x
 * 0: Align to the negedge of the dfi_clk1x
 */
#define DDRP_A_L_DMOUT_MUX                         0x210, 19, 19

/* [18:18]  RW  reset 0x0  --  reg_a_l_dq_ph90en_bp
 * Used to control the 90 degree of the DQ/DM for byte0 when the ph90en_bp_dqis set to 1'b1.
 * 1: Enable the 90 degree mode
 * 0: Disable the 90 degree mode
 */
#define DDRP_A_L_DQ_PH90EN_BP                      0x210, 18, 18

/* [17:17]  RW  reset 0x1  --  reg_a_l_dqs_ph90en_bp
 * Used to control the 90 degree of the DQS for byte0 when the ph90en_bp_dq is set to 1'b1.
 * 1: Enable the 90 degree mode
 * 0: Disable the 90 degree mode
 */
#define DDRP_A_L_DQS_PH90EN_BP                     0x210, 17, 17

/* [16:16]  RW  reset 0x1  --  reg_a_l_rcvdqsmodsel
 * Reserved.
 */
#define DDRP_A_L_RCVDQSMODSEL                      0x210, 16, 16

/* [15:15]  RW  reset 0x1  --  reg_a_l_selfclren
 * Reserved.
 */
#define DDRP_A_L_SELFCLREN                         0x210, 15, 15

/* [14:14]  RW  reset 0x1  --  reg_a_l_wrptrclrb
 * Reserved.
 */
#define DDRP_A_L_WRPTRCLRB                         0x210, 14, 14

/* [13:13]  RW  reset 0x1  --  reg_a_l_rxm_odiffampen
 * Reserved.
 */
#define DDRP_A_L_RXM_ODIFFAMPEN                    0x210, 13, 13

/* [12:12]  RW  reset 0x0  --  reg_a_l_abutobsmodeen
 * Reserved.
 */
#define DDRP_A_L_ABUTOBSMODEEN                     0x210, 12, 12

/* [10:10]  RW  reset 0x0  --  reg_a_l_rxm4p5en
 * The enable signal to extend the gating signal of the Rx DQS calibration for byte0 of the RANK0.
 * 1: Increase 0.5UI to the gating signal
 * 0: Keep the default state
 */
#define DDRP_A_L_RXM4P5EN                          0x210, 10, 10

/* [9:9]  RW  reset 0x1  --  reg_a_l_tsm_iobufact_bp
 * Reserved.
 */
#define DDRP_A_L_TSM_IOBUFACT_BP                   0x210,  9,  9

/* [8:8]  RW  reset 0x0  --  reg_a_l_rxpst_bp
 * Reserved.
 */
#define DDRP_A_L_RXPST_BP                          0x210,  8,  8

/* [6:6]  RW  reset 0x0  --  reg_a_l_rxm4p5en_r2
 * The enable signal to extend the gating signal of the Rx DQS calibration for byte0 of the RANK1.
 * 1: Increase 0.5UI to the gating signal
 * 0: Keep the default state
 */
#define DDRP_A_L_RXM4P5EN_R2                       0x210,  6,  6

/* [5:3]  RW  reset 0x0  --  reg_a_l_rrankdly_4x_cs0
 * Used to control the delay of the read rank switch signal of the byte0 for RANK0.
 * Unit= 0.5UI.
 */
#define DDRP_A_L_RRANKDLY_4X_CS0                   0x210,  5,  3

/* [2:0]  RW  reset 0x0  --  reg_a_l_rrankdly_4x_cs1
 * Used to control the delay of the read rank switch signal of the byte0 for RANK1.
 * Unit= 0.5UI.
 */
#define DDRP_A_L_RRANKDLY_4X_CS1                   0x210,  2,  0


/* ----- 0x214 ----- */
/* [18:16]  RW  reset 0x0  --  reg_a_l_rrankdly_1x_cs1
 * Used to control the delay of the read rank switch signal of the byte0 for RANK1.
 * Unit= 4UI.
 */
#define DDRP_A_L_RRANKDLY_1X_CS1                   0x214, 18, 16

/* [14:12]  RW  reset 0x0  --  reg_a_l_rrankdly_1x_cs0
 * Used to control the delay of the read rank switch signal of the byte0 for RANK0.
 * Unit= 4UI.
 */
#define DDRP_A_L_RRANKDLY_1X_CS0                   0x214, 14, 12

/* [10:8]  RW  reset 0x0  --  reg_a_l_wrankphsel
 * Used to control the 0.5*4x delay of the write rank switch signal of the byte0 for RANK0.
 * Unit = 0.5*UI.
 */
#define DDRP_A_L_WRANKPHSEL                        0x214, 10,  8

/* [7:0]  RW  reset 0x0  --  reg_a_l_wrankdlysel
 * Used to control the delay of the write rank switch signal of the byte0 for RANK0.
 * Unit= 0.5UI.
 */
#define DDRP_A_L_WRANKDLYSEL                       0x214,  7,  0


/* ----- 0x218 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_l_cs0_dm_invdelaysel
 * Used to control the Tx delay line of the A_DM0 for RANK0.
 */
#define DDRP_A_L_CS0_DM_INVDELAYSEL                0x218, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_l_cs0_dq0_invdelaysel
 * Used to control the Tx delay line of the A_DQ0 for RANK0.
 */
#define DDRP_A_L_CS0_DQ0_INVDELAYSEL               0x218, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_l_cs0_dq1_invdelaysel
 * Used to control the Tx delay line of the A_DQ1 for RANK0.
 */
#define DDRP_A_L_CS0_DQ1_INVDELAYSEL               0x218,  8,  0


/* ----- 0x21c ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_l_cs0_dq2_invdelaysel
 * Used to control the Tx delay line of the A_DQ2 for RANK0.
 */
#define DDRP_A_L_CS0_DQ2_INVDELAYSEL               0x21c, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_l_cs0_dq3_invdelaysel
 * Used to control the Tx delay line of the A_DQ3 for RANK0.
 */
#define DDRP_A_L_CS0_DQ3_INVDELAYSEL               0x21c, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_l_cs0_dq4_invdelaysel
 * Used to control the Tx delay line of the A_DQ4 for RANK0.
 */
#define DDRP_A_L_CS0_DQ4_INVDELAYSEL               0x21c,  8,  0


/* ----- 0x220 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_l_cs0_dq5_invdelaysel
 * Used to control the Tx delay line of the A_DQ5 for RANK0.
 */
#define DDRP_A_L_CS0_DQ5_INVDELAYSEL               0x220, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_l_cs0_dq6_invdelaysel
 * Used to control the Tx delay line of the A_DQ6 for RANK0.
 */
#define DDRP_A_L_CS0_DQ6_INVDELAYSEL               0x220, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_l_cs0_dq7_invdelaysel
 * Used to control the Tx delay line of the A_DQ7 for RANK0.
 */
#define DDRP_A_L_CS0_DQ7_INVDELAYSEL               0x220,  8,  0


/* ----- 0x224 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a_l_cs0_dqs_invdelaysel
 * Used to control the Tx delay line of the A_DQS0 for RANK0.
 */
#define DDRP_A_L_CS0_DQS_INVDELAYSEL               0x224, 31, 24

/* [15:8]  RW  reset 0x80  --  reg_a_l_cs0_dqsb_invdelaysel
 * Used to control the Tx delay line of the A_DQSB0 for RANK0.
 */
#define DDRP_A_L_CS0_DQSB_INVDELAYSEL              0x224, 15,  8


/* ----- 0x228 ----- */
/* [28:24]  RW  reset 0x0  --  reg_a_l_cs0_loop_invdelaysel
 * Choose the observation signal to check delay line control of the Byte0 for RANK0.
 * Set this register to the following values for the corresponding TX delay line:
 * 5'd0: DQ0/A_DQ0
 * 5'd1: DQ1/A_DQ1
 * …
 * 5'd7: DQ7/A_DQ7
 * 5'd8: DM0/A_DM0
 * 5'd9: DQS0/A_DQS0
 * 5'd10: DQSB0/A_DQSB0
 * Set this register to the following values for the corresponding RX delay line:
 * 5'd16: DQ0/A_DQ0
 * 5'd17: DQ1/A_DQ1
 * …
 * 5'd23: DQ7/A_DQ7
 * 5'd24: DM0/A_DM0
 * 5'd25: DQS0/A_DQS0
 * 5'd26: DQSB0/A_DQSB0
 */
#define DDRP_A_L_CS0_LOOP_INVDELAYSEL              0x228, 28, 24

/* [14:8]  RW  reset 0x7  --  reg_a_l_cs0_dm_invdelayselrx
 * Used to control the Rx delay line of the A_DM0 for RANK0.
 */
#define DDRP_A_L_CS0_DM_INVDELAYSELRX              0x228, 14,  8


/* ----- 0x22c ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_l_cs0_dq0_invdelayselrx
 * Used to control the Rx delay line of the A_DQ0 for RANK0.
 */
#define DDRP_A_L_CS0_DQ0_INVDELAYSELRX             0x22c, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_l_cs0_dq1_invdelayselrx
 * Used to control the Rx delay line of the A_DQ1 for RANK0.
 */
#define DDRP_A_L_CS0_DQ1_INVDELAYSELRX             0x22c, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_l_cs0_dq2_invdelayselrx
 * Used to control the Rx delay line of the A_DQ2 for RANK0.
 */
#define DDRP_A_L_CS0_DQ2_INVDELAYSELRX             0x22c, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_l_cs0_dq3_invdelayselrx
 * Used to control the Rx delay line of the A_DQ3 for RANK0.
 */
#define DDRP_A_L_CS0_DQ3_INVDELAYSELRX             0x22c,  6,  0


/* ----- 0x230 ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_l_cs0_dq4_invdelayselrx
 * Used to control the Rx delay line of the A_DQ4 for RANK0.
 */
#define DDRP_A_L_CS0_DQ4_INVDELAYSELRX             0x230, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_l_cs0_dq5_invdelayselrx
 * Used to control the Rx delay line of the A_DQ5 for RANK0.
 */
#define DDRP_A_L_CS0_DQ5_INVDELAYSELRX             0x230, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_l_cs0_dq6_invdelayselrx
 * Used to control the Rx delay line of the A_DQ6 for RANK0.
 */
#define DDRP_A_L_CS0_DQ6_INVDELAYSELRX             0x230, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_l_cs0_dq7_invdelayselrx
 * Used to control the Rx delay line of the A_DQ7 for RANK0.
 */
#define DDRP_A_L_CS0_DQ7_INVDELAYSELRX             0x230,  6,  0


/* ----- 0x234 ----- */
/* [30:24]  RW  reset 0x1f  --  reg_a_l_cs0_dqs_invdelayselrx
 * Used to control the Rx delay line of the A_DQS0 for RANK0.
 */
#define DDRP_A_L_CS0_DQS_INVDELAYSELRX             0x234, 30, 24

/* [14:8]  RW  reset 0x1f  --  reg_a_l_cs0_dqsb_invdelayselrx
 * Used to control the Rx delay line of the A_DQSB0 for RANK0.
 */
#define DDRP_A_L_CS0_DQSB_INVDELAYSELRX            0x234, 14,  8


/* ----- 0x238 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_l_cs1_dm_invdelaysel
 * Used to control the Tx delay line of the A_DM0 for RANK1.
 */
#define DDRP_A_L_CS1_DM_INVDELAYSEL                0x238, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_l_cs1_dq0_invdelaysel
 * Used to control the Tx delay line of the A_DQ0 for RANK1.
 */
#define DDRP_A_L_CS1_DQ0_INVDELAYSEL               0x238, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_l_cs1_dq1_invdelaysel
 * Used to control the Tx delay line of the A_DQ1 for RANK1.
 */
#define DDRP_A_L_CS1_DQ1_INVDELAYSEL               0x238,  8,  0


/* ----- 0x23c ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_l_cs1_dq2_invdelaysel
 * Used to control the Tx delay line of the A_DQ2 for RANK1.
 */
#define DDRP_A_L_CS1_DQ2_INVDELAYSEL               0x23c, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_l_cs1_dq3_invdelaysel
 * Used to control the Tx delay line of the A_DQ3 for RANK1.
 */
#define DDRP_A_L_CS1_DQ3_INVDELAYSEL               0x23c, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_l_cs1_dq4_invdelaysel
 * Used to control the Tx delay line of the A_DQ4 for RANK1.
 */
#define DDRP_A_L_CS1_DQ4_INVDELAYSEL               0x23c,  8,  0


/* ----- 0x240 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_l_cs1_dq5_invdelaysel
 * Used to control the Tx delay line of the A_DQ5 for RANK1.
 */
#define DDRP_A_L_CS1_DQ5_INVDELAYSEL               0x240, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_l_cs1_dq6_invdelaysel
 * Used to control the Tx delay line of the A_DQ6 for RANK1.
 */
#define DDRP_A_L_CS1_DQ6_INVDELAYSEL               0x240, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_l_cs1_dq7_invdelaysel
 * Used to control the Tx delay line of the A_DQ7 for RANK1.
 */
#define DDRP_A_L_CS1_DQ7_INVDELAYSEL               0x240,  8,  0


/* ----- 0x244 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a_l_cs1_dqs_invdelaysel
 * Used to control the Tx delay line of the A_DQS0 for RANK1.
 */
#define DDRP_A_L_CS1_DQS_INVDELAYSEL               0x244, 31, 24

/* [15:8]  RW  reset 0x80  --  reg_a_l_cs1_dqsb_invdelaysel
 * Used to control the Tx delay line of the A_DQSB0 for RANK1.
 */
#define DDRP_A_L_CS1_DQSB_INVDELAYSEL              0x244, 15,  8


/* ----- 0x248 ----- */
/* [28:24]  RW  reset 0x0  --  reg_a_l_cs1_loop_invdelaysel
 * Choose the observation signal to check delay line control of the Byte0 for RANK1.
 * Set this register to the following values for the corresponding Rx delay line:
 * 5'd0: DQ0/A_DQ0
 * 5'd1: DQ1/A_DQ1
 * …
 * 5'd7: DQ7/A_DQ7
 * 5'd8: DM0/A_DM0
 * 5'd9: DQS0/A_DQS0
 * 5'd10: DQSB0/A_DQSB0
 * Set this register to the following values for the corresponding Tx delay line:
 * 5'd16: DQ0/A_DQ0
 * 5'd17: DQ1/A_DQ1
 * …
 * 5'd23: DQ7/A_DQ7
 * 5'd24: DM0/A_DM0
 * 5'd25: DQS0/A_DQS0
 * 5'd26: DQSB0/A_DQSB0
 */
#define DDRP_A_L_CS1_LOOP_INVDELAYSEL              0x248, 28, 24

/* [14:8]  RW  reset 0x7  --  reg_a_l_cs1_dm_invdelayselrx
 * Used to control the Rx delay line of the A_DM0 for RANK1.
 */
#define DDRP_A_L_CS1_DM_INVDELAYSELRX              0x248, 14,  8


/* ----- 0x24c ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_l_cs1_dq0_invdelayselrx
 * Used to control the Rx delay line of the A_DQ0 for RANK1.
 */
#define DDRP_A_L_CS1_DQ0_INVDELAYSELRX             0x24c, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_l_cs1_dq1_invdelayselrx
 * Used to control the Rx delay line of the A_DQ1 for RANK1.
 */
#define DDRP_A_L_CS1_DQ1_INVDELAYSELRX             0x24c, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_l_cs1_dq2_invdelayselrx
 * Used to control the Rx delay line of the A_DQ2 for RANK1.
 */
#define DDRP_A_L_CS1_DQ2_INVDELAYSELRX             0x24c, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_l_cs1_dq3_invdelayselrx
 * Used to control the Rx delay line of the A_DQ3 for RANK1.
 */
#define DDRP_A_L_CS1_DQ3_INVDELAYSELRX             0x24c,  6,  0


/* ----- 0x250 ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_l_cs1_dq4_invdelayselrx
 * Used to control the Rx delay line of the A_DQ4 for RANK1.
 */
#define DDRP_A_L_CS1_DQ4_INVDELAYSELRX             0x250, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_l_cs1_dq5_invdelayselrx
 * Used to control the Rx delay line of the A_DQ5 for RANK1.
 */
#define DDRP_A_L_CS1_DQ5_INVDELAYSELRX             0x250, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_l_cs1_dq6_invdelayselrx
 * Used to control the Rx delay line of the A_DQ6 for RANK1.
 */
#define DDRP_A_L_CS1_DQ6_INVDELAYSELRX             0x250, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_l_cs1_dq7_invdelayselrx
 * Used to control the Rx delay line of the A_DQ7 for RANK1.
 */
#define DDRP_A_L_CS1_DQ7_INVDELAYSELRX             0x250,  6,  0


/* ----- 0x254 ----- */
/* [30:24]  RW  reset 0x1f  --  reg_a_l_cs1_dqs_invdelayselrx
 * Used to control the Rx delay line of the A_DQS0 for RANK1.
 */
#define DDRP_A_L_CS1_DQS_INVDELAYSELRX             0x254, 30, 24

/* [14:8]  RW  reset 0x1f  --  reg_a_l_cs1_dqsb_invdelayselrx
 * Used to control the Rx delay line of the A_DQSB0 for RANK1.
 */
#define DDRP_A_L_CS1_DQSB_INVDELAYSELRX            0x254, 14,  8


/* ----- 0x258 ----- */
/* [30:24]  RW  reset 0x1f  --  reg_a_l_rd_train_dqs_default
 * Used to control the start point of the Rx delay line of the DQS when read training for byte0. The read training will start based on this value.
 */
#define DDRP_A_L_RD_TRAIN_DQS_DEFAULT              0x258, 30, 24

/* [23:16]  RW  reset 0x7  --  reg_a_l_train_dqs_default
 * In the default mode, the write training will use the write training result to control the Tx delay line of the DQS and keep unchanged. The user also can set the reg_wr_train_dqs_default_bypass to 1'b1 to use this register to control the Tx delay line of the DQS for write training for byte0.
 */
#define DDRP_A_L_TRAIN_DQS_DEFAULT                 0x258, 23, 16

/* [14:8]  RW  reset 0x3f  --  reg_a_l_rd_train_dqs_range_max
 * Used to set the max scan range of the Rx delay line of the DQS when read training for byte0. The PHY will stop scanning when it increases the Rx delay line to this value.
 */
#define DDRP_A_L_RD_TRAIN_DQS_RANGE_MAX            0x258, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_l_rd_train_dqs_range_min
 * Used to set the min scan range of the Rx delay line of the DQS when read training for byte0. The PHY will stop scanning when it decreases the Rx delay line to this value.
 */
#define DDRP_A_L_RD_TRAIN_DQS_RANGE_MIN            0x258,  6,  0


/* ----- 0x25c ----- */
/* [31:24]  RW  reset 0x0  --  reg_a_l_rdtrain_check_wrap0
 * Used to control the read check pattern after the remap in byte0. Valid only when the reg_rd_train_check_value_en is set to 1'b1.
 * For the read training of LPDDR4, the even DQ pads such as the DQ0/DQ2/DQ4/DQ6 have different check patterns with the ones of the odd DQS pads such as the DQ1/DQ3/DQ5/DQ7. After the remap in one byte, the DQ0 of the PHY may connect the DQ1 of the SDRAM. The read back data from the odd DQ pads will be sent to the even DQ pads to check. So the PHY needs to know the remap relation between the PHY and SDRAM.
 * For LPDDR4 mode, if the DQ pad of the PHY connects to the even pad of the SDRAM, the corresponding bit should be set to 1'b1, otherwise it will be set to 1'b0.
 * eg. If the DQ6 of the SDRAM connects to the DQ1 of the PHY. The bit[1] should be set to 1'b1. If the DQ7 of the SDRAM connects to the DQ2 of the PHY. The bit[7] should be set to 1'b0.
 * For the read training of DDR4, the DQ0/DQ1/DQ2/DQ3 have different check pattern and DQ4/DQ5/DQ6/DQ7 repeat the check pattern of the DQ0/DQ1/DQ2/DQ3.  After the remapping in byte0, the user needs to choose the check pattern according to the connection between the PHY and the SDRAM.
 * If the DQ0/DQ4 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ1/DQ5 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b01.
 * If the DQ2/DQ6 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ3/DQ7 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b01.
 * [1:0]: Used to control the check pattern of the DQ0 for byte0.
 * [3:2]: Used to control the check pattern of the DQ1 for byte0.
 * [5:4]: Used to control the check pattern of the DQ2 for byte0.
 * [7:6]: Used to control the check pattern of the DQ3 for byte0.
 * User can find more information in read training section
 */
#define DDRP_A_L_RDTRAIN_CHECK_WRAP0               0x25c, 31, 24

/* [23:16]  RW  reset 0x0  --  reg_a_l_rdtrain_check_wrap1
 * || For the read training of DDR4, the DQ0/DQ1/DQ2/DQ3 have different check pattern and DQ4/DQ5/DQ6/DQ7 repeat the check pattern of the DQ0/DQ1/DQ2/DQ3.  After the remap in byte0, the user needs to choose the check pattern according to the connection between the PHY and the SDRAM.
 * If the DQ0/DQ4 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ1/DQ5 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b01.
 * If the DQ2/DQ6 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ3/DQ7 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b01.
 * [1:0]: Used to control the check pattern of the DQ4 for byte0.
 * [3:2]: Used to control the check pattern of the DQ5 for byte0.
 * [5:4]: Used to control the check pattern of the DQ6 for byte0.
 * [7:6]: Used to control the check pattern of the DQ7 for byte0.
 * User can find more information in read training section.
 */
#define DDRP_A_L_RDTRAIN_CHECK_WRAP1               0x25c, 23, 16

/* [15:14]  RW  reset 0x0  --  reg_a_l_cat_wrap_sel
 * Used to control the wrap between the bytes for LPDDR4 CAT.
 */
#define DDRP_A_L_CAT_WRAP_SEL                      0x25c, 15, 14

/* [13:10]  RW  reset 0x8  --  reg_a_l_dm_bit_wrap_sel
 * Used to control the wrap of DM between the bits in the same byte.
 */
#define DDRP_A_L_DM_BIT_WRAP_SEL                   0x25c, 13, 10


/* ----- 0x260 ----- */
/* [31:28]  RW  reset 0x7  --  reg_a_l_dq7_bit_wrap_sel
 * Used to control the wrap of DQ7 between the bits in the same byte.
 */
#define DDRP_A_L_DQ7_BIT_WRAP_SEL                  0x260, 31, 28

/* [27:24]  RW  reset 0x6  --  reg_a_l_dq6_bit_wrap_sel
 * Used to control the wrap of DQ6 between the bits in the same byte.
 */
#define DDRP_A_L_DQ6_BIT_WRAP_SEL                  0x260, 27, 24

/* [23:20]  RW  reset 0x5  --  reg_a_l_dq5_bit_wrap_sel
 * Used to control the wrap of DQ5 between the bits in the same byte.
 */
#define DDRP_A_L_DQ5_BIT_WRAP_SEL                  0x260, 23, 20

/* [19:16]  RW  reset 0x4  --  reg_a_l_dq4_bit_wrap_sel
 * Used to control the wrap of DQ4 between the bits in the same byte.
 */
#define DDRP_A_L_DQ4_BIT_WRAP_SEL                  0x260, 19, 16

/* [15:12]  RW  reset 0x3  --  reg_a_l_dq3_bit_wrap_sel
 * Used to control the wrap of DQ3 between the bits in the same byte.
 */
#define DDRP_A_L_DQ3_BIT_WRAP_SEL                  0x260, 15, 12

/* [11:8]  RW  reset 0x2  --  reg_a_l_dq2_bit_wrap_sel
 * Used to control the wrap of DQ2 between the bits in the same byte.
 */
#define DDRP_A_L_DQ2_BIT_WRAP_SEL                  0x260, 11,  8

/* [7:4]  RW  reset 0x1  --  reg_a_l_dq1_bit_wrap_sel
 * Used to control the wrap of DQ1 between the bits in the same byte.
 */
#define DDRP_A_L_DQ1_BIT_WRAP_SEL                  0x260,  7,  4

/* [3:0]  RW  reset 0x0  --  reg_a_l_dq0_bit_wrap_sel
 * Used to control the wrap of DQ0 between the bits in the same byte.
 */
#define DDRP_A_L_DQ0_BIT_WRAP_SEL                  0x260,  3,  0


/* ----- 0x280 ----- */
/* [30:30]  RO  reset 0x0  --  reg_a_l_dqs_idqshigh
 * Reserved.
 */
#define DDRP_A_L_DQS_IDQSHIGH                      0x280, 30, 30

/* [23:16]  RO  reset 0x80  --  reg_a_l_tdqs_invdelaysel0
 * The write-leveling result of byte0 for RANK0. The user can get the write-leveling result after the write-leveling is complete.
 */
#define DDRP_A_L_TDQS_INVDELAYSEL0                 0x280, 23, 16

/* [7:0]  RO  reset 0x80  --  reg_a_l_tdqs_invdelaysel1
 * The write-leveling result of byte0 for RANK1. The user can get the write-leveling result after the write-leveling is complete.
 */
#define DDRP_A_L_TDQS_INVDELAYSEL1                 0x280,  7,  0


/* ----- 0x284 ----- */
/* [18:16]  RO  reset 0x1  --  reg_a_l_cycsel
 * The current 1x delay of the Rx DQS calibration of byte0 for RANK0. Use this register to get the current working state of the Rx DQS calibration.
 */
#define DDRP_A_L_CYCSEL                            0x284, 18, 16

/* [10:8]  RO  reset 0x0  --  reg_a_l_ophsel
 * The current 0.5UI delay of the Rx DQS calibration of byte0 for RANK0. Use this register to get the current working state of the Rx DQS calibration.
 */
#define DDRP_A_L_OPHSEL                            0x284, 10,  8

/* [4:0]  RO  reset 0x2  --  reg_a_l_dllsel
 * The current 1/64UI Tx delay of the Rx DQS calibration of byte0 for RANK0. Use this register to get the current working state of the Rx DQS calibration.
 */
#define DDRP_A_L_DLLSEL                            0x284,  4,  0


/* ----- 0x288 ----- */
/* [26:16]  RO  reset 0x0  --  reg_a_l_calib_result_cs0
 * The training result of the Rx DQS calibration of byte0 for RANK0. The user can use this register to get the current working state of the Rx DQS calibration.
 * [20:16]: The 1/64UI Tx delay line value
 * [23:21]: The 0.5UI delay value
 * [26:24]: The 1x delay value
 */
#define DDRP_A_L_CALIB_RESULT_CS0                  0x288, 26, 16

/* [10:0]  RO  reset 0x0  --  reg_a_l_calib_result_cs1
 * The training result of the Rx DQS calibration of byte0 for RANK1. The user can use this register to get the current working state of the Rx DQS calibration.
 * [4:0]: The 1/64UI Tx delay line value
 * [7:5]: The 0.5UI delay value
 * [10:8]: The 1x delay value
 */
#define DDRP_A_L_CALIB_RESULT_CS1                  0x288, 10,  0


/* ----- 0x28c ----- */
/* [8:0]  RO  reset 0x0  --  reg_a_l_cs0_value_dqx_invdelaysel
 * The delay line value of the current observation signal choosing of reg_a_l_cs0_loop_invdelaysel.
 */
#define DDRP_A_L_CS0_VALUE_DQX_INVDELAYSEL         0x28c,  8,  0


/* ----- 0x290 ----- */
/* [8:0]  RO  reset 0x0  --  reg_a_l_cs1_value_dqx_invdelaysel
 * The delay line value of the current observation signal choosing of reg_a_l_cs1_loop_invdelaysel.
 */
#define DDRP_A_L_CS1_VALUE_DQX_INVDELAYSEL         0x290,  8,  0


/* ----- 0x294 ----- */
/* [30:24]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq0
 * The min pass point of the Rx delay line for A_DQ0.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ0              0x294, 30, 24

/* [22:16]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq1
 * The min pass point of the Rx delay line for A_DQ1.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ1              0x294, 22, 16

/* [14:8]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq2
 * The min pass point of the Rx delay line for A_DQ2.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ2              0x294, 14,  8

/* [6:0]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq3
 * The min pass point of the Rx delay line for A_DQ3.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ3              0x294,  6,  0


/* ----- 0x298 ----- */
/* [30:24]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq4
 * The min pass point of the Rx delay line for A_DQ4.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ4              0x298, 30, 24

/* [22:16]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq5
 * The min pass point of the Rx delay line for A_DQ5.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ5              0x298, 22, 16

/* [14:8]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq6
 * The min pass point of the Rx delay line for A_DQ6.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ6              0x298, 14,  8

/* [6:0]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dq7
 * The min pass point of the Rx delay line for A_DQ7.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQ7              0x298,  6,  0


/* ----- 0x29c ----- */
/* [14:8]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dqs
 * The min pass point of the Rx delay line for A_DQS0/A_DQSB0.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DQS              0x29c, 14,  8


/* ----- 0x2a0 ----- */
/* [30:24]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq0
 * The max pass point of the Rx delay line for A_DQ0.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ0              0x2a0, 30, 24

/* [22:16]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq1
 * The max pass point of the Rx delay line for A_DQ1.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ1              0x2a0, 22, 16

/* [14:8]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq2
 * The max pass point of the Rx delay line for A_DQ2.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ2              0x2a0, 14,  8

/* [6:0]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq3
 * The max pass point of the Rx delay line for A_DQ3.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ3              0x2a0,  6,  0


/* ----- 0x2a4 ----- */
/* [30:24]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq4
 * The max pass point of the Rx delay line for A_DQ4.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ4              0x2a4, 30, 24

/* [22:16]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq5
 * The max pass point of the Rx delay line for A_DQ5.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ5              0x2a4, 22, 16

/* [14:8]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq6
 * The max pass point of the Rx delay line for A_DQ6.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ6              0x2a4, 14,  8

/* [6:0]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dq7
 * The max pass point of the Rx delay line for A_DQ7.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQ7              0x2a4,  6,  0


/* ----- 0x2a8 ----- */
/* [14:8]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dqs
 * The max pass point of the Rx delay line for A_DQS0/A_DQSB0.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DQS              0x2a8, 14,  8


/* ----- 0x2ac ----- */
/* [30:24]  RO  reset 0x0  --  reg_a_l_train_result_for_rd_base_dqs
 * The best point of the Rx delay line for A_DQS0/A_DQSB0 after the read training.
 */
#define DDRP_A_L_TRAIN_RESULT_FOR_RD_BASE_DQS      0x2ac, 30, 24

/* [5:5]  RO  reset 0x0  --  reg_a_l_change_rd_dqs_default
 * The flag that can't find the pass window of the read training for byte0.
 * Active State: High
 */
#define DDRP_A_L_CHANGE_RD_DQS_DEFAULT             0x2ac,  5,  5

/* [3:3]  RO  reset 0x0  --  reg_a_l_left_boundary_overflow_for_rd
 * The flag that the DQS Rx delay line has reached the min value when doing the read training of byte0.
 */
#define DDRP_A_L_LEFT_BOUNDARY_OVERFLOW_FOR_RD     0x2ac,  3,  3

/* [2:2]  RO  reset 0x0  --  reg_a_l_right_boundary_overflow_for_rd
 * The flag that the DQS Rx delay line has reached the max value when doing the read training of byte0.
 */
#define DDRP_A_L_RIGHT_BOUNDARY_OVERFLOW_FOR_RD    0x2ac,  2,  2


/* ----- 0x2b0 ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq0
 * The read back data of the read training for A_DQ0. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ0        0x2b0, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq1
 * The read back data of the read training for A_DQ1. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ1        0x2b0, 15,  0


/* ----- 0x2b4 ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq2
 * The read back data of the read training for A_DQ2. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ2        0x2b4, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq3
 * The read back data of the read training for A_DQ3. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ3        0x2b4, 15,  0


/* ----- 0x2b8 ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq4
 * The read back data of the read training for A_DQ4. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ4        0x2b8, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq5
 * The read back data of the read training for A_DQ5. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ5        0x2b8, 15,  0


/* ----- 0x2bc ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq6
 * The read back data of the read training for A_DQ6. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ6        0x2bc, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_l_rd_train_readback_data_dq7
 * The read back data of the read training for A_DQ7. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_L_RD_TRAIN_READBACK_DATA_DQ7        0x2bc, 15,  0


/* ----- 0x2c0 ----- */
/* [26:26]  RO  reset 0x0  --  reg_a_l_change_dqs_default
 * The flag that can't find the pass window of the DQ after scanning all the Tx delay lines of the DQ for byte0. When this signal changes to high after the write training, it means that the current DQS is not in the good point.
 */
#define DDRP_A_L_CHANGE_DQS_DEFAULT                0x2c0, 26, 26

/* [25:18]  RO  reset 0xff  --  reg_a_l_train_min_for_dqs
 * The min pass point of the Tx delay line for A_DQS0/A_DQSB0.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQS                 0x2c0, 25, 18

/* [17:9]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq0
 * The min pass point of the Tx delay line for A_DQ0.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ0                 0x2c0, 17,  9

/* [8:0]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq1
 * The min pass point of the Tx delay line for A_DQ1.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ1                 0x2c0,  8,  0


/* ----- 0x2c4 ----- */
/* [26:18]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq2
 * The min pass point of the Tx delay line for A_DQ2.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ2                 0x2c4, 26, 18

/* [17:9]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq3
 * The min pass point of the Tx delay line for A_DQ3.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ3                 0x2c4, 17,  9

/* [8:0]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq4
 * The min pass point of the Tx delay line for A_DQ4.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ4                 0x2c4,  8,  0


/* ----- 0x2c8 ----- */
/* [26:18]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq5
 * The min pass point of the Tx delay line for A_DQ5.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ5                 0x2c8, 26, 18

/* [17:9]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq6
 * The min pass point of the Tx delay line for A_DQ6.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ6                 0x2c8, 17,  9

/* [8:0]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dq7
 * The min pass point of the Tx delay line for A_DQ7.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DQ7                 0x2c8,  8,  0


/* ----- 0x2cc ----- */
/* [25:18]  RO  reset 0x0  --  reg_a_l_train_max_for_dqs
 * The max pass point of the Tx delay line for A_DQS0/A_DQSB0.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQS                 0x2cc, 25, 18

/* [17:9]  RO  reset 0x0  --  reg_a_l_train_max_for_dq0
 * The max pass point of the Tx delay line for A_DQ0.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ0                 0x2cc, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_a_l_train_max_for_dq1
 * The max pass point of the Tx delay line for A_DQ1.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ1                 0x2cc,  8,  0


/* ----- 0x2e0 ----- */
/* [26:18]  RO  reset 0x0  --  reg_a_l_train_max_for_dq2
 * The max pass point of the Tx delay line for A_DQ2.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ2                 0x2e0, 26, 18

/* [17:9]  RO  reset 0x0  --  reg_a_l_train_max_for_dq3
 * The max pass point of the Tx delay line for A_DQ3.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ3                 0x2e0, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_a_l_train_max_for_dq4
 * The max pass point of the Tx delay line for A_DQ4.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ4                 0x2e0,  8,  0


/* ----- 0x2e4 ----- */
/* [26:18]  RO  reset 0x0  --  reg_a_l_train_max_for_dq5
 * The max pass point of the Tx delay line for A_DQ5.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ5                 0x2e4, 26, 18

/* [17:9]  RO  reset 0x0  --  reg_a_l_train_max_for_dq6
 * The max pass point of the Tx delay line for A_DQ6.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ6                 0x2e4, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_a_l_train_max_for_dq7
 * The max pass point of the Tx delay line for A_DQ7.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DQ7                 0x2e4,  8,  0


/* ----- 0x2e8 ----- */
/* [24:16]  RO  reset 0x0  --  reg_a_l_rdtrain_vref_max
 * The max value of the Rx VREF after the read training of the byte0/byte1.
 */
#define DDRP_A_L_RDTRAIN_VREF_MAX                  0x2e8, 24, 16

/* [8:0]  RO  reset 0x0  --  reg_a_l_rdtrain_vref_min
 * The min value of the Rx VREF after the read training of the byte0/byte1.
 */
#define DDRP_A_L_RDTRAIN_VREF_MIN                  0x2e8,  8,  0


/* ----- 0x2ec ----- */
/* [31:23]  RO  reset 0x1ff  --  reg_a_l_train_min_for_dm
 * The min pass point of the Tx delay line for A_DM0 when enabling reg_dm_wr_train_en.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_DM                  0x2ec, 31, 23

/* [22:14]  RO  reset 0x0  --  reg_a_l_train_max_for_dm
 * The max pass point of the Tx delay line for A_DM0 when enabling reg_dm_wr_train_en.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_DM                  0x2ec, 22, 14

/* [13:7]  RO  reset 0x7f  --  reg_a_l_train_min_for_rd_dm
 * The min pass point of the Rx delay line for A_DM0.
 */
#define DDRP_A_L_TRAIN_MIN_FOR_RD_DM               0x2ec, 13,  7

/* [6:0]  RO  reset 0x0  --  reg_a_l_train_max_for_rd_dm
 * The max pass point of the Rx delay line for A_DM0.
 */
#define DDRP_A_L_TRAIN_MAX_FOR_RD_DM               0x2ec,  6,  0


/* ----- 0x2f0 ----- */
/* [10:10]  RW  reset 0x0  --  reg_a_l_dqs_io_highz
 * set 1 to highz io of byte. A_DQS0/A_DQSB0/A_DM0/A_DQ0~A_DQ7.
 */
#define DDRP_A_L_DQS_IO_HIGHZ                      0x2f0, 10, 10

/* [9:2]  RW  reset 0x0  --  reg_a_l_dq_pad_test_pattern
 * DQ IO voltage and current_test pattern of tx.  When set to 1, DQ IO  keep 1.
 * [0]: A_DQ0
 * [1]: A_DQ1
 * ...
 * [7]: A_DQ7
 */
#define DDRP_A_L_DQ_PAD_TEST_PATTERN               0x2f0,  9,  2

/* [1:1]  RW  reset 0x0  --  reg_a_l_dm_pad_test_pattern
 * DM IO voltage and current_test pattern of tx.  When set to 1, A_DM0 IO  keep 1.
 */
#define DDRP_A_L_DM_PAD_TEST_PATTERN               0x2f0,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_a_l_dqs_pad_test_pattern
 * A_DQS0/A_DQSB0 IO voltage and current_test pattern of tx.  When set to 1, A_DQS0 IO keep 1, A_DQSB0 keep 0.
 */
#define DDRP_A_L_DQS_PAD_TEST_PATTERN              0x2f0,  0,  0


/* ============================ Byte1 ============================ */


/* ----- 0x300 ----- */
/* [31:23]  RW  reset 0x80  --  reg_a_h_vref1_margsel_reg
 * Used to configure the Vref value of the PHY.
 */
#define DDRP_A_H_VREF1_MARGSEL_REG                 0x300, 31, 23

/* [22:22]  RW  reset 0x0  --  reg_a_h_dq_odt_zqcali_en
 * The DQ ODT control signal.
 * 1: ZQ calibration result will be written into analog circuit.
 * 0: Values in I/O drive strength registers register will be written into the analog circuit.
 * Refer to ZQ calibration section in the Databook
 */
#define DDRP_A_H_DQ_ODT_ZQCALI_EN                  0x300, 22, 22

/* [21:21]  RW  reset 0x0  --  reg_a_h_dq_drv_zqcali_en
 * The DQ driver control signal.
 * 1: ZQ calibration result will be written into analog circuit.
 * 0: Values in I/O drive strength registers register will be written into the analog circuit.
 * Refer to ZQ calibration section in the Databook
 */
#define DDRP_A_H_DQ_DRV_ZQCALI_EN                  0x300, 21, 21

/* [20:20]  RW  reset 0x0  --  reg_a_h_abutweakpddq_reg
 * Weak pull-down of A_DQ8~A_DQ15.
 * 0: Disable
 * 1: Enable, about 2.2kohm pull-down strength
 */
#define DDRP_A_H_ABUTWEAKPDDQ_REG                  0x300, 20, 20

/* [19:19]  RW  reset 0x1  --  reg_a_h_abutweakpubdq_reg
 * Weak pull-up of A_DQ8~A_DQ15.
 * 0: Enable, about 2.2kohm pull-up strength
 * 1: Disable
 */
#define DDRP_A_H_ABUTWEAKPUBDQ_REG                 0x300, 19, 19

/* [18:18]  RW  reset 0x0  --  reg_a_h_abutdiffampseen_reg
 * Reserved.
 */
#define DDRP_A_H_ABUTDIFFAMPSEEN_REG               0x300, 18, 18

/* [17:17]  RW  reset 0x0  --  reg_a_h_vref1_pd_reg
 * Byte1 internal power down signal.
 * Active State: High
 */
#define DDRP_A_H_VREF1_PD_REG                      0x300, 17, 17

/* [16:15]  RW  reset 0x0  --  reg_a_h_weakpd_reg
 * DQS pull-down and DQSB pull-up control.
 * 00: Disable
 * 01/10: About 400ohm
 * 11: About 200ohm
 */
#define DDRP_A_H_WEAKPD_REG                        0x300, 16, 15

/* [14:13]  RW  reset 0x3  --  reg_a_h_weakpub_reg
 * DQS pull-up and DQSB pull-down control.
 * 11: Disable
 * 01/10: About 400ohm
 * 00: About 200ohm
 */
#define DDRP_A_H_WEAKPUB_REG                       0x300, 14, 13

/* [12:8]  RW  reset 0x0  --  reg_a_h_abutslewpu_reg
 * Byte1 edge slew rate control , default 0 means maximum slew rate.
 */
#define DDRP_A_H_ABUTSLEWPU_REG                    0x300, 12,  8

/* [7:7]  RW  reset 0x1  --  reg_a_h_enb_lp4mode_reg
 * Reserved.
 */
#define DDRP_A_H_ENB_LP4MODE_REG                   0x300,  7,  7

/* [6:6]  RW  reset 0x0  --  reg_a_h_dqsweakpd_reg
 * Weak pull-down of A_DQS1.
 * Active State: High
 * 0: Disable
 * 1: About 3.2kohm weak pull-down
 */
#define DDRP_A_H_DQSWEAKPD_REG                     0x300,  6,  6

/* [5:5]  RW  reset 0x1  --  reg_a_h_dqsbweakpub_reg
 * Weak pull-up of A_DQSB1.
 * Active State: Low
 * 0: About 3.2kohm weak pull-up
 * 1: Disable
 */
#define DDRP_A_H_DQSBWEAKPUB_REG                   0x300,  5,  5

/* [4:0]  RW  reset 0x0  --  reg_a_h_abutslewpd_reg
 * Reserved.
 */
#define DDRP_A_H_ABUTSLEWPD_REG                    0x300,  4,  0


/* ----- 0x304 ----- */
/* [30:30]  RW  reset 0x0  --  reg_a_h_dqfben_reg
 * The PHY BIST byte1 feedback enable signal.
 * Active State: High
 * Only for debug purpose.
 */
#define DDRP_A_H_DQFBEN_REG                        0x304, 30, 30

/* [29:29]  RW  reset 0x0  --  reg_a_h_dqfbsel_reg
 * The select signal of PHY BIST byte1.
 * 1: Choose the internal Tx driver output as the end
 * 0: Choose the pad as the end
 */
#define DDRP_A_H_DQFBSEL_REG                       0x304, 29, 29

/* [28:24]  RW  reset 0xe  --  reg_a_h_abutnrcompdq_reg
 * The driver pull-down resistance of byte1.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_H_ABUTNRCOMPDQ_REG                  0x304, 28, 24

/* [20:16]  RW  reset 0xe  --  reg_a_h_abutprcompdq_reg
 * The driver pull-up resistance of byte1.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_H_ABUTPRCOMPDQ_REG                  0x304, 20, 16

/* [12:8]  RW  reset 0x5  --  reg_a_h_abutodtpddq_reg
 * The ODT pull-down resistance of byte1.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_H_ABUTODTPDDQ_REG                   0x304, 12,  8

/* [4:0]  RW  reset 0x5  --  reg_a_h_abutodtpudq_reg
 * The ODT pull-up resistance of byte1.
 * Refer to CMD IO Drive Strengt hin the Databook.
 */
#define DDRP_A_H_ABUTODTPUDQ_REG                   0x304,  4,  0


/* ----- 0x308 ----- */
/* [26:24]  RW  reset 0x1  --  reg_a_h_rxmen0_delay_bp
 * Control the 1x delay of the Rx DQS calibration delay for byte1 of RANK0 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_H_RXMEN0_DELAY_BP                   0x308, 26, 24

/* [23:21]  RW  reset 0x7  --  reg_a_h_rxmen0_ophsel_bp
 * Control the 0.5*4x delay of the Rx DQS calibration delay for byte1 of RANK0 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_H_RXMEN0_OPHSEL_BP                  0x308, 23, 21

/* [20:16]  RW  reset 0x4  --  reg_a_h_rxmen0_sdlltap_bp
 * Control the delay line of the Rx DQS calibration delay for byte1 of RANK0 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_H_RXMEN0_SDLLTAP_BP                 0x308, 20, 16

/* [10:8]  RW  reset 0x1  --  reg_a_h_rxmen1_delay_bp
 * Control the 1x delay of the Rx DQS calibration delay  for byte1 of RANK1 when enabling  the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_H_RXMEN1_DELAY_BP                   0x308, 10,  8

/* [7:5]  RW  reset 0x7  --  reg_a_h_rxmen1_ophsel_bp
 * Control the 0.5*4x delay of the Rx DQS calibration delay for byte1 of RANK1 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_H_RXMEN1_OPHSEL_BP                  0x308,  7,  5

/* [4:0]  RW  reset 0x4  --  reg_a_h_rxmen1_sdlltap_bp
 * Control the delay line of the Rx DQS calibration delay for byte1 of RANK1 when enabling the Rx DQS calibration bypass mode by setting reg_calib_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_H_RXMEN1_SDLLTAP_BP                 0x308,  4,  0


/* ----- 0x30c ----- */
/* [26:24]  RW  reset 0x2  --  reg_a_h_rdodt0_delay
 * Control the 0.5*4x delay of the Rx ODT delay for byte1 of RANK0 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_H_RDODT0_DELAY                      0x30c, 26, 24

/* [23:21]  RW  reset 0x2  --  reg_a_h_rdodt0_ophsel
 * Control the delay line of the Rx ODT delay for byte1 of RANK0 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_H_RDODT0_OPHSEL                     0x30c, 23, 21

/* [20:16]  RW  reset 0x4  --  reg_a_h_rdodt0_dllsel
 * Control the 1x delay of the Rx ODT delay for byte1 of RANK0 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_H_RDODT0_DLLSEL                     0x30c, 20, 16

/* [10:8]  RW  reset 0x2  --  reg_a_h_rdodt1_delay
 * Control the 0.5*4x delay of the Rx ODT delay for byte1 of RANK1 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 0.5UI.
 */
#define DDRP_A_H_RDODT1_DELAY                      0x30c, 10,  8

/* [7:5]  RW  reset 0x2  --  reg_a_h_rdodt1_ophsel
 * Control the delay line of the Rx ODT delay for byte1 of RANK1 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI/256.
 */
#define DDRP_A_H_RDODT1_OPHSEL                     0x30c,  7,  5

/* [4:0]  RW  reset 0x4  --  reg_a_h_rdodt1_dllsel
 * Control the 1x delay of the Rx ODT delay  for byte1 of RANK1 when enabling the Rx ODT bypass mode by setting reg_rdodt_bypass to 1'b1.
 * Unit = 4UI.
 */
#define DDRP_A_H_RDODT1_DLLSEL                     0x30c,  4,  0


/* ----- 0x310 ----- */
/* [29:29]  RW  reset 0x0  --  reg_a_h_dq_invdelay_lp_en
 * Reserved.
 */
#define DDRP_A_H_DQ_INVDELAY_LP_EN                 0x310, 29, 29

/* [28:28]  RW  reset 0x0  --  reg_a_h_dm_obsdataen
 * Reserved.
 */
#define DDRP_A_H_DM_OBSDATAEN                      0x310, 28, 28

/* [27:24]  RW  reset 0x0  --  reg_a_h_dqobsmuxsel
 * Reserved.
 */
#define DDRP_A_H_DQOBSMUXSEL                       0x310, 27, 24

/* [23:23]  RW  reset 0x0  --  reg_a_h_rxen_lp4
 * Enable the Rx receiver of the LPDDR4 mode for byte1.
 * Active State: High
 */
#define DDRP_A_H_RXEN_LP4                          0x310, 23, 23

/* [22:22]  RW  reset 0x0  --  reg_a_h_lp4x_en
 * Enable the Rx receiver of the LPDDR4X mode for byte1.
 * Active State: High
 */
#define DDRP_A_H_LP4X_EN                           0x310, 22, 22

/* [21:21]  RW  reset 0x1  --  reg_a_h_pvt_comp_en
 * Enable the PVT compensation update function for byte1.
 * Active State: High
 */
#define DDRP_A_H_PVT_COMP_EN                       0x310, 21, 21

/* [20:20]  RW  reset 0x1  --  reg_a_h_dqout_mux
 * Used to control the timing of the DQ for byte1 from the digital part to the analog part.
 * 1: Align to the posedge of the dfi_clk1x
 * 0: Align to the negedge of the dfi_clk1x
 */
#define DDRP_A_H_DQOUT_MUX                         0x310, 20, 20

/* [19:19]  RW  reset 0x1  --  reg_a_h_dmout_mux
 * Used to control the timing of the DM for byte1 from the digital part to the analog part.
 * 1: Align to the posedge of the dfi_clk1x
 * 0: Align to the negedge of the dfi_clk1x
 */
#define DDRP_A_H_DMOUT_MUX                         0x310, 19, 19

/* [18:18]  RW  reset 0x0  --  reg_a_h_dq_ph90en_bp
 * Used to control the 90 degree of the DQ/DM for byte1 when the ph90en_bp_dqis set to 1'b1.
 * 1: Enable the 90 degree mode
 * 0: Disable the 90 degree mode
 */
#define DDRP_A_H_DQ_PH90EN_BP                      0x310, 18, 18

/* [17:17]  RW  reset 0x1  --  reg_a_h_dqs_ph90en_bp
 * Used to control the 90 degree of the DQS for byte1 when the ph90en_bp_dq is set to 1'b1.
 * 1: Enable the 90 degree mode
 * 0: Disable the 90 degree mode
 */
#define DDRP_A_H_DQS_PH90EN_BP                     0x310, 17, 17

/* [16:16]  RW  reset 0x1  --  reg_a_h_rcvdqsmodsel
 * Reserved.
 */
#define DDRP_A_H_RCVDQSMODSEL                      0x310, 16, 16

/* [15:15]  RW  reset 0x1  --  reg_a_h_selfclren
 * Reserved.
 */
#define DDRP_A_H_SELFCLREN                         0x310, 15, 15

/* [14:14]  RW  reset 0x1  --  reg_a_h_wrptrclrb
 * Reserved.
 */
#define DDRP_A_H_WRPTRCLRB                         0x310, 14, 14

/* [13:13]  RW  reset 0x1  --  reg_a_h_rxm_odiffampen
 * Reserved.
 */
#define DDRP_A_H_RXM_ODIFFAMPEN                    0x310, 13, 13

/* [12:12]  RW  reset 0x0  --  reg_a_h_abutobsmodeen
 * Reserved.
 */
#define DDRP_A_H_ABUTOBSMODEEN                     0x310, 12, 12

/* [10:10]  RW  reset 0x0  --  reg_a_h_rxm4p5en
 * The enable signal to extend the gating signal of the Rx DQS calibration for byte1 of the RANK0.
 * 1: Increase 0.5UI to the gating signal
 * 0: Keep the default state
 */
#define DDRP_A_H_RXM4P5EN                          0x310, 10, 10

/* [9:9]  RW  reset 0x1  --  reg_a_h_tsm_iobufact_bp
 * Reserved.
 */
#define DDRP_A_H_TSM_IOBUFACT_BP                   0x310,  9,  9

/* [8:8]  RW  reset 0x0  --  reg_a_h_rxpst_bp
 * Reserved.
 */
#define DDRP_A_H_RXPST_BP                          0x310,  8,  8

/* [6:6]  RW  reset 0x0  --  reg_a_h_rxm4p5en_r2
 * The enable signal to extend the gating signal of the Rx DQS calibration for byte1 of the RANK1.
 * 1: Increase 0.5UI to the gating signal
 * 0: Keep the default state
 */
#define DDRP_A_H_RXM4P5EN_R2                       0x310,  6,  6

/* [5:3]  RW  reset 0x0  --  reg_a_h_rrankdly_4x_cs0
 * Used to control the delay of the read rank switch signal of the byte1 for RANK0.
 * Unit= 0.5UI.
 */
#define DDRP_A_H_RRANKDLY_4X_CS0                   0x310,  5,  3

/* [2:0]  RW  reset 0x0  --  reg_a_h_rrankdly_4x_cs1
 * Used to control the delay of the read rank switch signal of the byte1 for RANK1.
 * Unit= 0.5UI.
 */
#define DDRP_A_H_RRANKDLY_4X_CS1                   0x310,  2,  0


/* ----- 0x314 ----- */
/* [18:16]  RW  reset 0x0  --  reg_a_h_rrankdly_1x_cs1
 * Used to control the delay of the read rank switch signal of the byte1 for RANK1.
 * Unit= 4UI.
 */
#define DDRP_A_H_RRANKDLY_1X_CS1                   0x314, 18, 16

/* [14:12]  RW  reset 0x0  --  reg_a_h_rrankdly_1x_cs0
 * Used to control the delay of the read rank switch signal of the byte1 for RANK0.
 * Unit= 4UI.
 */
#define DDRP_A_H_RRANKDLY_1X_CS0                   0x314, 14, 12

/* [10:8]  RW  reset 0x0  --  reg_a_h_wrankphsel
 * Used to control the 0.5*4x delay of the write rank switch signal of the byte1 for RANK0.
 * Unit = 0.5*UI.
 */
#define DDRP_A_H_WRANKPHSEL                        0x314, 10,  8

/* [7:0]  RW  reset 0x0  --  reg_a_h_wrankdlysel
 * Used to control the delay of the write rank switch signal of the byte1 for RANK0.
 * Unit= 0.5UI.
 */
#define DDRP_A_H_WRANKDLYSEL                       0x314,  7,  0


/* ----- 0x318 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_h_cs0_dm_invdelaysel
 * Used to control the Tx delay line of the A_DM1 for RANK0.
 */
#define DDRP_A_H_CS0_DM_INVDELAYSEL                0x318, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_h_cs0_dq0_invdelaysel
 * Used to control the Tx delay line of the A_DQ8 for RANK0.
 */
#define DDRP_A_H_CS0_DQ0_INVDELAYSEL               0x318, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_h_cs0_dq1_invdelaysel
 * Used to control the Tx delay line of the A_DQ9 for RANK0.
 */
#define DDRP_A_H_CS0_DQ1_INVDELAYSEL               0x318,  8,  0


/* ----- 0x31c ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_h_cs0_dq2_invdelaysel
 * Used to control the Tx delay line of the A_DQ10 for RANK0.
 */
#define DDRP_A_H_CS0_DQ2_INVDELAYSEL               0x31c, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_h_cs0_dq3_invdelaysel
 * Used to control the Tx delay line of the A_DQ11 for RANK0.
 */
#define DDRP_A_H_CS0_DQ3_INVDELAYSEL               0x31c, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_h_cs0_dq4_invdelaysel
 * Used to control the Tx delay line of the A_DQ12 for RANK0.
 */
#define DDRP_A_H_CS0_DQ4_INVDELAYSEL               0x31c,  8,  0


/* ----- 0x320 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_h_cs0_dq5_invdelaysel
 * Used to control the Tx delay line of the A_DQ13 for RANK0.
 */
#define DDRP_A_H_CS0_DQ5_INVDELAYSEL               0x320, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_h_cs0_dq6_invdelaysel
 * Used to control the Tx delay line of the A_DQ14 for RANK0.
 */
#define DDRP_A_H_CS0_DQ6_INVDELAYSEL               0x320, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_h_cs0_dq7_invdelaysel
 * Used to control the Tx delay line of the A_DQ15 for RANK0.
 */
#define DDRP_A_H_CS0_DQ7_INVDELAYSEL               0x320,  8,  0


/* ----- 0x324 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a_h_cs0_dqs_invdelaysel
 * Used to control the Tx delay line of the A_DQS1 for RANK0.
 */
#define DDRP_A_H_CS0_DQS_INVDELAYSEL               0x324, 31, 24

/* [15:8]  RW  reset 0x80  --  reg_a_h_cs0_dqsb_invdelaysel
 * Used to control the Tx delay line of the A_DQSB1 for RANK0.
 */
#define DDRP_A_H_CS0_DQSB_INVDELAYSEL              0x324, 15,  8


/* ----- 0x328 ----- */
/* [28:24]  RW  reset 0x0  --  reg_a_h_cs0_loop_invdelaysel
 * Choose the observation signal to check delay line control of the Byte1 for RANK0.
 * Set this register to the following values for the corresponding TX delay line:
 * 5'd0: DQ0/A_DQ8
 * 5'd1: DQ1/A_DQ9
 * …
 * 5'd7: DQ7/A_DQ15
 * 5'd8: DM0/A_DM1
 * 5'd9: DQS0/A_DQS1
 * 5'd10: DQSB0/A_DQSB1
 * Set this register to the following values for the corresponding RX delay line:
 * 5'd16: DQ0/A_DQ8
 * 5'd17: DQ1/A_DQ9
 * …
 * 5'd23: DQ7/A_DQ15
 * 5'd24: DM0/A_DM1
 * 5'd25: DQS0/A_DQS1
 * 5'd26: DQSB0/A_DQSB1
 */
#define DDRP_A_H_CS0_LOOP_INVDELAYSEL              0x328, 28, 24

/* [14:8]  RW  reset 0x7  --  reg_a_h_cs0_dm_invdelayselrx
 * Used to control the Rx delay line of the A_DM1 for RANK0.
 */
#define DDRP_A_H_CS0_DM_INVDELAYSELRX              0x328, 14,  8


/* ----- 0x32c ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_h_cs0_dq0_invdelayselrx
 * Used to control the Rx delay line of the A_DQ8 for RANK0.
 */
#define DDRP_A_H_CS0_DQ0_INVDELAYSELRX             0x32c, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_h_cs0_dq1_invdelayselrx
 * Used to control the Rx delay line of the A_DQ9 for RANK0.
 */
#define DDRP_A_H_CS0_DQ1_INVDELAYSELRX             0x32c, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_h_cs0_dq2_invdelayselrx
 * Used to control the Rx delay line of the A_DQ10 for RANK0.
 */
#define DDRP_A_H_CS0_DQ2_INVDELAYSELRX             0x32c, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_h_cs0_dq3_invdelayselrx
 * Used to control the Rx delay line of the A_DQ11 for RANK0.
 */
#define DDRP_A_H_CS0_DQ3_INVDELAYSELRX             0x32c,  6,  0


/* ----- 0x330 ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_h_cs0_dq4_invdelayselrx
 * Used to control the Rx delay line of the A_DQ12 for RANK0.
 */
#define DDRP_A_H_CS0_DQ4_INVDELAYSELRX             0x330, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_h_cs0_dq5_invdelayselrx
 * Used to control the Rx delay line of the A_DQ13 for RANK0.
 */
#define DDRP_A_H_CS0_DQ5_INVDELAYSELRX             0x330, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_h_cs0_dq6_invdelayselrx
 * Used to control the Rx delay line of the A_DQ14 for RANK0.
 */
#define DDRP_A_H_CS0_DQ6_INVDELAYSELRX             0x330, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_h_cs0_dq7_invdelayselrx
 * Used to control the Rx delay line of the A_DQ15 for RANK0.
 */
#define DDRP_A_H_CS0_DQ7_INVDELAYSELRX             0x330,  6,  0


/* ----- 0x334 ----- */
/* [30:24]  RW  reset 0x1f  --  reg_a_h_cs0_dqs_invdelayselrx
 * Used to control the Rx delay line of the A_DQS1 for RANK0.
 */
#define DDRP_A_H_CS0_DQS_INVDELAYSELRX             0x334, 30, 24

/* [14:8]  RW  reset 0x1f  --  reg_a_h_cs0_dqsb_invdelayselrx
 * Used to control the Rx delay line of the A_DQSB1 for RANK0.
 */
#define DDRP_A_H_CS0_DQSB_INVDELAYSELRX            0x334, 14,  8


/* ----- 0x338 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_h_cs1_dm_invdelaysel
 * Used to control the Tx delay line of the A_DM1 for RANK1.
 */
#define DDRP_A_H_CS1_DM_INVDELAYSEL                0x338, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_h_cs1_dq0_invdelaysel
 * Used to control the Tx delay line of the A_DQ8 for RANK1.
 */
#define DDRP_A_H_CS1_DQ0_INVDELAYSEL               0x338, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_h_cs1_dq1_invdelaysel
 * Used to control the Tx delay line of the A_DQ9 for RANK1.
 */
#define DDRP_A_H_CS1_DQ1_INVDELAYSEL               0x338,  8,  0


/* ----- 0x33c ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_h_cs1_dq2_invdelaysel
 * Used to control the Tx delay line of the A_DQ10 for RANK1.
 */
#define DDRP_A_H_CS1_DQ2_INVDELAYSEL               0x33c, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_h_cs1_dq3_invdelaysel
 * Used to control the Tx delay line of the A_DQ11 for RANK1.
 */
#define DDRP_A_H_CS1_DQ3_INVDELAYSEL               0x33c, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_h_cs1_dq4_invdelaysel
 * Used to control the Tx delay line of the A_DQ12 for RANK1.
 */
#define DDRP_A_H_CS1_DQ4_INVDELAYSEL               0x33c,  8,  0


/* ----- 0x340 ----- */
/* [26:18]  RW  reset 0x80  --  reg_a_h_cs1_dq5_invdelaysel
 * Used to control the Tx delay line of the A_DQ13 for RANK1.
 */
#define DDRP_A_H_CS1_DQ5_INVDELAYSEL               0x340, 26, 18

/* [17:9]  RW  reset 0x80  --  reg_a_h_cs1_dq6_invdelaysel
 * Used to control the Tx delay line of the A_DQ14 for RANK1.
 */
#define DDRP_A_H_CS1_DQ6_INVDELAYSEL               0x340, 17,  9

/* [8:0]  RW  reset 0x80  --  reg_a_h_cs1_dq7_invdelaysel
 * Used to control the Tx delay line of the A_DQ15 for RANK1.
 */
#define DDRP_A_H_CS1_DQ7_INVDELAYSEL               0x340,  8,  0


/* ----- 0x344 ----- */
/* [31:24]  RW  reset 0x80  --  reg_a_h_cs1_dqs_invdelaysel
 * Used to control the Tx delay line of the A_DQS1 for RANK1.
 */
#define DDRP_A_H_CS1_DQS_INVDELAYSEL               0x344, 31, 24

/* [15:8]  RW  reset 0x80  --  reg_a_h_cs1_dqsb_invdelaysel
 * Used to control the Tx delay line of the A_DQSB1 for RANK1.
 */
#define DDRP_A_H_CS1_DQSB_INVDELAYSEL              0x344, 15,  8


/* ----- 0x348 ----- */
/* [28:24]  RW  reset 0x0  --  reg_a_h_cs1_loop_invdelaysel
 * Choose the observation signal to check delay line control of the Byte1 for RANK1.
 * Set this register to the following values for the corresponding Rx delay line:
 * 5'd0: DQ0/A_DQ8
 * 5'd1: DQ1/A_DQ9
 * …
 * 5'd7: DQ7/A_DQ15
 * 5'd8: DM0/A_DM1
 * 5'd9: DQS0/A_DQS1
 * 5'd10: DQSB0/A_DQSB1
 * Set this register to the following values for the corresponding Tx delay line:
 * 5'd16: DQ0/A_DQ8
 * 5'd17: DQ1/A_DQ9
 * …
 * 5'd23: DQ7/A_DQ15
 * 5'd24: DM0/A_DM1
 * 5'd25: DQS0/A_DQS1
 * 5'd26: DQSB0/A_DQSB1
 */
#define DDRP_A_H_CS1_LOOP_INVDELAYSEL              0x348, 28, 24

/* [14:8]  RW  reset 0x7  --  reg_a_h_cs1_dm_invdelayselrx
 * Used to control the Rx delay line of the A_DM1 for RANK1.
 */
#define DDRP_A_H_CS1_DM_INVDELAYSELRX              0x348, 14,  8


/* ----- 0x34c ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_h_cs1_dq0_invdelayselrx
 * Used to control the Rx delay line of the A_DQ8 for RANK1.
 */
#define DDRP_A_H_CS1_DQ0_INVDELAYSELRX             0x34c, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_h_cs1_dq1_invdelayselrx
 * Used to control the Rx delay line of the A_DQ9 for RANK1.
 */
#define DDRP_A_H_CS1_DQ1_INVDELAYSELRX             0x34c, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_h_cs1_dq2_invdelayselrx
 * Used to control the Rx delay line of the A_DQ10 for RANK1.
 */
#define DDRP_A_H_CS1_DQ2_INVDELAYSELRX             0x34c, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_h_cs1_dq3_invdelayselrx
 * Used to control the Rx delay line of the A_DQ11 for RANK1.
 */
#define DDRP_A_H_CS1_DQ3_INVDELAYSELRX             0x34c,  6,  0


/* ----- 0x350 ----- */
/* [30:24]  RW  reset 0x0  --  reg_a_h_cs1_dq4_invdelayselrx
 * Used to control the Rx delay line of the A_DQ12 for RANK1.
 */
#define DDRP_A_H_CS1_DQ4_INVDELAYSELRX             0x350, 30, 24

/* [22:16]  RW  reset 0x0  --  reg_a_h_cs1_dq5_invdelayselrx
 * Used to control the Rx delay line of the A_DQ13 for RANK1.
 */
#define DDRP_A_H_CS1_DQ5_INVDELAYSELRX             0x350, 22, 16

/* [14:8]  RW  reset 0x0  --  reg_a_h_cs1_dq6_invdelayselrx
 * Used to control the Rx delay line of the A_DQ14 for RANK1.
 */
#define DDRP_A_H_CS1_DQ6_INVDELAYSELRX             0x350, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_h_cs1_dq7_invdelayselrx
 * Used to control the Rx delay line of the A_DQ15 for RANK1.
 */
#define DDRP_A_H_CS1_DQ7_INVDELAYSELRX             0x350,  6,  0


/* ----- 0x354 ----- */
/* [30:24]  RW  reset 0x1f  --  reg_a_h_cs1_dqs_invdelayselrx
 * Used to control the Rx delay line of the A_DQS1 for RANK1.
 */
#define DDRP_A_H_CS1_DQS_INVDELAYSELRX             0x354, 30, 24

/* [14:8]  RW  reset 0x1f  --  reg_a_h_cs1_dqsb_invdelayselrx
 * Used to control the Rx delay line of the A_DQSB1 for RANK1.
 */
#define DDRP_A_H_CS1_DQSB_INVDELAYSELRX            0x354, 14,  8


/* ----- 0x358 ----- */
/* [30:24]  RW  reset 0x1f  --  reg_a_h_rd_train_dqs_default
 * Used to control the start point of the Rx delay line of the DQS when read training for byte1. The read training will start based on this value.
 */
#define DDRP_A_H_RD_TRAIN_DQS_DEFAULT              0x358, 30, 24

/* [23:16]  RW  reset 0x7  --  reg_a_h_train_dqs_default
 * In the default mode, the write training will use the write training result to control the Tx delay line of the DQS and keep unchanged. The user also can set the reg_wr_train_dqs_default_bypass to 1'b1 to use this register to control the Tx delay line of the DQS for write training for byte1.
 */
#define DDRP_A_H_TRAIN_DQS_DEFAULT                 0x358, 23, 16

/* [14:8]  RW  reset 0x3f  --  reg_a_h_rd_train_dqs_range_max
 * Used to set the max scan range of the Rx delay line of the DQS when read training for byte1. The PHY will stop scanning when it increases the Rx delay line to this value.
 */
#define DDRP_A_H_RD_TRAIN_DQS_RANGE_MAX            0x358, 14,  8

/* [6:0]  RW  reset 0x0  --  reg_a_h_rd_train_dqs_range_min
 * Used to set the min scan range of the Rx delay line of the DQS when read training for byte1. The PHY will stop scanning when it decreases the Rx delay line to this value.
 */
#define DDRP_A_H_RD_TRAIN_DQS_RANGE_MIN            0x358,  6,  0


/* ----- 0x35c ----- */
/* [31:24]  RW  reset 0x0  --  reg_a_h_rdtrain_check_wrap0
 * Used to control the read check pattern after the remap in byte1. Valid only when the reg_rd_train_check_value_en is set to 1'b1.
 * For the read training of LPDDR4, the even DQ pads such as the DQ0/DQ2/DQ4/DQ6 have different check patterns with the ones of the odd DQS pads such as the DQ1/DQ3/DQ5/DQ7. After the remap in one byte, the DQ0 of the PHY may connect the DQ1 of the SDRAM. The read back data from the odd DQ pads will be sent to the even DQ pads to check. So the PHY needs to know the remap relation between the PHY and SDRAM.
 * For LPDDR4 mode, if the DQ pad of the PHY connects to the even pad of the SDRAM, the corresponding bit should be set to 1'b1, otherwise it will be set to 1'b0.
 * eg. If the DQ6 of the SDRAM connects to the DQ1 of the PHY. The bit[1] should be set to 1'b1. If the DQ7 of the SDRAM connects to the DQ2 of the PHY. The bit[7] should be set to 1'b0.
 * For the read training of DDR4, the DQ0/DQ1/DQ2/DQ3 have different check pattern and DQ4/DQ5/DQ6/DQ7 repeat the check pattern of the DQ0/DQ1/DQ2/DQ3.  After the remapping in byte1, the user needs to choose the check pattern according to the connection between the PHY and the SDRAM.
 * If the DQ0/DQ4 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ1/DQ5 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b01.
 * If the DQ2/DQ6 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ3/DQ7 of the SDRAM connect to the current pad of the PHY, the current pad should be set to 2'b01.
 * [1:0]: Used to control the check pattern of the DQ0 for byte1.
 * [3:2]: Used to control the check pattern of the DQ1 for byte1.
 * [5:4]: Used to control the check pattern of the DQ2 for byte1.
 * [7:6]: Used to control the check pattern of the DQ3 for byte1.
 * User can find more information in read training section
 */
#define DDRP_A_H_RDTRAIN_CHECK_WRAP0               0x35c, 31, 24

/* [23:16]  RW  reset 0x0  --  reg_a_h_rdtrain_check_wrap1
 * || For the read training of DDR4, the DQ0/DQ1/DQ2/DQ3 have different check pattern and DQ4/DQ5/DQ6/DQ7 repeat the check pattern of the DQ0/DQ1/DQ2/DQ3.  After the remap in byte1, the user needs to choose the check pattern according to the connection between the PHY and the SDRAM.
 * If the DQ0/DQ4 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ1/DQ5 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b01.
 * If the DQ2/DQ6 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b00.
 * If the DQ3/DQ7 of the SDRAM connect to current pad of the PHY, the current pad should be set to 2'b01.
 * [1:0]: Used to control the check pattern of the DQ4 for byte1.
 * [3:2]: Used to control the check pattern of the DQ5 for byte1.
 * [5:4]: Used to control the check pattern of the DQ6 for byte1.
 * [7:6]: Used to control the check pattern of the DQ7 for byte1.
 * User can find more information in read training section.
 */
#define DDRP_A_H_RDTRAIN_CHECK_WRAP1               0x35c, 23, 16

/* [15:14]  RW  reset 0x0  --  reg_a_h_cat_wrap_sel
 * Used to control the wrap between the bytes for LPDDR4 CAT.
 */
#define DDRP_A_H_CAT_WRAP_SEL                      0x35c, 15, 14

/* [13:10]  RW  reset 0x8  --  reg_a_h_dm_bit_wrap_sel
 * Used to control the wrap of DM between the bits in the same byte.
 */
#define DDRP_A_H_DM_BIT_WRAP_SEL                   0x35c, 13, 10


/* ----- 0x360 ----- */
/* [31:28]  RW  reset 0x7  --  reg_a_h_dq7_bit_wrap_sel
 * Used to control the wrap of DQ7 between the bits in the same byte.
 */
#define DDRP_A_H_DQ7_BIT_WRAP_SEL                  0x360, 31, 28

/* [27:24]  RW  reset 0x6  --  reg_a_h_dq6_bit_wrap_sel
 * Used to control the wrap of DQ6 between the bits in the same byte.
 */
#define DDRP_A_H_DQ6_BIT_WRAP_SEL                  0x360, 27, 24

/* [23:20]  RW  reset 0x5  --  reg_a_h_dq5_bit_wrap_sel
 * Used to control the wrap of DQ5 between the bits in the same byte.
 */
#define DDRP_A_H_DQ5_BIT_WRAP_SEL                  0x360, 23, 20

/* [19:16]  RW  reset 0x4  --  reg_a_h_dq4_bit_wrap_sel
 * Used to control the wrap of DQ4 between the bits in the same byte.
 */
#define DDRP_A_H_DQ4_BIT_WRAP_SEL                  0x360, 19, 16

/* [15:12]  RW  reset 0x3  --  reg_a_h_dq3_bit_wrap_sel
 * Used to control the wrap of DQ3 between the bits in the same byte.
 */
#define DDRP_A_H_DQ3_BIT_WRAP_SEL                  0x360, 15, 12

/* [11:8]  RW  reset 0x2  --  reg_a_h_dq2_bit_wrap_sel
 * Used to control the wrap of DQ2 between the bits in the same byte.
 */
#define DDRP_A_H_DQ2_BIT_WRAP_SEL                  0x360, 11,  8

/* [7:4]  RW  reset 0x1  --  reg_a_h_dq1_bit_wrap_sel
 * Used to control the wrap of DQ1 between the bits in the same byte.
 */
#define DDRP_A_H_DQ1_BIT_WRAP_SEL                  0x360,  7,  4

/* [3:0]  RW  reset 0x0  --  reg_a_h_dq0_bit_wrap_sel
 * Used to control the wrap of DQ0 between the bits in the same byte.
 */
#define DDRP_A_H_DQ0_BIT_WRAP_SEL                  0x360,  3,  0


/* ----- 0x380 ----- */
/* [30:30]  RO  reset 0x0  --  reg_a_h_dqs_idqshigh
 * Reserved.
 */
#define DDRP_A_H_DQS_IDQSHIGH                      0x380, 30, 30

/* [23:16]  RO  reset 0x80  --  reg_a_h_tdqs_invdelaysel0
 * The write-leveling result of byte1 for RANK0. The user can get the write-leveling result after the write-leveling is complete.
 */
#define DDRP_A_H_TDQS_INVDELAYSEL0                 0x380, 23, 16

/* [7:0]  RO  reset 0x80  --  reg_a_h_tdqs_invdelaysel1
 * The write-leveling result of byte1 for RANK1. The user can get the write-leveling result after the write-leveling is complete.
 */
#define DDRP_A_H_TDQS_INVDELAYSEL1                 0x380,  7,  0


/* ----- 0x384 ----- */
/* [18:16]  RO  reset 0x1  --  reg_a_h_cycsel
 * The current 1x delay of the Rx DQS calibration of byte1 for RANK0. Use this register to get the current working state of the Rx DQS calibration.
 */
#define DDRP_A_H_CYCSEL                            0x384, 18, 16

/* [10:8]  RO  reset 0x0  --  reg_a_h_ophsel
 * The current 0.5UI delay of the Rx DQS calibration of byte1 for RANK0. Use this register to get the current working state of the Rx DQS calibration.
 */
#define DDRP_A_H_OPHSEL                            0x384, 10,  8

/* [4:0]  RO  reset 0x2  --  reg_a_h_dllsel
 * The current 1/64UI Tx delay of the Rx DQS calibration of byte1 for RANK0. Use this register to get the current working state of the Rx DQS calibration.
 */
#define DDRP_A_H_DLLSEL                            0x384,  4,  0


/* ----- 0x388 ----- */
/* [26:16]  RO  reset 0x0  --  reg_a_h_calib_result_cs0
 * The training result of the Rx DQS calibration of byte1 for RANK0. The user can use this register to get the current working state of the Rx DQS calibration.
 * [20:16]: The 1/64UI Tx delay line value
 * [23:21]: The 0.5UI delay value
 * [26:24]: The 1x delay value
 */
#define DDRP_A_H_CALIB_RESULT_CS0                  0x388, 26, 16

/* [10:0]  RO  reset 0x0  --  reg_a_h_calib_result_cs1
 * The training result of the Rx DQS calibration of byte1 for RANK1. The user can use this register to get the current working state of the Rx DQS calibration.
 * [4:0]: The 1/64UI Tx delay line value
 * [7:5]: The 0.5UI delay value
 * [10:8]: The 1x delay value
 */
#define DDRP_A_H_CALIB_RESULT_CS1                  0x388, 10,  0


/* ----- 0x38c ----- */
/* [8:0]  RO  reset 0x0  --  reg_a_h_cs0_value_dqx_invdelaysel
 * The delay line value of the current observation signal choosing of reg_a_h_cs0_loop_invdelaysel.
 */
#define DDRP_A_H_CS0_VALUE_DQX_INVDELAYSEL         0x38c,  8,  0


/* ----- 0x390 ----- */
/* [8:0]  RO  reset 0x0  --  reg_a_h_cs1_value_dqx_invdelaysel
 * The delay line value of the current observation signal choosing of reg_a_h_cs1_loop_invdelaysel.
 */
#define DDRP_A_H_CS1_VALUE_DQX_INVDELAYSEL         0x390,  8,  0


/* ----- 0x394 ----- */
/* [30:24]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq0
 * The min pass point of the Rx delay line for A_DQ8.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ0              0x394, 30, 24

/* [22:16]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq1
 * The min pass point of the Rx delay line for A_DQ9.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ1              0x394, 22, 16

/* [14:8]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq2
 * The min pass point of the Rx delay line for A_DQ10.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ2              0x394, 14,  8

/* [6:0]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq3
 * The min pass point of the Rx delay line for A_DQ11.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ3              0x394,  6,  0


/* ----- 0x398 ----- */
/* [30:24]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq4
 * The min pass point of the Rx delay line for A_DQ12.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ4              0x398, 30, 24

/* [22:16]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq5
 * The min pass point of the Rx delay line for A_DQ13.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ5              0x398, 22, 16

/* [14:8]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq6
 * The min pass point of the Rx delay line for A_DQ14.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ6              0x398, 14,  8

/* [6:0]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dq7
 * The min pass point of the Rx delay line for A_DQ15.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQ7              0x398,  6,  0


/* ----- 0x39c ----- */
/* [14:8]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dqs
 * The min pass point of the Rx delay line for A_DQS1/A_DQSB1.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DQS              0x39c, 14,  8


/* ----- 0x3a0 ----- */
/* [30:24]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq0
 * The max pass point of the Rx delay line for A_DQ8.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ0              0x3a0, 30, 24

/* [22:16]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq1
 * The max pass point of the Rx delay line for A_DQ9.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ1              0x3a0, 22, 16

/* [14:8]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq2
 * The max pass point of the Rx delay line for A_DQ10.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ2              0x3a0, 14,  8

/* [6:0]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq3
 * The max pass point of the Rx delay line for A_DQ11.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ3              0x3a0,  6,  0


/* ----- 0x3a4 ----- */
/* [30:24]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq4
 * The max pass point of the Rx delay line for A_DQ12.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ4              0x3a4, 30, 24

/* [22:16]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq5
 * The max pass point of the Rx delay line for A_DQ13.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ5              0x3a4, 22, 16

/* [14:8]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq6
 * The max pass point of the Rx delay line for A_DQ14.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ6              0x3a4, 14,  8

/* [6:0]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dq7
 * The max pass point of the Rx delay line for A_DQ15.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQ7              0x3a4,  6,  0


/* ----- 0x3a8 ----- */
/* [14:8]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dqs
 * The max pass point of the Rx delay line for A_DQS1/A_DQSB1.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DQS              0x3a8, 14,  8


/* ----- 0x3ac ----- */
/* [30:24]  RO  reset 0x0  --  reg_a_h_train_result_for_rd_base_dqs
 * The best point of the Rx delay line for A_DQS1/A_DQSB1 after the read training.
 */
#define DDRP_A_H_TRAIN_RESULT_FOR_RD_BASE_DQS      0x3ac, 30, 24

/* [5:5]  RO  reset 0x0  --  reg_a_h_change_rd_dqs_default
 * The flag that can't find the pass window of the read training for byte1.
 * Active State: High
 */
#define DDRP_A_H_CHANGE_RD_DQS_DEFAULT             0x3ac,  5,  5

/* [3:3]  RO  reset 0x0  --  reg_a_h_left_boundary_overflow_for_rd
 * The flag that the DQS Rx delay line has reached the min value when doing the read training of byte1.
 */
#define DDRP_A_H_LEFT_BOUNDARY_OVERFLOW_FOR_RD     0x3ac,  3,  3

/* [2:2]  RO  reset 0x0  --  reg_a_h_right_boundary_overflow_for_rd
 * The flag that the DQS Rx delay line has reached the max value when doing the read training of byte1.
 */
#define DDRP_A_H_RIGHT_BOUNDARY_OVERFLOW_FOR_RD    0x3ac,  2,  2


/* ----- 0x3b0 ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq0
 * The read back data of the read training for A_DQ8. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ0        0x3b0, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq1
 * The read back data of the read training for A_DQ9. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ1        0x3b0, 15,  0


/* ----- 0x3b4 ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq2
 * The read back data of the read training for A_DQ10. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ2        0x3b4, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq3
 * The read back data of the read training for A_DQ11. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ3        0x3b4, 15,  0


/* ----- 0x3b8 ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq4
 * The read back data of the read training for A_DQ12. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ4        0x3b8, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq5
 * The read back data of the read training for A_DQ13. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ5        0x3b8, 15,  0


/* ----- 0x3bc ----- */
/* [31:16]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq6
 * The read back data of the read training for A_DQ14. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ6        0x3bc, 31, 16

/* [15:0]  RO  reset 0x0  --  reg_a_h_rd_train_readback_data_dq7
 * The read back data of the read training for A_DQ15. Used for LPDDR4 read training bypass mode.
 */
#define DDRP_A_H_RD_TRAIN_READBACK_DATA_DQ7        0x3bc, 15,  0


/* ----- 0x3c0 ----- */
/* [26:26]  RO  reset 0x0  --  reg_a_h_change_dqs_default
 * The flag that can't find the pass window of the DQ after scanning all the Tx delay lines of the DQ for byte1. When this signal changes to high after the write training, it means that the current DQS is not in the good point.
 */
#define DDRP_A_H_CHANGE_DQS_DEFAULT                0x3c0, 26, 26

/* [25:18]  RO  reset 0xff  --  reg_a_h_train_min_for_dqs
 * The min pass point of the Tx delay line for A_DQS1/A_DQSB1.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQS                 0x3c0, 25, 18

/* [17:9]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq0
 * The min pass point of the Tx delay line for A_DQ8.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ0                 0x3c0, 17,  9

/* [8:0]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq1
 * The min pass point of the Tx delay line for A_DQ9.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ1                 0x3c0,  8,  0


/* ----- 0x3c4 ----- */
/* [26:18]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq2
 * The min pass point of the Tx delay line for A_DQ10.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ2                 0x3c4, 26, 18

/* [17:9]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq3
 * The min pass point of the Tx delay line for A_DQ11.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ3                 0x3c4, 17,  9

/* [8:0]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq4
 * The min pass point of the Tx delay line for A_DQ12.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ4                 0x3c4,  8,  0


/* ----- 0x3c8 ----- */
/* [26:18]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq5
 * The min pass point of the Tx delay line for A_DQ13.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ5                 0x3c8, 26, 18

/* [17:9]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq6
 * The min pass point of the Tx delay line for A_DQ14.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ6                 0x3c8, 17,  9

/* [8:0]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dq7
 * The min pass point of the Tx delay line for A_DQ15.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DQ7                 0x3c8,  8,  0


/* ----- 0x3cc ----- */
/* [25:18]  RO  reset 0x0  --  reg_a_h_train_max_for_dqs
 * The max pass point of the Tx delay line for A_DQS1/A_DQSB1.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQS                 0x3cc, 25, 18

/* [17:9]  RO  reset 0x0  --  reg_a_h_train_max_for_dq0
 * The max pass point of the Tx delay line for A_DQ8.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ0                 0x3cc, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_a_h_train_max_for_dq1
 * The max pass point of the Tx delay line for A_DQ9.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ1                 0x3cc,  8,  0


/* ----- 0x3e0 ----- */
/* [26:18]  RO  reset 0x0  --  reg_a_h_train_max_for_dq2
 * The max pass point of the Tx delay line for A_DQ10.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ2                 0x3e0, 26, 18

/* [17:9]  RO  reset 0x0  --  reg_a_h_train_max_for_dq3
 * The max pass point of the Tx delay line for A_DQ11.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ3                 0x3e0, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_a_h_train_max_for_dq4
 * The max pass point of the Tx delay line for A_DQ12.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ4                 0x3e0,  8,  0


/* ----- 0x3e4 ----- */
/* [26:18]  RO  reset 0x0  --  reg_a_h_train_max_for_dq5
 * The max pass point of the Tx delay line for A_DQ13.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ5                 0x3e4, 26, 18

/* [17:9]  RO  reset 0x0  --  reg_a_h_train_max_for_dq6
 * The max pass point of the Tx delay line for A_DQ14.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ6                 0x3e4, 17,  9

/* [8:0]  RO  reset 0x0  --  reg_a_h_train_max_for_dq7
 * The max pass point of the Tx delay line for A_DQ15.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DQ7                 0x3e4,  8,  0


/* ----- 0x3e8 ----- */
/* [24:16]  RO  reset 0x0  --  reg_a_h_rdtrain_vref_max
 * The max value of the Rx VREF after the read training of the byte1/byte1.
 */
#define DDRP_A_H_RDTRAIN_VREF_MAX                  0x3e8, 24, 16

/* [8:0]  RO  reset 0x0  --  reg_a_h_rdtrain_vref_min
 * The min value of the Rx VREF after the read training of the byte1/byte1.
 */
#define DDRP_A_H_RDTRAIN_VREF_MIN                  0x3e8,  8,  0


/* ----- 0x3ec ----- */
/* [31:23]  RO  reset 0x1ff  --  reg_a_h_train_min_for_dm
 * The min pass point of the Tx delay line for A_DM1 when enabling reg_dm_wr_train_en.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_DM                  0x3ec, 31, 23

/* [22:14]  RO  reset 0x0  --  reg_a_h_train_max_for_dm
 * The max pass point of the Tx delay line for A_DM1 when enabling reg_dm_wr_train_en.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_DM                  0x3ec, 22, 14

/* [13:7]  RO  reset 0x7f  --  reg_a_h_train_min_for_rd_dm
 * The min pass point of the Rx delay line for A_DM1.
 */
#define DDRP_A_H_TRAIN_MIN_FOR_RD_DM               0x3ec, 13,  7

/* [6:0]  RO  reset 0x0  --  reg_a_h_train_max_for_rd_dm
 * The max pass point of the Rx delay line for A_DM1.
 */
#define DDRP_A_H_TRAIN_MAX_FOR_RD_DM               0x3ec,  6,  0


/* ----- 0x3f0 ----- */
/* [10:10]  RW  reset 0x0  --  reg_a_h_dqs_io_highz
 * set 1 to highz io of byte. A_DQS1/A_DQSB1/A_DM1/A_DQ8~A_DQ15.
 */
#define DDRP_A_H_DQS_IO_HIGHZ                      0x3f0, 10, 10

/* [9:2]  RW  reset 0x0  --  reg_a_h_dq_pad_test_pattern
 * DQ IO voltage and current_test pattern of tx.  When set to 1, DQ IO  keep 1.
 * [0]: A_DQ8
 * [1]: A_DQ9
 * ...
 * [7]: A_DQ15
 */
#define DDRP_A_H_DQ_PAD_TEST_PATTERN               0x3f0,  9,  2

/* [1:1]  RW  reset 0x0  --  reg_a_h_dm_pad_test_pattern
 * DM IO voltage and current_test pattern of tx.  When set to 1, A_DM1 IO  keep 1.
 */
#define DDRP_A_H_DM_PAD_TEST_PATTERN               0x3f0,  1,  1

/* [0:0]  RW  reset 0x0  --  reg_a_h_dqs_pad_test_pattern
 * A_DQS1/A_DQSB1 IO voltage and current_test pattern of tx.  When set to 1, A_DQS1 IO keep 1, A_DQSB1 keep 0.
 */
#define DDRP_A_H_DQS_PAD_TEST_PATTERN              0x3f0,  0,  0



#endif /* __DDRP_REGS_H__ */
