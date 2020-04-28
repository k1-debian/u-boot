include $(TOPDIR)/config.mk

LIB	:= $(obj)libscboot.o

ifdef CONFIG_X2000_V12
SCV = jz_sec_v2
else
SCV = jz_sec_v1
endif

ifdef CONFIG_JZ_SCBOOT
COBJS-$(CONFIG_JZ_CKEYAES) += $(SCV)/ckey_aes.o spi_checksum.o
COBJS-$(CONFIG_JZ_SECURE_SUPPORT) += $(SCV)/scboot.o md5.o flash.o
COBJS-y += aes.o rsa.o
ifdef CONFIG_BURNER
COBJS-y += $(SCV)/secure.o $(SCV)/otp.o
ifdef CONFIG_X2000_V12
COBJS-y += $(SCV)/cmd_burn_key.o
else
COBJS-y += $(SCV)/get_rn.o $(SCV)/cmd_sctest.o $(SCV)/test_otp.o \
	$(SCV)/test_common.o $(SCV)/test_scboot.o $(SCV)/test_nku.o

endif
endif
endif

$(shell mkdir -p $(SPLTREE)/drivers/scboot/$(SCV))

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
