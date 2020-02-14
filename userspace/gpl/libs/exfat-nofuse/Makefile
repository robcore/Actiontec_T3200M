#
# Makefile for Linux FAT12/FAT16/FAT32(VFAT)/FAT64(ExFAT) filesystem driver.
#
# call from kernel build system

obj-m += exfat.o

exfat-objs := exfat_core.o exfat_super.o exfat_api.o exfat_blkdev.o exfat_cache.o \
			   exfat_data.o exfat_bitmap.o exfat_nls.o exfat_oal.o exfat_upcase.o

ROMFSINST = install -m 755
INSTALL_ROOT = $(DIR_ROMFS)

EXTRA_FLAGS += -I$(PWD)

#
# KDIR is a path to a directory containing kernel source.
# It can be specified on the command line passed to make to enable the module to
# be built and installed for a kernel other than the one currently running.
# By default it is the path to the symbolic link created when
# the current kernel's modules were installed, but
# any valid path to the directory in which the target kernel's source is located
# can be provided on the command line.
#
KDIR	?= $$(pwd)/../../../../kernel/linux-3.4rt/
MDIR	?= $$(pwd)/../../../../kernel/
PWD	:= $(shell pwd)


all:
	$(MAKE) -C $(KDIR) M=$(PWD) CROSS_COMPILE=$(CROSS_COMPILE) modules ARCH=$(ARCH)
	mkdir -p $(INSTALL_DIR)/sbin
	$(ROMFSINST) exfat.ko $(INSTALL_DIR)/bin/exfat.ko
	mkdir -p $(INSTALL_DIR)/etc/init.d
	mkdir -p $(INSTALL_DIR)/etc/rc3.d
	install scripts/exfat.sh $(INSTALL_DIR)/etc/init.d
	chmod +x  $(INSTALL_DIR)/etc/init.d/exfat.sh
	(cd $(INSTALL_DIR)/etc/rc3.d; rm -f S65exfat; ln -s ../init.d/exfat.sh S65exfat)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

help:
	$(MAKE) -C $(KDIR) M=$(PWD) help

