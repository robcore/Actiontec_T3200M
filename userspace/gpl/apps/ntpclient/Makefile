# ntpd makefile

LDFLAGS = -Wl,--allow-shlib-undefined

prefix=/usr
SBINDIR=/sbin
USRSBINDIR=${prefix}/sbin
USRBINDIR=${prefix}/bin
USRSHAREDIR=${prefix}/share

VER := 1.20.2

OBJS_SHARED = bb_strtonum.o default_error_retval.o full_write.o getopt32.o llist.o messages.o perror_msg.o ptr_to_globals.o safe_strncpy.o safe_write.o signals.o udp_io.o verror_msg.o vfork_daemon_rexec.o xatonum.o xconnect.o xfunc_die.o xfuncs.o xfuncs_printf.o dst.o
NTPD_OBJS = ntpd.o

EXEC1 = ntpclient
OBJS1 = $(NTPD_OBJS) $(OBJS_SHARED)

CFLAGS += -s -W -Wall -Wstrict-prototypes -DVERSION='"$(VER)"' -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
CFLAGS += -DBRCM_CMS_BUILD -DCMS_LOG3 -I$(BUILD_DIR)/userspace/public/include -I$(BUILD_DIR)/userspace/public/include/linux
CFLAGS += -I$(TOOLCHAIN)/include -I$(BUILD_DIR)/kernel/linux/include

LIBS=-Wl,-rpath,/lib:/lib/public:$(INSTALL_DIR)/lib:$(INSTALL_DIR)/lib/public -L$(INSTALL_DIR)/lib/public -lcms_util -lcms_msg


CFLAGS += -D_GNU_SOURCE -D"BB_VER=KBUILD_STR(1.20.2)" -D"KBUILD_STR(s)=\#s"
CFLAGS += -Os -fomit-frame-pointer -fno-strict-aliasing
CFLAGS += $(BRCM_WERROR_CFLAGS)
CFLAGS +=$(ACTION_TEC_DFLAGS)
CFLAGS += -Werror  -Wfatal-errors

all: $(EXEC1)

install: $(EXEC1)
	install -m 755 $(EXEC1) $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/ntpclient

.c.o:
	$(CC) -c $(CFLAGS) $<

$(EXEC1): $(OBJS1)
	$(CC) $(LDFLAGS) $(OBJS1) -o $(EXEC1) $(LIBS)

dynamic: all install

clean:
	-rm -f ntpclient *.o core
