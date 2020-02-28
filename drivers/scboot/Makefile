include $(TOPDIR)/config.mk

LIB	:= $(obj)libscboot.o

ifdef CONFIG_JZ_SCBOOT
COBJS-$(CONFIG_JZ_CKEYAES) += $(SOC)/ckey_aes.o spi_checksum.o
COBJS-$(CONFIG_JZ_SECURE_SUPPORT) += $(SOC)/scboot.o md5.o flash.o
COBJS-y += aes.o rsa.o
ifdef CONFIG_BURNER
COBJS-y += secure.o
ifdef CONFIG_X2000_V12
COBJS-y += $(SOC)/otp.o $(SOC)/cmd_burn_key.o
else
COBJS-y += $(SOC)/otp.o $(SOC)/get_rn.o $(SOC)/cmd_sctest.o \
	$(SOC)/test_common.o $(SOC)/test_scboot.o $(SOC)/test_nku.o \
	$(SOC)/test_otp.o
endif
endif
endif

$(shell mkdir -p $(SPLTREE)/drivers/scboot/$(SOC))

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
