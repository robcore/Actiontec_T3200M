
libcap: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)


ifneq ($(strip $(BUILD_LIBCAP)),)
conditional_build: all
else
conditional_build:
	@echo "skipping libcap (not configured)"
endif


check_untar:
ifeq ($(wildcard libcap/untarred),)
	mkdir -p libcap
	cd libcap ; tar --strip-components=1 -xkzf ../libcap.tar.gz
	touch libcap/untarred
endif
	@echo "libcap is untarred"

check_config: check_untar
ifeq ($(wildcard libcap/$(CONFIG_ID_FILE)),)
	-[ -e libcap/Makefile ] && cd libcap  && make distclean
	rm -f libcap/BRCM_CONFIG_ID_*
	touch libcap/$(CONFIG_ID_FILE)
endif

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	cp -f Make.Rules libcap/
	cd libcap ; make LIBATTR=no lib=lib prefix=$(BCM_FSBUILD_DIR)/public cross_compile=/opt/toolchains/crosstools-arm-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-NPTL/usr/bin/arm-unknown-linux-uclibcgnueabi-
	cd libcap ; make LIBATTR=no lib=lib prefix=$(BCM_FSBUILD_DIR)/public cross_compile=/opt/toolchains/crosstools-arm-gcc-4.6-linux-3.4-uclibc-0.9.32-binutils-2.21-NPTL/usr/bin/arm-unknown-linux-uclibcgnueabi- install
	cp -d libcap/libcap/libcap.so* $(INSTALL_DIR)/lib/public/

clean:
	rm -f $(INSTALL_DIR)/lib/public/libcap.so*
	-[ -e libcap/Makefile ] && cd libcap && make clean

bcm_dorel_distclean: distclean

distclean: clean
	rm -f libcap

shell:
	bash -i

