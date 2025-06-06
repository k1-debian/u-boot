/****************************************************************
*****************************************************************/
#ifndef __JZM_VPU_H__
#define __JZM_VPU_H__

/****************************************************************
  VPU register map
*****************************************************************/

#ifdef JZM_HUNT_SIM
# include "hunt.h"
#else
# define __place_k0_data__
# define __place_k0_text__
#endif

#define VPU_BASE             0xb3300000

#define	HID_SCH	             0x0
#define	HID_VDMA	     0x1
#define	HID_EFE	             0x4
#define	HID_MCE		     0x5
#define	HID_DBLK	     0x7
#define	HID_VMAU	     0x8
#define	HID_SDE		     0x9
#define	HID_AUX		     0xA
#define	HID_TCSM	     0xB
#define	HID_JPGC	     0xE
#define HID_SRAM             0xF

#define VPU_MAX_MB_WIDTH     256

#define MSCOPE_START(mbnum)  write_reg(VPU_BASE+0x24, mbnum)
#define MSCOPE_STOP()        write_reg(VPU_BASE+0x28, 0)

/********************************************
  SCH (Scheduler)
*********************************************/
#define TCSM_FLUSH           0xc0000
#define REG_SCH_GLBC         0x00000
#define SCH_GLBC_SLDE        (0x1<<31)
#define SCH_GLBC_TLBE        (0x1<<30)
#define SCH_GLBC_TLBINV      (0x1<<29)
#define SCH_INTE_ACFGERR     (0x1<<20)
#define SCH_INTE_TLBERR      (0x1<<18)
#define SCH_INTE_BSERR       (0x1<<17)
#define SCH_INTE_ENDF        (0x1<<16)
#define SCH_GLBC_HIMAP       (0x1<<15)
#define SCH_GLBC_HIAXI       (0x1<<9)
#define SCH_GLBC_EPRI0       (0x0<<7)
#define SCH_GLBC_EPRI1       (0x1<<7)
#define SCH_GLBC_EPRI2       (0x2<<7)
#define SCH_GLBC_EPRI3       (0x3<<7)

#define REG_SCH_TLBA         0x00030

#define REG_SCH_STAT         0x00034

#define REG_SCH_SLDE0        0x00040
#define REG_SCH_SLDE1        0x00044
#define REG_SCH_SLDE2        0x00048
#define REG_SCH_SLDE3        0x0004C
#define SCH_SLD_VTAG(val)    (((val) & 0xFFF)<<20)
#define SCH_SLD_MASK(val)    (((val) & 0xFFF)<<8)
#define SCH_SLD_VLD          (0x1<<0)

#define REG_SCH_SCHC         0x00060
#define SCH_CH1_PCH(ch)      (((ch) & 0x3)<<0)
#define SCH_CH2_PCH(ch)      (((ch) & 0x3)<<8)
#define SCH_CH3_PCH(ch)      (((ch) & 0x3)<<16)
#define SCH_CH4_PCH(ch)      (((ch) & 0x3)<<24)
#define SCH_CH1_PE           (0x1<<2)
#define SCH_CH2_PE           (0x1<<10)
#define SCH_CH3_PE           (0x1<<18)
#define SCH_CH4_PE           (0x1<<26)
#define SCH_CH1_GS0          (0x0<<3)
#define SCH_CH1_GS1          (0x1<<3)
#define SCH_CH2_GS0          (0x0<<11)
#define SCH_CH2_GS1          (0x1<<11)
#define SCH_CH3_GS0          (0x0<<19)
#define SCH_CH3_GS1          (0x1<<19)
#define SCH_CH4_GS0          (0x0<<27)
#define SCH_CH4_GS1          (0x1<<27)

#define REG_SCH_BND          0x00064
#define SCH_CH1_HID(hid)     (((hid) & 0xF)<<16)
#define SCH_CH2_HID(hid)     (((hid) & 0xF)<<20)
#define SCH_CH3_HID(hid)     (((hid) & 0xF)<<24)
#define SCH_CH4_HID(hid)     (((hid) & 0xF)<<28)
#define SCH_BND_G0F1         (0x1<<0)
#define SCH_BND_G0F2         (0x1<<1)
#define SCH_BND_G0F3         (0x1<<2)
#define SCH_BND_G0F4         (0x1<<3)
#define SCH_BND_G1F1         (0x1<<4)
#define SCH_BND_G1F2         (0x1<<5)
#define SCH_BND_G1F3         (0x1<<6)
#define SCH_BND_G1F4         (0x1<<7)
#define SCH_DEPTH(val)       (((val-1) & 0xF)<<8)

#define REG_SCH_SCHG0        0x00068
#define REG_SCH_SCHG1        0x0006C
#define REG_SCH_SCHE1        0x00070
#define REG_SCH_SCHE2        0x00074
#define REG_SCH_SCHE3        0x00078
#define REG_SCH_SCHE4        0x0007C

#define DSA_SCH_CH1          (VPU_BASE | REG_SCH_SCHE1)
#define DSA_SCH_CH2          (VPU_BASE | REG_SCH_SCHE2)
#define DSA_SCH_CH3          (VPU_BASE | REG_SCH_SCHE3)
#define DSA_SCH_CH4          (VPU_BASE | REG_SCH_SCHE4)

/********************************************
  VDMA (VPU general-purpose DMA)
*********************************************/
#define REG_VDMA_LOCK        0x10000
#define REG_VDMA_UNLK        0x10004

#define REG_VDMA_TASKRG      0x10008
#define VDMA_ACFG_RUN        (0x1)
#define VDMA_DESC_RUN        (0x3)
#define VDMA_ACFG_CLR        (0x8)
#define VDMA_ACFG_SAFE       (0x4)
#define VDMA_ACFG_DHA(a)     (((unsigned int)(a)) & 0xFFFFFF80)
#define VDMA_DESC_DHA(a)     (((unsigned int)(a)) & 0xFFFF0)

#define REG_VDMA_TASKST      0x1000C
#define VDMA_ACFG_ERR        (0x1<<3)
#define VDMA_ACFG_END        (0x1<<2)
#define VDMA_DESC_END        (0x1<<1)
#define VDMA_VPU_BUSY        (0x1<<0)

#define VDMA_DESC_EXTSEL     (0x1<<0)
#define VDMA_DESC_TLBSEL     (0x1<<1)
#define VDMA_DESC_LK         (0x1<<31)

#define VDMA_ACFG_VLD        (0x1<<31)
#define VDMA_ACFG_TERM       (0x1<<30)
#define VDMA_ACFG_IDX(a)     (((unsigned int)(a)) & 0xFFFFC)

#define GEN_VDMA_ACFG(chn, reg, lk, val)                 \
({*chn++ = val;                                          \
  *chn++ = (VDMA_ACFG_VLD | (lk) | VDMA_ACFG_IDX(reg));  \
})

/********************************************
  EFE (Encoder Front End)
*********************************************/
#define REG_EFE_CTRL         0x40000
#define EFE_BP_MBY(mb)       (((mb) & 0xFF)<<24)
#define EFE_BP_MBX(mb)       (((mb) & 0xFF)<<16)
#define EFE_X264_QP(qp)      (((qp) & 0x3F)<<8)
#define EFE_DBLK_EN          (0x1<<5)
#define EFE_SLICE_TYPE(a)    (((a) & 0x1)<<4)
#define EFE_DEBUG_EN         (0x1<<2)
#define EFE_EN               (0x1<<1)
#define EFE_RUN              (0x1<<0)

#define REG_EFE_GEOM         0x40004
#define EFE_FST_MBY(mb)      (((mb) & 0xFF)<<24)
#define EFE_FST_MBX(mb)      (((0/*FIXME*/) & 0xFF)<<16)
#define EFE_LST_MBY(mb)      (((mb) & 0xFF)<<8)
#define EFE_LST_MBX(mb)      (((mb) & 0xFF)<<0)

#define REG_EFE_COEF_BA      0x4000C
#define REG_EFE_RAWY_SBA     0x40010
#define REG_EFE_RAWC_SBA     0x40014
#define REG_EFE_TOPMV_BA     0x40018
#define REG_EFE_TOPPA_BA     0x4001C
#define REG_EFE_MECHN_BA     0x40020
#define REG_EFE_MAUCHN_BA    0x40024
#define REG_EFE_DBLKCHN_BA   0x40028
#define REG_EFE_SDECHN_BA    0x4002C
#define REG_EFE_RAW_DBA      0x40030
#define REG_EFE_MVRP         0x40100
#define REG_EFE_STAT         0x40110

/********************************************
  MCE (Motion Compensation/Estimation COMBO)
*********************************************/
#define REG_MCE_CTRL         0x50000
#define MCE_ESTI_MAX_BDIA(a) (((a) & 0xF)<<28)
#define MCE_ESTI_MAX_SDIA(a) (((a) & 0xF)<<24)
#define MCE_ESTI_USE_PMV     (0x1<<22)
#define MCE_ESTI_INTPEL      (0x0<<20)
#define MCE_ESTI_HPEL        (0x2<<20)
#define MCE_ESTI_QPEL        (0x3<<20)
#define MCE_ESTI_PUT_MET     (0x1<<19)
#define MCE_COMP_AUTO_EXPD   (0x1<<17)
#define MCE_CH2_EN           (0x1<<11)
#define MCE_CLKG_EN          (0x1<<8)
#define MCE_OFA_EN           (0x1<<7)
#define MCE_MODE_COMP        (0x0<<4)
#define MCE_MODE_ESTI        (0x1<<4)
#define MCE_CACHE_FLUSH      (0x1<<3)
#define MCE_EN               (0x1<<0)

#define REG_MCE_CH1_STAT     0x50004
#define REG_MCE_CH2_STAT     0x50804
#define MCE_PREF_END         (0x1<<2)
#define MCE_LINK_END         (0x1<<1)
#define MCE_TASK_END         (0x1<<0)

#define REG_MCE_MVPA         0x5000C
#define REG_MCE_IWTA         0x5000C

#define REG_MCE_CH1_PINFO    0x50020
#define REG_MCE_CH2_PINFO    0x50820
#define MCE_PINFO(rgr, its, its_sft, its_scale, its_rnd) \
( ((rgr) & 0x1)<<31 |                                    \
  ((its) & 0x1)<<28 |                                    \
  ((its_sft) & 0x7)<<24 |                                \
  ((its_scale) & 0xFF)<<16 |                             \
  ((its_rnd) & 0xFFFF)<<0                                \
)

#define REG_MCE_CH1_WINFO    0x50024
#define REG_MCE_CH2_WINFO1   0x50824
#define REG_MCE_CH2_WINFO2   0x50828
#define MCE_WINFO(wt, wtpd, wtmd, biavg_rnd, wt_denom,   \
                  wt_sft, wt_lcoef, wt_rcoef)            \
( ((wt) & 0x1)<<31 |                                     \
  ((wtpd) & 0x1)<<30 |                                   \
  ((wtmd) & 0x3)<<28 |                                   \
  ((biavg_rnd) & 0x1)<<27 |                              \
  ((wt_denom) & 0x7)<<24 |                               \
  ((wt_sft) & 0xF)<<16 |                                 \
  ((wt_lcoef) & 0xFF)<<8 |                               \
  ((wt_rcoef) & 0xFF)<<0                                 \
)

#define REG_MCE_CH1_WTRND    0x5002C
#define REG_MCE_CH2_WTRND    0x5082C
#define MCE_WTRND(wt2_rnd, wt1_rnd)                      \
( ((wt2_rnd) & 0xFFFF)<<16 |                             \
  ((wt1_rnd) & 0xFFFF)<<0                                \
)

#define REG_MCE_CH1_BINFO    0x50030
#define REG_MCE_CH2_BINFO    0x50830
#define MCE_BINFO(ary, expdy, expdx, ilmd, pel)          \
( ((ary) & 0x1)<<31 |                                    \
  ((expdy) & 0xF)<<24 |                                  \
  ((expdx) & 0xF)<<20 |                                  \
  ((ilmd) & 0x3)<<16 |                                   \
  ((pel) & 0x3)<<14                                      \
)

#define REG_MCE_CH1_IINFO1   0x50034
#define REG_MCE_CH1_IINFO2   0x50038
#define MCE_CH1_IINFO(intp, tap, pkg, hldgl, avsdgl,     \
                      intp_dir, intp_rnd, intp_sft,      \
                      sintp, sintp_rnd, sintp_bias)      \
( ((intp) & 0x1)<<31 |                                   \
  ((tap) & 0x3)<<28 |                                    \
  ((pkg) & 0x1)<<27 |                                    \
  ((hldgl) & 0x1)<<26 |                                  \
  ((avsdgl) & 0x1)<<25 |                                 \
  ((intp_dir) & 0x1)<<24 |                               \
  ((intp_rnd) & 0xFF)<<16 |                              \
  ((intp_sft) & 0xF)<<8 |                                \
  ((sintp) & 0x1)<<2 |                                   \
  ((sintp_rnd) & 0x1)<<1 |                               \
  ((sintp_bias) & 0x1)<<0                                \
)

#define REG_MCE_CH2_IINFO1   0x50834
#define REG_MCE_CH2_IINFO2   0x50838
#define MCE_CH2_IINFO(intp, intp_dir, intp_sft,          \
                      intp_lcoef, intp_rcoef, intp_rnd)  \
( ((intp) & 0x1)<<31 |                                   \
  ((intp_dir) & 0x1)<<15 |                               \
  ((intp_sft) & 0x7)<<12 |                               \
  ((intp_lcoef) & 0x7)<<9 |                              \
  ((intp_rcoef) & 0x7)<<6 |                              \
  ((intp_rnd) & 0x3F)<<0                                 \
)

#define REG_MCE_CH1_TAP1L    0x5003C
#define REG_MCE_CH1_TAP2L    0x50040
#define REG_MCE_CH1_TAP1M    0x50044
#define REG_MCE_CH1_TAP2M    0x50048
#define MCE_CH1_TAP(c1, c2, c3, c4)                      \
( ((c4) & 0xFF)<<24 |                                    \
  ((c3) & 0xFF)<<16 |                                    \
  ((c2) & 0xFF)<<8 |                                     \
  ((c1) & 0xFF)<<0                                       \
)

#define REG_MCE_CH1_STRD     0x5004C
#define REG_MCE_CH2_STRD     0x5084C
#define MCE_STRD(ref_strd, raw_strd, dst_strd)           \
( ((ref_strd) & 0xFFF)<<16 |                             \
  ((raw_strd) & 0xFF)<<8 |                               \
  ((dst_strd) & 0xFF)<<0                                 \
)

#define REG_MCE_GEOM         0x50050
#define MCE_GEOM(frm_height, frm_width)                  \
( ((frm_height) & 0xFFF)<<16 |                           \
  ((frm_width) & 0xFFF)<<0                               \
)

#define REG_MCE_DDC          0x50054
#define MCE_TDD_RUN          0x1<<0

#define REG_MCE_DSA          0x50058

#define REG_MCE_ESTIC        0x5005C
#define MCE_ESTIC(fsct, fsst, fsce, fsse)                \
( ((fsct) & 0xFFFF)<<16 |                                \
  ((fsst) & 0xF)<<4 |                                    \
  ((fsce) & 0x1)<<1 |                                    \
  ((fsse) & 0x1)<<0                                      \
)

#define REG_MCE_CH1_RLUT     0x50300
#define REG_MCE_CH2_RLUT     0x50B00
#define MCE_RLUT_WT(wcoef2, wofst2, wcoef1, wofst1)      \
( ((wcoef2) & 0xFF)<<24 |                                \
  ((wofst2) & 0xFF)<<16 |                                \
  ((wcoef1) & 0xFF)<<8 |                                 \
  ((wofst1) & 0xFF)<<0                                   \
)

#define REG_MCE_CH1_CLUT     0x50400

#define REG_MCE_CH1_ILUT     0x50500
#define REG_MCE_CH2_ILUT     0x50D00

/*Motion TDD*/
#define MCE_TDD_COMP_HEAD(vld, lk, ch1pel, ch2pel,       \
                          posmd, mvmd, tkn, mby, mbx)    \
( ((vld) & 0x1)<<31 |                                    \
  ((lk) & 0x1)<<30 |                                     \
  ((ch1pel) & 0x1)<<27 |                                 \
  ((ch2pel) & 0x3)<<25 |                                 \
  ((posmd) & 0x1)<<24 |                                  \
  ((mvmd) & 0x1)<<23 |                                   \
  ((tkn) & 0x7F)<<16 |                                   \
  ((mby) & 0xFF)<<8 |                                    \
  ((mbx) & 0xFF)<<0                                      \
)
#define MCE_TDD_COMP_MV(mvy, mvx)                        \
( ((mvy) & 0xFFFF)<<16 |                                 \
  ((mvx) & 0xFFFF)<<0                                    \
)
#define MCE_TDD_COMP_CMD(bidir, refdir, fld, fldsel,     \
                         rgr, its, doe,	cflo, ypos,      \
                         lilmd, cilmd, list,		 \
		         boy, box, bh, bw, pos) 	 \
( ((bidir) & 0x1)<<31 |                                  \
  ((refdir) & 0x1)<<30 |                                 \
  ((fld) & 0x1)<<29 |                                    \
  ((fldsel) & 0x1)<<28 |                                 \
  ((rgr) & 0x1)<<27 |                                    \
  ((its) & 0x1)<<26 |                                    \
  ((doe) & 0x1)<<25 |                                    \
  ((cflo) & 0x1)<<24 |                                   \
  ((ypos) & 0xF)<<20 |                                   \
  ((lilmd) & 0x3)<<18 |                                  \
  ((cilmd) & 0x3)<<16 |                                  \
  ((list) & 0xF)<<12 |                                   \
  ((boy) & 0x3)<<10 |                                    \
  ((box) & 0x3)<<8 |                                     \
  ((bh) & 0x3)<<6 |                                      \
  ((bw) & 0x3)<<4 |                                      \
  ((pos) & 0xF)<<0                                       \
)
#define MCE_TDD_ESTI(vld, lk, dmy, pmc, list,            \
                     boy, box, bh, bw, mby, mbx)         \
( ((vld) & 0x1)<<31 |                                    \
  ((lk) & 0x1)<<30 |                                     \
  ((dmy) & 0x1)<<27 |                                    \
  ((pmc) & 0x1)<<26 |                                    \
  ((list) & 0x3)<<24 |                                   \
  ((boy) & 0x3)<<22 |                                    \
  ((box) & 0x3)<<20 |                                    \
  ((bh) & 0x3)<<18 |                                     \
  ((bw) & 0x3)<<16 |                                     \
  ((mby) & 0xFF)<<8 |                                    \
  ((mbx) & 0xFF)<<0                                      \
)
#define MCE_TDD_CFG(vld, lk, cidx)                       \
( 0x1<<28 |                                              \
  ((vld) & 0x1)<<31 |                                    \
  ((lk) & 0x1)<<30 |                                     \
  ((cidx) & 0xFFF)<<0                                    \
)
#define MCE_TDD_SYNC(vld, lk, crst, id)                  \
( 0x1<<29 |                                              \
  ((vld) & 0x1)<<31 |                                    \
  ((lk) & 0x1)<<30 |                                     \
  ((crst) & 0x1)<<27 |                                   \
  ((id) & 0xFFFF)<<0                                     \
)

/********************************************
  VMAU (VPU Matrix Arithmetic Unit)
*********************************************/
#define REG_VMAU_MCBP        0x80000

#define REG_VMAU_QTPARA      0x80004

#define REG_VMAU_MAIN_ADDR   0x80008

#define REG_VMAU_NCCHN_ADDR  0x8000C

#define REG_VMAU_CHN_LEN     0x80010

#define REG_VMAU_ACBP        0x80014

#define REG_VMAU_CPREDM_TLV  0x80018

#define REG_VMAU_YPREDM0     0x8001C

#define REG_VMAU_YPREDM1     0x80020

#define REG_VMAU_GBL_RUN     0x80040
#define VMAU_RUN             0x1
#define VMAU_STOP            0x2
#define VMAU_RESET           0x4

#define REG_VMAU_GBL_CTR     0x80044
#define VMAU_CTRL_FIFO_M     0x1
#define VMAU_CTRL_IRQ_EN     0x10
#define VMAU_CTRL_SLPOW      0x10000
#define VMAU_CTRL_TO_DBLK    0x1000000

#define REG_VMAU_STATUS      0x80048

#define REG_VMAU_CCHN_ADDR   0x8004C

#define REG_VMAU_VIDEO_TYPE  0x80050
#define VMAU_FMT_H264        0x1
#define VMAU_FMT_RV9         0x2
#define VMAU_FMT_VC1         0x3
#define VMAU_FMT_MPEG2       0x4
#define VMAU_FMT_MPEG4       0x5
#define VMAU_FMT_VP8         0x6
#define VMAU_MODE_DEC        (0x0<<11)
#define VMAU_MODE_ENC        (0x1<<11)

#define REG_VMAU_Y_GS        0x80054

#define REG_VMAU_DEC_DONE    0x80058

#define REG_VMAU_ENC_DONE    0x8005C

#define REG_VMAU_POS         0x80060

#define REG_VMAU_MCF_STA     0x80064

#define REG_VMAU_DEC_YADDR   0x80068

#define REG_VMAU_DEC_UADDR   0x8006C

#define REG_VMAU_DEC_VADDR   0x80070

#define REG_VMAU_DEC_STR     0x80074

#define REG_VMAU_MEML        0x84000
#define REG_VMAU_QT          0x88000

/********************************************
  DBLK (deblock)
*********************************************/
#define REG_DBLK_DHA         0x70000

#define REG_DBLK_TRIG        0x70060
#define DBLK_RUN             0x1
#define DBLK_STOP            0x2
#define DBLK_RESET           0x4
#define DBLK_SLICE_RUN       0x8

#define REG_DBLK_CTRL        0x70064
#define DBLK_CTRL(expand, rotate, loop_filter)           \
( ((expand) & 0x1)<<4 |                                  \
  ((rotate) & 0x3)<<1 |                                  \
  ((loop_filter) & 0x1)<<0                               \
)

#define REG_DBLK_VTR         0x70068
#define DBLK_FMT_H264        0x1
#define DBLK_FMT_RV9         0x2
#define DBLK_FMT_VC1         0x3
#define DBLK_FMT_MPEG2       0x4
#define DBLK_FMT_MPEG4       0x5
#define DBLK_FMT_VP8         0x6
#define DBLK_FRM_I           0x0
#define DBLK_FRM_P           0x1
#define DBLK_FRM_B           0x2
#define DBLK_VTR(beta, alpha, vp8_spl, vp8_kf,           \
                 frm_typ, video_fmt)                     \
( ((beta) & 0xFF)<<24 |                                  \
  ((alpha) & 0xFF)<<16 |                                 \
  ((vp8_spl) & 0x1)<<9 |                                 \
  ((vp8_kf) & 0x1)<<5 |                                  \
  ((frm_typ) & 0x3)<<3 |                                 \
  ((video_fmt) & 0x7)<<0                                 \
)

#define REG_DBLK_FSTA        0x7006C

#define REG_DBLK_GSTA        0x70070

#define REG_DBLK_GSIZE       0x70074
#define DBLK_GSIZE(mb_height, mb_width)                  \
( ((mb_height) & 0xFFFF)<<16 |                           \
  ((mb_width) & 0xFFFF)<<0                               \
)

#define REG_DBLK_GENDA       0x70078

#define REG_DBLK_GPOS        0x7007C
#define DBLK_GPOS(first_mby, first_mbx)                  \
( ((first_mby) & 0xFFFF)<<16 |                           \
  ((first_mbx) & 0xFFFF)<<0                              \
)

#define REG_DBLK_GPIC_STR    0x70080
#define DBLK_GPIC_STR(dst_strd_c, dst_strd_y)            \
( ((dst_strd_c) & 0xFFFF)<<16 |                          \
  ((dst_strd_y) & 0xFFFF)<<0                             \
)

#define REG_DBLK_GPIC_YA     0x70084

#define REG_DBLK_GPIC_CA     0x70088

#define REG_DBLK_GP_ENDA     0x7008C

#define REG_DBLK_SLICE_ENDA  0x70090

#define REG_DBLK_BLK_CTRL    0x70094

#define REG_DBLK_BLK_FIFO    0x70098

#define REG_DBLK_GPIC_Y1A    0x700A0

#define REG_DBLK_GPIC_C1A    0x700A4

#define REG_DBLK_GPIC_STR1   0x700A8

/********************************************
  SDE (stream parser/encoding)
*********************************************/
#define REG_SDE_STAT         0x90000

#define REG_SDE_SL_CTRL      0x90004
#define SDE_SLICE_INIT       (0x1<<1)
#define SDE_MB_RUN           (0x1<<0)

#define REG_SDE_SL_GEOM      0x90008
#define SDE_SL_GEOM(mb_height, mb_width,                 \
                    first_mby, first_mbx)                \
( ((mb_height) & 0xFF)<<24 |                             \
  ((mb_width) & 0xFF)<<16 |                              \
  ((first_mby) & 0xFF)<<8 |                              \
  ((first_mbx) & 0xFF)<<0                                \
)

#define REG_SDE_GL_CTRL      0x9000C
#define SDE_BP(mby, mbx)                                 \
( ((mby) & 0xFF)<<24 |                                   \
  ((mbx) & 0xFF)<<16                                     \
)
#define SDE_MODE_AUTO        (0x0<<4)
#define SDE_MODE_STEP        (0x1<<4)
#define SDE_MODE_DEBUG       (0x2<<4)
#define SDE_EN               (0x1<<0)

#define REG_SDE_CODEC_ID     0x90010
#define SDE_FMT_H264_DEC     (0x1<<0)
#define SDE_FMT_H264_ENC     (0x1<<1)
#define SDE_FMT_VP8_DEC      (0x1<<2)
#define SDE_FMT_VC1_DEC      (0x1<<3)
#define SDE_FMT_MPEG2_DEC    (0x1<<4)

#define REG_SDE_CFG0         0x90014
#define REG_SDE_CFG1         0x90018
#define REG_SDE_CFG2         0x9001C
#define REG_SDE_CFG3         0x90020
#define REG_SDE_CFG4         0x90024
#define REG_SDE_CFG5         0x90028
#define REG_SDE_CFG6         0x9002C
#define REG_SDE_CFG7         0x90030
#define REG_SDE_CFG8         0x90034
#define REG_SDE_CFG9         0x90038
#define REG_SDE_CFG10        0x9003C
#define REG_SDE_CFG11        0x90040
#define REG_SDE_CFG12        0x90044
#define REG_SDE_CFG13        0x90048
#define REG_SDE_CFG14        0x9004C
#define REG_SDE_CFG15        0x90050

#define REG_SDE_CTX_TBL      0x92000
#define REG_SDE_CQP_TBL      0x93800

/****************************************************************
  VPU tables
*****************************************************************/
/********************************************
  Motion interpolation programable table
*********************************************/
#define IS_SKIRT  0
#define IS_MIRROR 1

#define IS_BIAVG  0
#define IS_WT1    1
#define IS_WT2    2
#define IS_FIXWT  3

#define IS_ILUT0  0
#define IS_ILUT1  2
#define IS_EC     1

#define IS_TCS     1
#define NOT_TCS    0
#define IS_SCS     1
#define NOT_SCS    0
#define IS_HLDGL   1
#define NOT_HLDGL  0
#define IS_AVSDGL  1
#define NOT_AVSDGL 0

#define INTP_HDIR  0
#define INTP_VDIR  1

enum IntpID {
  MPEG_HPEL = 0,
  MPEG_QPEL,
  H264_QPEL,
  H264_EPEL,
  RV8_TPEL,
  RV9_QPEL,
  RV9_CPEL,
  WMV2_QPEL,
  VC1_QPEL,
  AVS_QPEL,
  VP6_QPEL,
  VP8_QPEL,
  VP8_EPEL,
  VP8_BIL,
  VP8_FPEL, /*full-pixel for chroma*/
};

enum PosID {
  H0V0 = 0,
  H1V0,
  H2V0,
  H3V0,
  H0V1,
  H1V1,
  H2V1,
  H3V1,
  H0V2,
  H1V2,
  H2V2,
  H3V2,
  H0V3,
  H1V3,
  H2V3,
  H3V3,
};

enum TapTYP {
  TAP2 = 0,
  TAP4,
  TAP6,
  TAP8,
};

enum SPelSFT {
  HPEL = 1,
  QPEL,
  EPEL,
};

typedef struct IntpFMT_t{
  char tap;
  char intp_pkg[2];
  char hldgl;
  char avsdgl;
  char intp[2];
  char intp_dir[2];
  char intp_coef[2][8];
  char intp_rnd[2];
  char intp_sft[2];
  char intp_sintp[2];
  char intp_srnd[2];
  char intp_sbias[2];
}IntpFMT_t;

__place_k0_data__
static char AryFMT[] = {IS_SKIRT, IS_MIRROR, IS_SKIRT, IS_SKIRT,
			IS_SKIRT, IS_SKIRT, IS_SKIRT, IS_SKIRT,
			IS_SKIRT, IS_SKIRT, IS_SKIRT, IS_SKIRT,
			IS_SKIRT, IS_SKIRT, IS_SKIRT, IS_SKIRT};

__place_k0_data__
static char SubPel[] = {HPEL, QPEL, QPEL, EPEL,
			QPEL, QPEL, QPEL, QPEL,
			QPEL, QPEL, QPEL, QPEL,
			EPEL, HPEL, QPEL, QPEL};

__place_k0_data__
static IntpFMT_t IntpFMT[][16] = {
  {
    /************* MPEG_HPEL ***************/
    {/*H0V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H0V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H0V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H0V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0} },
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
  },

  {
    /************* MPEG_QPEL ***************/
    {/*H0V0*/
      TAP8, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1},{0},},
      {/*intp_rnd*/15, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1},{0},},
      {/*intp_rnd*/15, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1},{0},},
      {/*intp_rnd*/15, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V1*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1},{0},},
      {/*intp_rnd*/15, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/1, 1},
      {/*intp_srnd*/1, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/1, 1},
      {/*intp_srnd*/1, 1},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V2*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1},{0},},
      {/*intp_rnd*/15, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V3*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1},{0},},
      {/*intp_rnd*/15, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/1, 0},
    },
    {/*H1V3*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/1, 1},
      {/*intp_srnd*/1, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H2V3*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H3V3*/
      TAP8, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 3, -6, 20, 20, -6, 3, -1}, {-1, 3, -6, 20, 20, -6, 3, -1},},
      {/*intp_rnd*/15, 15},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/1, 1},
      {/*intp_srnd*/1, 1},
      {/*intp_sbias*/1, 1},
    },
  },

  {
    /************* H264_QPEL ***************/
    {/*H0V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V1*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP6, {NOT_TCS, IS_SCS}, IS_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 16},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 10},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP6, {NOT_TCS, IS_SCS}, IS_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 16},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V2*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 10},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 10},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 10},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H0V3*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/1, 0},
    },
    {/*H1V3*/
      TAP6, {NOT_TCS, IS_SCS}, IS_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 16},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H2V3*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 10},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H3V3*/
      TAP6, {NOT_TCS, IS_SCS}, IS_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 16},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/1, 1},
    },
  },

  {
    /************* H264_EPEL ***************/
    {/*H0V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/0},
    {/*H2V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{0},{0},},
      {/*intp_rnd*/4, 0},
      {/*intp_sft*/3, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V0*/0},
    {/*H0V1*/0},
    {/*H1V1*/0},
    {/*H2V1*/0},
    {/*H3V1*/0},
    {/*H0V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{0},{0},},
      {/*intp_rnd*/4, 0},
      {/*intp_sft*/3, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V2*/0},
    {/*H2V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{0},{0},},
      {/*intp_rnd*/0, 32},
      {/*intp_sft*/0, 6},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V2*/0},
    {/*H0V3*/0},
    {/*H1V3*/0},
    {/*H2V3*/0},
    {/*H3V3*/0},
  },

  {
    /************* RV8_TPEL ***************/
    {/*H0V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 12, 6, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 6, 12, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/0},
    {/*H0V1*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 12, 6, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 12, 6, -1, 0, 0, 0, 0}, {-1, 12, 6, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 0}, //{0,128}
      {/*intp_sft*/0, 8},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 6, 12, -1, 0, 0, 0, 0}, {-1, 12, 6, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 0}, //{0,128}
      {/*intp_sft*/0, 8},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/0},
    {/*H0V2*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 6, 12, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 12, 6, -1, 0, 0, 0, 0}, {-1, 6, 12, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 0}, //{0,128}
      {/*intp_sft*/0, 8},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 6, 12, -1, 0, 0, 0, 0}, {-1, 6, 12, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 0}, //{0,128}
      {/*intp_sft*/0, 8},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/0},
    {/*H0V3*/0},
    {/*H1V3*/0},
    {/*H2V3*/0},
    {/*H3V3*/0},
  },

  {
    /************* RV9_QPEL ***************/
    {/*H0V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -5, 52, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/32, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -5, 20, 52, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/32, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V1*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -5, 52, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/32, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 52, 20, -5, 1, 0, 0}, {1, -5, 52, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/32, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 52, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 32},
      {/*intp_sft*/5, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 52, -5, 1, 0, 0}, {1, -5, 52, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/32, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V2*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/16, 0},
      {/*intp_sft*/5, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 52, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/32, 16},
      {/*intp_sft*/6, 5},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 16},
      {/*intp_sft*/5, 5},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 52, -5, 1, 0, 0}, {1, -5, 20, 20, -5, 1, 0, 0},},
      {/*intp_rnd*/32, 16},
      {/*intp_sft*/6, 5},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V3*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -5, 20, 52, -5, 1, 0, 0},{0},},
      {/*intp_rnd*/32, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V3*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 52, 20, -5, 1, 0, 0}, {1, -5, 20, 52, -5, 1, 0, 0},},
      {/*intp_rnd*/32, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V3*/
      TAP6, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -5, 20, 20, -5, 1, 0, 0}, {1, -5, 20, 52, -5, 1, 0, 0},},
      {/*intp_rnd*/16, 32},
      {/*intp_sft*/5, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 2},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
  },

  {
    /************* RV9_CPEL ***************/
    {/*H0V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/2, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0}, {3, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 7},
      {/*intp_sft*/0, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {3, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 4},
      {/*intp_sft*/0, 3},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0}, {3, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 7},
      {/*intp_sft*/0, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 4},
      {/*intp_sft*/0, 3},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 1},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 4},
      {/*intp_sft*/0, 3},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/2, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0}, {1, 3, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 7},
      {/*intp_sft*/0, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 3, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 4},
      {/*intp_sft*/0, 3},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V3*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 1},
      {/*intp_sft*/0, 2},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
  },

  {
    /************* WMV2_QPEL ***************/
    {/*H0V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/1, 0},
      {/*intp_srnd*/1, 0},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V1*/0},
    {/*H1V1*/0},
    {/*H2V1*/0},
    {/*H3V1*/0},
    {/*H0V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0}, {-1, 9, 9, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 8},
      {/*intp_sft*/4, 4},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0}, {-1, 9, 9, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 8},
      {/*intp_sft*/4, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0}, {-1, 9, 9, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/8, 8},
      {/*intp_sft*/4, 4},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H0V3*/0},
    {/*H1V3*/0},
    {/*H2V3*/0},
    {/*H3V3*/0},
  },

  {
    /************* VC1_QPEL ***************/
    {/*H0V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-4, 53, 18, -3, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/32, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/8, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-3, 18, 53, -4, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/32, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V1*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-4, 53, 18, -3, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/31, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-4, 53, 18, -3, 0, 0, 0, 0}, {-4, 53, 18, -3, 0, 0, 0, 0},},
      {/*intp_rnd*/31, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-4, 53, 18, -3, 0, 0, 0, 0}, {-1, 9, 9, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/31, 8},
      {/*intp_sft*/6, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-4, 53, 18, -3, 0, 0, 0, 0}, {-3, 18, 53, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/31, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/7, 0},
      {/*intp_sft*/4, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0}, {-4, 53, 18, -3, 0, 0, 0, 0},},
      {/*intp_rnd*/7, 32},
      {/*intp_sft*/4, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0}, {-1, 9, 9, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/7, 8},
      {/*intp_sft*/4, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-1, 9, 9, -1, 0, 0, 0, 0}, {-3, 18, 53, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/7, 32},
      {/*intp_sft*/4, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V3*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-3, 18, 53, -4, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/31, 0},
      {/*intp_sft*/6, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V3*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-3, 18, 53, -4, 0, 0, 0, 0}, {-4, 53, 18, -3, 0, 0, 0, 0},},
      {/*intp_rnd*/31, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V3*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-3, 18, 53, -4, 0, 0, 0, 0}, {-1, 9, 9, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/31, 8},
      {/*intp_sft*/6, 4},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V3*/
      TAP4, {NOT_TCS, IS_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_VDIR, INTP_HDIR},
      {/*intp_coef*/{-3, 18, 53, -4, 0, 0, 0, 0}, {-3, 18, 53, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/31, 32},
      {/*intp_sft*/6, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
  },

  {
    /************* AVS_QPEL ***************/
    {/*H0V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, -2, 96, 42, -7, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/4, 0},
      {/*intp_sft*/3, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{0, -7, 42, 96, -2, -1, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V1*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, -2, 96, 42, -7, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, IS_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0}, {-1, 5, -5, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 32},
      {/*intp_sft*/0, 6},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{0, -1, 5, 5, -1, 0, 0, 0}, {-1, -2, 96, 42, -7, 0, 0, 0},},
      {/*intp_rnd*/64, 0}, //{0,512}
      {/*intp_sft*/0, 10},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, IS_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0}, {-1, 5, -5, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 32},
      {/*intp_sft*/0, 6},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/1, 0},
    },
    {/*H0V2*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/4, 0},
      {/*intp_sft*/3, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, -2, 96, 42, -7, 0, 0, 0}, {0, -1, 5, 5, -1, 0, 0, 0},},
      {/*intp_rnd*/64, 0}, //{0,512}
      {/*intp_sft*/0, 10},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0}, {-1, 5, 5, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 32},
      {/*intp_sft*/0, 6},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{0, -7, 42, 96, -2, -1, 0, 0}, {0, -1, 5, 5, -1, 0, 0, 0},},
      {/*intp_rnd*/64, 0}, //{0,512}
      {/*intp_sft*/0, 10},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V3*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{0, -7, 42, 96, -2, -1, 0, 0}, {0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V3*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, IS_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0}, {-1, 5, -5, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 32},
      {/*intp_sft*/0, 6},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/0, 1},
    },
    {/*H2V3*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{0, -1, 5, 5, -1, 0, 0, 0}, {0, -7, 42, 96, -2, -1, 0, 0},},
      {/*intp_rnd*/64, 0}, //{0,512}
      {/*intp_sft*/0, 10},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V3*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, IS_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 5, 5, -1, 0, 0, 0, 0}, {-1, 5, -5, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/0, 32},
      {/*intp_sft*/0, 6},
      {/*intp_sintp*/0, 1},
      {/*intp_srnd*/0, 1},
      {/*intp_sbias*/1, 1},
    },
  },

  {
    /************* VP6_QPEL ***************/
    {/*H0V0*/
      TAP4, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-4, 109, 24, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-4, 68, 68, -4, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{-1, 24, 109, -4, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V1*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-4, 109, 24, -1, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-4, 109, 24, -1, 0, 0, 0, 0}, {-4, 109, 24, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-4, 68, 68, -4, 0, 0, 0, 0}, {-4, 109, 24, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 24, 109, -4, 0, 0, 0, 0}, {-4, 109, 24, -1, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V2*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-4, 68, 68, -4, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-4, 109, 24, -1, 0, 0, 0, 0}, {-4, 68, 68, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-4, 68, 68, -4, 0, 0, 0, 0}, {-4, 68, 68, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 24, 109, -4, 0, 0, 0, 0}, {-4, 68, 68, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V3*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{-1, 24, 109, -4, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V3*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-4, 109, 24, -1, 0, 0, 0, 0}, {-1, 24, 109, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V3*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-4, 68, 68, -4, 0, 0, 0, 0}, {-1, 24, 109, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V3*/
      TAP4, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{-1, 24, 109, -4, 0, 0, 0, 0}, {-1, 24, 109, -4, 0, 0, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
  },

  {
    /************* VP8_QPEL ***************/
    {/*H0V0*/
      TAP6, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{2, -11, 108, 36, -8, 1, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V0*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{3, -16, 77, 77, -16, 3, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V0*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, -8, 36, 108, -11, 2, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V1*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{2, -11, 108, 36, -8, 1, 0, 0}, {0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V1*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{2, -11, 108, 36, -8, 1, 0, 0}, {2, -11, 108, 36, -8, 1, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V1*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, -16, 77, 77, -16, 3, 0, 0}, {2, -11, 108, 36, -8, 1, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V1*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -8, 36, 108, -11, 2, 0, 0}, {2, -11, 108, 36, -8, 1, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V2*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{3, -16, 77, 77, -16, 3, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V2*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{2, -11, 108, 36, -8, 1, 0, 0}, {3, -16, 77, 77, -16, 3, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V2*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, -16, 77, 77, -16, 3, 0, 0}, {3, -16, 77, 77, -16, 3, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V2*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -8, 36, 108, -11, 2, 0, 0}, {3, -16, 77, 77, -16, 3, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H0V3*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, -8, 36, 108, -11, 2, 0, 0},{0},},
      {/*intp_rnd*/64, 0},
      {/*intp_sft*/7, 0},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H1V3*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{2, -11, 108, 36, -8, 1, 0, 0}, {1, -8, 36, 108, -11, 2, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H2V3*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, -16, 77, 77, -16, 3, 0, 0}, {1, -8, 36, 108, -11, 2, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
    {/*H3V3*/
      TAP6, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, -8, 36, 108, -11, 2, 0, 0}, {1, -8, 36, 108, -11, 2, 0, 0},},
      {/*intp_rnd*/64, 64},
      {/*intp_sft*/7, 7},
      {/*intp_sintp*/0, 0},
      {/*intp_srnd*/0, 0},
      {/*intp_sbias*/0, 0},
    },
  },

  {
    /************* VP8_EPEL ***************/
    {/*H0V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/0},
    {/*H2V0*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{0},{0},},
      {/*intp_rnd*/4, 0},
      {/*intp_sft*/3, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V0*/0},
    {/*H0V1*/0},
    {/*H1V1*/0},
    {/*H2V1*/0},
    {/*H3V1*/0},
    {/*H0V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{0},{0},},
      {/*intp_rnd*/4, 0},
      {/*intp_sft*/3, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V2*/0},
    {/*H2V2*/
      TAP2, {NOT_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{0},{0},},
      {/*intp_rnd*/4, 4},
      {/*intp_sft*/3, 3},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V2*/0},
    {/*H0V3*/0},
    {/*H1V3*/0},
    {/*H2V3*/0},
    {/*H3V3*/0},
  },

  {
    /************* VP8_BIL ***************/
    {/*H0V0*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/0, 0}, {0},
      {/*intp_coef*/{1, 0, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/0, 0},
      {/*intp_sft*/0, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V0*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/2, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V0*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V0*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_HDIR, 0},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/2, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H0V1*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/2, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V1*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{3, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/2, 2},
      {/*intp_sft*/2, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V1*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{3, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/1, 2},
      {/*intp_sft*/1, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V1*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{3, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/2, 2},
      {/*intp_sft*/2, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H0V2*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/1, 0},
      {/*intp_sft*/1, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V2*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/2, 1},
      {/*intp_sft*/2, 1},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V2*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/1, 1},
      {/*intp_sft*/1, 1},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V2*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{1, 1, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/2, 1},
      {/*intp_sft*/2, 1},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H0V3*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 0}, {INTP_VDIR, 0},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{0},},
      {/*intp_rnd*/2, 0},
      {/*intp_sft*/2, 0},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H1V3*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{3, 1, 0, 0, 0, 0, 0, 0},{1, 3, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/2, 2},
      {/*intp_sft*/2, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H2V3*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 1, 0, 0, 0, 0, 0, 0},{1, 3, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/1, 2},
      {/*intp_sft*/1, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
    {/*H3V3*/
      TAP2, {IS_TCS, NOT_SCS}, NOT_HLDGL, NOT_AVSDGL,
      {/*intp*/1, 1}, {INTP_HDIR, INTP_VDIR},
      {/*intp_coef*/{1, 3, 0, 0, 0, 0, 0, 0},{1, 3, 0, 0, 0, 0, 0, 0},},
      {/*intp_rnd*/2, 2},
      {/*intp_sft*/2, 2},
      {/*intp_sintp*/0},
      {/*intp_srnd*/0},
      {/*intp_sbias*/0},
    },
  },

  {
    /************* VP8_FPEL ***************/
    {/*H0V0*/0},
    {/*H1V0*/0},
    {/*H2V0*/0},
    {/*H3V0*/0},
    {/*H0V1*/0},
    {/*H1V1*/0},
    {/*H2V1*/0},
    {/*H3V1*/0},
    {/*H0V2*/0},
    {/*H1V2*/0},
    {/*H2V2*/0},
    {/*H3V2*/0},
    {/*H0V3*/0},
    {/*H1V3*/0},
    {/*H2V3*/0},
    {/*H3V3*/0},
  },
};

#endif /*__JZM_VPU_H__*/
