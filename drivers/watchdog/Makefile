#
# (C) Copyright 2008
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libwatchdog.o

COBJS-$(CONFIG_AT91SAM9_WATCHDOG) += at91sam9_wdt.o
COBJS-$(CONFIG_FTWDT010_WATCHDOG) += ftwdt010_wdt.o
ifneq (,$(filter $(SOC), mx31 mx35 mx5 mx6 vf610))
COBJS-y += imx_watchdog.o
endif
COBJS-$(CONFIG_TNETV107X_WATCHDOG) += tnetv107x_wdt.o
COBJS-$(CONFIG_S5P)               += s5p_wdt.o
COBJS-$(CONFIG_XILINX_TB_WATCHDOG) += xilinx_tb_wdt.o
COBJS-$(CONFIG_BFIN_WATCHDOG)  += bfin_wdt.o
ifdef CONFIG_JZ_WATCHDOG
ifeq ($(CONFIG_X2600),y)
COBJS-y += x2600_wdt.o
else ifeq ($(CONFIG_AD100),y)
COBJS-y += ad100_wdt.o
else ifeq ($(CONFIG_X1600),y)
COBJS-y += jz_wdt.o
endif
endif
COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
