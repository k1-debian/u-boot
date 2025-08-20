#ifndef __CCU_H__
#define __CCU_H__

#define CCU_IO_BASE			0xb2200000
#define CCU_RESET_ENTRY		0xbfc00000

#define ccu_inl(addr) *(volatile unsigned int *)(addr)
#define ccu_outl(val,addr) *(volatile unsigned int *)(addr)=(unsigned int)val



#define get_ccu_cscr()			ccu_inl(CCU_IO_BASE + 0)
#define set_ccu_cscr(val)		ccu_outl(val, CCU_IO_BASE + 0)

#define get_ccu_cssr()			ccu_inl(CCU_IO_BASE + 0x20)

#define get_ccu_csrr()			ccu_inl(CCU_IO_BASE + 0x40)
#define set_ccu_csrr(val)		ccu_outl(val, CCU_IO_BASE + 0x40)

#define get_ccu_pipr()			ccu_inl(CCU_IO_BASE + 0x100)

#define get_ccu_pimr()			ccu_inl(CCU_IO_BASE + 0x120)
#define set_ccu_pimr(val)		ccu_outl(val, CCU_IO_BASE + 0x120)

#define get_ccu_mipr()			ccu_inl(CCU_IO_BASE + 0x140)

#define get_ccu_mimr()			ccu_inl(CCU_IO_BASE + 0x160)
#define set_ccu_mimr(val)		ccu_outl(val, CCU_IO_BASE + 0x160)

#define get_ccu_oipr()			ccu_inl(CCU_IO_BASE + 0x180)

#define get_ccu_oimr()			ccu_inl(CCU_IO_BASE + 0x1a0)
#define set_ccu_oimr(val)		ccu_outl(val, CCU_IO_BASE + 0x1a0)

#define get_ccu_rer()			ccu_inl(CCU_IO_BASE + 0xf00)
#define set_ccu_rer(val)		ccu_outl(val, CCU_IO_BASE + 0xf00)

#define get_ccu_cslr()			ccu_inl(CCU_IO_BASE + 0xff8)
#define set_ccu_cslr(val)		ccu_outl(val, CCU_IO_BASE + 0xff8)

#define get_ccu_csar()			ccu_inl(CCU_IO_BASE + 0xffc)
#define set_ccu_csar(val)		ccu_outl(val, CCU_IO_BASE + 0xffc)

#endif
