include $(TOPDIR)/config.mk

LIB := $(obj)libverity.o

COBJS-$(CONFIG_DM_VERITY) += dm_verity_runtime.o
COBJS-$(CONFIG_DM_VERITY) += dm_verity_media.o
COBJS-$(CONFIG_DM_VERITY) += dm_verity_manifest_source.o
COBJS-$(CONFIG_DM_VERITY) += dm_verity_pubkey_source.o
COBJS-$(CONFIG_DM_VERITY) += dm_verity_boot.o
COBJS-$(CONFIG_SECURITY_PUBKEY) += security_pubkey.o

COBJS := $(COBJS-y)
SRCS := $(COBJS:.o=.c)
OBJS := $(addprefix $(obj),$(COBJS))

all: $(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

include $(SRCTREE)/rules.mk

sinclude $(obj).depend
