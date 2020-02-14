obj-$(CONFIG_BCM_WLAN) += dhd.o 
dhd-objs += dhd_dep.o

$(obj)/dhd_dep.o:
	cp $(obj)/dhd$(PROFILE).o_save $(obj)/dhd_dep.o
	if [ ! -d $(INSTALL_DIR)/etc/wlan/dhd ]; then \
		rm -f $(INSTALL_DIR)/etc/wlan/dhd; \
		install -d $(INSTALL_DIR)/etc/wlan/dhd; \
	fi;
	for x in $(obj)/*map.bin_save; do \
		cp $$x $${x%_save}; \
		install -m 755 $${x%_save} $(INSTALL_DIR)/etc/wlan/dhd; \
	done
	for x in $(obj)/*vars.bin_save; do \
		cp $$x $${x%_save}; \
		install -m 755 $${x%_save} $(INSTALL_DIR)/etc/wlan/dhd; \
	done
obj-$(CONFIG_BCM_WLAN) += wl.o
wl-objs += wl_dep.o

$(obj)/wl_dep.o:
	cp $(obj)/wl$(PROFILE).o_save $(obj)/wl_dep.o
	if [ ! -d $(INSTALL_DIR)/etc/wlan ]; then \
		rm -f $(INSTALL_DIR)/etc/wlan; \
		install -d $(INSTALL_DIR)/etc/wlan; \
	fi;
	for x in $(obj)/*map.bin_save; do \
		cp $$x $${x%_save}; \
		install -m 755 $${x%_save} $(INSTALL_DIR)/etc/wlan; \
	done
	for x in $(obj)/*vars.bin_save; do \
		cp $$x $${x%_save}; \
		install -m 755 $${x%_save} $(INSTALL_DIR)/etc/wlan; \
	done
obj-$(CONFIG_BCM_WLAN) += wlcsm.o 
wlcsm-objs += wlcsm_dep.o

$(obj)/wlcsm_dep.o:
	cp $(obj)/wlcsm$(PROFILE).o_save $(obj)/wlcsm_dep.o
obj-$(CONFIG_BCM_WLAN) += wlemf.o 
wlemf-objs += wlemf_dep.o

$(obj)/wlemf_dep.o:
	cp $(obj)/wlemf$(PROFILE).o_save $(obj)/wlemf_dep.o
