
openssl: conditional_build 

.PHONY: all distclean clean conditional_build openssl

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

UNTAR_COMPLETE_FILE = .untar_complete
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)

ifneq ($(strip $(AEI_OPENSSL_1_1_0E)),)
conditional_build: all
else
conditional_build:
	@echo "skipping openssl 1.1.0e (not configured)"
endif

#clear CC to avoid conflict with openssl configuration
CC=

# APP must match the openssl folder name
APP=openssl-1.1.0e

SHLIBCMD= grep -Po '(?<=SHLIB_VERSION_NUMBER=).*' $(APP)/Makefile

# the options are ordered for the configure script
ifeq ($(strip $(PROFILE_ARCH)),arm)
SSLOPTIONS := linux-armv4
else
SSLOPTIONS := linux-mips32
endif
SSLOPTIONS += --prefix=/usr/local/ssl --openssldir=/usr/local/ssl

AR=
NM=
SSLOPTIONS += --cross-compile-prefix=$(CROSS_COMPILE)
SSLOPTIONS += no-hw shared no-md4 no-mdc2 no-rc2 no-idea no-unit-test no-cast no-bf no-engine no-async no-asm

ifneq ($(strip $(GPL_CODE)),)
SSLOPTIONS += $(ACTION_TEC_DFLAGS)
endif

ifeq ($(strip $(ARCH_ENDIAN)),little)
SSLOPTIONS += -DL_ENDIAN
else
SSLOPTIONS += -DB_ENDIAN 
endif
ifneq ($(strip $(BUILD_OCF_LINUX)),)
SSLOPTIONS += -DHAVE_CRYPTODEV -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
endif

SSLOPTIONS += $(BRCM_COMMON_CFLAGS) $(SSP_TYP_COMPILER_OPTS) $(SSP_TYP_LIBS) $(SSP_LIB_INC)

$(APP)/$(UNTAR_COMPLETE_FILE): $(APP).tar.gz
	rm -rf $(APP)
	tar xzf $(APP).tar.gz
	@echo "$(APP) is untarred"
	-[ -e $(APP)/Makefile ] && $(MAKE) -j 1 -C $(APP) dclean
	touch $(APP)/$(UNTAR_COMPLETE_FILE)

$(APP)/$(CONFIG_ID_FILE): $(APP)/$(UNTAR_COMPLETE_FILE)
	@echo "$(APP) is being configured"
	(cd $(APP); ./Configure $(SSLOPTIONS);)
	$(MAKE) -j 1 -C $(APP) depend
	rm -f $(APP)/BRCM_CONFIG_ID_*
	touch $(APP)/$(CONFIG_ID_FILE)

all: $(APP)/$(CONFIG_ID_FILE)
	$(MAKE) -j 1 -C $(APP) all
ifneq ($(strip $(BUILD_IPSEC_TOOLS)$(BUILD_CERT)),)
	mkdir -p $(INSTALL_DIR)/bin
	install -m 755 $(APP)/apps/openssl $(INSTALL_DIR)/bin/$(APP)
endif
	mkdir -p $(INSTALL_DIR)/lib
	mkdir -p $(BCM_FSBUILD_DIR)/lib
	@SHLIB_VERSION_NUMBER=$$(grep -Po '(?<=SHLIB_VERSION_NUMBER=).*' ./$(APP)/Makefile ); \
	echo "install -m 755 $(APP)/libssl.so.$$SHLIB_VERSION_NUMBER $(INSTALL_DIR)/lib"; \
	install -m 755 $(APP)/libssl.so.$$SHLIB_VERSION_NUMBER $(INSTALL_DIR)/lib; \
	cd $(INSTALL_DIR)/lib; rm -f libssl-1-1.so; ln -s libssl.so.$$SHLIB_VERSION_NUMBER libssl-1-1.so; cd - > /dev/null; \
	echo "install -m 755 $(APP)/libcrypto.so.$$SHLIB_VERSION_NUMBER $(INSTALL_DIR)/lib"; \
	install -m 755 $(APP)/libcrypto.so.$$SHLIB_VERSION_NUMBER $(INSTALL_DIR)/lib; \
	cd $(INSTALL_DIR)/lib; rm -f libcrypto-1-1.so; ln -s libcrypto.so.$$SHLIB_VERSION_NUMBER libcrypto-1-1.so; cd - > /dev/null; \
	echo "cp -u $(APP)/libssl.so.$$SHLIB_VERSION_NUMBER $(BCM_FSBUILD_DIR)/lib"; \
	cp -u $(APP)/libssl.so.$$SHLIB_VERSION_NUMBER $(BCM_FSBUILD_DIR)/lib/; \
	cd $(BCM_FSBUILD_DIR)/lib; rm -f libssl-1-1.so; ln -s libssl.so.$$SHLIB_VERSION_NUMBER libssl-1-1.so; cd - > /dev/null; \
	echo "cp -u $(APP)/libcrypto.so.$$SHLIB_VERSION_NUMBER $(BCM_FSBUILD_DIR)/lib"; \
	cp -u $(APP)/libcrypto.so.$$SHLIB_VERSION_NUMBER $(BCM_FSBUILD_DIR)/lib; \
	cd $(BCM_FSBUILD_DIR)/lib; rm -f libcrypto-1-1.so; ln -s libcrypto.so.$$SHLIB_VERSION_NUMBER libcrypto-1-1.so;
	mkdir -p $(BCM_FSBUILD_DIR)/public/include/$(APP)/openssl
	cp -u $(APP)/include/openssl/* $(BCM_FSBUILD_DIR)/public/include/$(APP)/openssl

clean:
	rm -rf $(APP)
	rm -f $(INSTALL_DIR)/lib/libssl-1-1.so*
	rm -f $(INSTALL_DIR)/lib/libcrypto-1-1.so*
	rm -f $(INSTALL_DIR)/bin/openssl-$(APP)
	rm -f $(BCM_FSBUILD_DIR)/lib/libssl-1-1.so*
	rm -f $(BCM_FSBUILD_DIR)/lib/libcrypto-1-1.so*
	rm -f $(BCM_FSBUILD_DIR)/public/include/$(APP)/openssl/*

distclean: clean

shell:
	bash -i

