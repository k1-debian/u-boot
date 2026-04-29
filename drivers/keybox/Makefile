include $(TOPDIR)/config.mk

LIB := $(obj)libkeybox.o

COBJS-$(CONFIG_KEYBOX) += keybox.o
COBJS-$(CONFIG_KEYBOX) += keybox_source.o
COBJS-$(CONFIG_KEYBOX) += keybox_source_mmc.o
COBJS-$(CONFIG_KEYBOX) += keybox_source_nor.o
COBJS-$(CONFIG_KEYBOX) += keybox_source_nand.o

COBJS := $(COBJS-y)
SRCS := $(COBJS:.o=.c)
OBJS := $(addprefix $(obj),$(COBJS))

all: $(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

include $(SRCTREE)/rules.mk

sinclude $(obj).depend
