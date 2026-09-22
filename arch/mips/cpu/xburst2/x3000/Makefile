#
# Ingenic X3000 SOC Makefile
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(SOC).o

COBJS-y	= soc.o
COBJS-y += clk.o
COBJS-y += pllsetting.o
#COBJS-y += ddr_set_dll.o

ifdef CONFIG_DDR_X3000_DWC_INNOPHY_DDR4
$(shell mkdir -p $(obj)ddr)
COBJS-y += ddr/ddr.o
COBJS-y += ddr/ddrc.o
COBJS-y += ddr/ddrp.o
COBJS-$(CONFIG_DDR_DEBUG_STUB) += ddr/debug_stub.o
endif

COBJS-$(CONFIG_DDR_TEST) += ../ddr_test.o
COBJS-$(CONFIG_DDR_AUTO_REFRESH_TEST) += ../ddr_test_auto_refresh.o
COBJS-$(CONFIG_BURNER) += ../param_manage.o

SRCS	:= $(START:.o=.S) $(SOBJS-y:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS-y) $(COBJS-y))
START	:= $(addprefix $(obj),$(START))

all:	$(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(call cmd_link_o_target, $(OBJS))

HOSTCPPFLAGS =	-include $(SRCTREE)/include/libfdt_env.h \
		-idirafter $(SRCTREE)/include \
		-idirafter $(OBJTREE)/include2 \
		-idirafter $(OBJTREE)/include \
		-I $(SRCTREE)/lib/libfdt \
		-I $(SRCTREE)/tools \
		-DCONFIG_SYS_TEXT_BASE=$(CONFIG_SYS_TEXT_BASE) \
		-DUSE_HOSTCC \
		-D__KERNEL_STRICT_NAMES

$(obj).depend.pllsetting:$(OBJTREE)/include/generated/pll_reg_values.h
$(obj)pllsetting.o:$(OBJTREE)/include/generated/pll_reg_values.h

$(OBJTREE)/include/generated/pll_reg_values.h:$(OBJTREE)/$(dir)/pll_params_creator.exe
	$< > $@

$(OBJTREE)/$(dir)/pll_params_creator.exe:$(SRCTREE)/$(dir)/pll_params_creator.c | $(SRCTREE)/include/configs/$(BOARD).h
	$(HOSTCC) $(HOSTCFLAGS_NOPED) $(HOSTLDFLAGS) $(HOSTCPPFLAGS) -o $@ $^

$(obj).depend.clk:$(OBJTREE)/include/generated/clk_reg_values.h
$(obj)clk.o:$(OBJTREE)/include/generated/clk_reg_values.h

$(OBJTREE)/include/generated/clk_reg_values.h:$(OBJTREE)/$(dir)/clk_params_creator.exe
	$< > $@

$(OBJTREE)/$(dir)/clk_params_creator.exe:$(SRCTREE)/$(dir)/clk_params_creator.c | $(SRCTREE)/include/configs/$(BOARD).h
	$(HOSTCC) $(HOSTCFLAGS_NOPED) $(HOSTLDFLAGS) $(HOSTCPPFLAGS) -o $@ $^

include $(SRCTREE)/rules.mk

sinclude $(obj).depend
