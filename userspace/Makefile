#***********************************************************************
#
#  Copyright (c) 2007  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

#
# You probably don't need to modify anything in this Makefile.
#

LIBS_BASE:=public-libs-base private-libs-base gpl-libs-base
LIBS_LAYER1:=public-libs-layer1 private-libs-layer1 gpl-libs-layer1

APPS:=public-apps-base private-apps-base gpl-apps-base cs-apps

CS_LIBS:=cs-libs

# === This is the default target ===
ifneq ($(strip $(AEI_CONTROL_LAYER)),)
all: ctl_layer $(APPS)
ctl_layer: private-libs-base private-libs-layer1 public-libs-base public-libs-layer1

ifneq ($(strip $(GPL_CODE)),)
else
endif

else
all : $(APPS)
endif


# Build the LIBS in two passes.  First build LIBS_BASE, which are the libs
# which do not depend on any other libs or their header files.
# Libs with empty .autodetect and hardcoded libs are in this category.
# LIBS_LAYER1 are libs which depend on LIBS_BASE.  Their .autodetect file
# must contain a line which begins with the word "Layer1".
# Additional layers can be defined if necessary.
# APPS are built in a single pass, after all LIBS have been built.

$(LIBS_BASE) : public-include

$(LIBS_LAYER1) : $(LIBS_BASE)

$(CS_LIBS) : $(LIBS_LAYER1)

all_libs : $(CS_LIBS)

$(APPS) : all_libs



# autogenerate version header file
public-include:
	$(MAKE) -C public/include

# LIBS_BASE targets
private-libs-base:

public-libs-base:
	$(MAKE) -C public/libs base

gpl-libs-base:
	$(MAKE) -C gpl/libs base

# LIBS_LAYER1 targets
private-libs-layer1:

public-libs-layer1:
	$(MAKE) -C public/libs layer1

gpl-libs-layer1:
	$(MAKE) -C gpl/libs layer1

# APPS targets
gpl-apps-base:
	$(MAKE) -C gpl/apps base

public-apps-base:
	$(MAKE) -C public/apps base

private-apps-base:

cs-libs:
ifeq ($(AEI_SUPPORT_CLOUD),1)
	$(MAKE) -C $(CLOUD_SERVICE_DIR)/lib \
		ROOT_PATH=$(CLOUD_SERVICE_DIR) LIBPATH_MK=$(CLOUD_SERVICE_DIR)/LIBPATH.mk \
		ROOT_FS=$(INSTALL_DIR)
	$(MAKE) -C $(CLOUD_SERVICE_DIR)/lib \
		ROOT_PATH=$(CLOUD_SERVICE_DIR) LIBPATH_MK=$(CLOUD_SERVICE_DIR)/LIBPATH.mk \
		ROOT_FS=$(INSTALL_DIR) install
else
	echo "Don't support cloud service libs"
endif

cs-apps:
ifeq ($(AEI_SUPPORT_CLOUD),1)
	$(MAKE) -C $(CLOUD_SERVICE_DIR) touchBuildVersion
	$(MAKE) -C $(CLOUD_SERVICE_DIR)/app \
		ROOT_PATH=$(CLOUD_SERVICE_DIR) LIBPATH_MK=$(CLOUD_SERVICE_DIR)/LIBPATH.mk \
		ROOT_FS=$(INSTALL_DIR)
	$(MAKE) -C $(CLOUD_SERVICE_DIR)/app \
		ROOT_PATH=$(CLOUD_SERVICE_DIR) LIBPATH_MK=$(CLOUD_SERVICE_DIR)/LIBPATH.mk \
		ROOT_FS=$(INSTALL_DIR) install
else
	echo "Don't support cloud service apps"
endif

#
# Cleaning rules
#
clean: 
	@for dir in public private gpl; do \
		for apporlib in libs apps; do \
			$(MAKE) -C $$dir/$$apporlib clean; \
		done \
	done
	@$(MAKE) -C public/include clean

