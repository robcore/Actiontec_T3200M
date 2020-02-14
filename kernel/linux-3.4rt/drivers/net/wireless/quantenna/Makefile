#
# Makefile for Quantenna RC paltform
#
#

EXTRA_CFLAGS    += -Wall  \
		   -I$(src)/host \
		   -I$(src)/include \
		   -I$(src)/host/common \
		   -I$(src)/host/st \
		   -I../../shared/opensource/include/bcm963xx \
		   -Iarch/arm/mach-bcm963xx/include

EXTRA_CFLAGS    += -mlong-calls -DTOPAZ_PLATFORM -DGPL_CODE -DCONFIG_FW_LOADER

ifeq (${PCIE_HOST_CRUMBS},1)
EXTRA_CFLAGS += -finstrument-functions
endif

#EXTRA_CFLAGS	+= -DDEBUG
#AEI_SKB_POOL defined in bcmcpe2/customers/BELLALIANT/R3000H/R3000H_specific/DEFINES
EXTRA_CFLAGS	+= -DAEI_MPS128
#EXTRA_CFLAGS	+= -DAEI_CPU_CACHE_OPTIMIZATION

ifneq ($(KERNELRELEASE),)
COMMON_DIR	:= host/common
qdpc-host-objs   := $(COMMON_DIR)/topaz_vnet.o $(COMMON_DIR)/qdpc_init.o $(COMMON_DIR)/qdpc_pcie.o host/st/qdpc_platform.o

obj-m           :=  qdpc-host.o

else

KERNELDIR	?= ../../../..
INSTALL		= INSTALL_MOD_PATH=../linux/modules
CROSS		= ARCH=arm CROSS_COMPILE=/opt/toolchains/crosstools-arm-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-NPTL/usr/bin/arm-linux-
PWD		:= $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) $(CROSS) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) $(CROSS) $(INSTALL) M=$(PWD) modules_install

endif

clean:
	rm -rf *.o  *~  core  .depend  .*.cmd  *.ko  *.mod.c  .tmp_versions  Module.symvers  modules.order
	rm -f  host/*/*.o
	rm -rf $(COMMON_DIR)/.*.cmd $(COMMON_DIR)/.tmp_versions $(COMMON_DIR)/*.o

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

