#
# Copyright (C) 2007-2014 Actiontec.com
#
#
EXE = arp-scan

CONFIGURE_ARGS+= \
    --host=$(TOOLCHAIN_PREFIX) \
	CC=$(CC) \
	LDFLAGS="-L$(PROFILE_DIR)/fs.build/public/lib" \
    CFLAGS="-I$(PROFILE_DIR)/public/include -I$(BUILD_DIR)/userspace/public/libs/libpcap/libpcap"

install: $(EXE)
	install -m 755 arp-scan-1.8/$(EXE) $(INSTALL_DIR)/bin/

$(EXE):
	@echo "need to do configure arp-scan"
	cd arp-scan-1.8; \
	./configure $(CONFIGURE_ARGS); \
	make


clean:
	cd arp-scan-1.8; \
	make clean
	
