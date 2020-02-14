all: build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
SOURCE_DIR=Python-2.7.3
SOURCE_PACKAGE=Python-2.7.3.tar.bz2
HOST_DIR=$(CURR_DIR)/host/
HOST_COMMAND_DIR=./../host/bin/
TARGET_DIR=$(CURR_DIR)/target/


INSTALL_INCLUDE_FILE=$(SOURCE_DIR)/pyconfig.h

INSTALL_LIB_FILE= $(TARGET_DIR)/lib/python2.7/__future__.pyc \
		$(TARGET_DIR)/lib/python2.7/argparse.pyc \
		$(TARGET_DIR)/lib/python2.7/sre_constants.pyc \
		$(TARGET_DIR)/lib/python2.7/copy.pyc \
		$(TARGET_DIR)/lib/python2.7/_abcoll.pyc \
		$(TARGET_DIR)/lib/python2.7/StringIO.pyc \
		$(TARGET_DIR)/lib/python2.7/site.pyc \
		$(TARGET_DIR)/lib/python2.7/re.pyc \
		$(TARGET_DIR)/lib/python2.7/textwrap.pyc \
		$(TARGET_DIR)/lib/python2.7/hashlib.pyc \
		$(TARGET_DIR)/lib/python2.7/timeit.pyc \
		$(TARGET_DIR)/lib/python2.7/functools.pyc \
		$(TARGET_DIR)/lib/python2.7/abc.pyc \
		$(TARGET_DIR)/lib/python2.7/locale.pyc \
		$(TARGET_DIR)/lib/python2.7/tokenize.pyc \
		$(TARGET_DIR)/lib/python2.7/opcode.pyc \
		$(TARGET_DIR)/lib/python2.7/gettext.pyc \
		$(TARGET_DIR)/lib/python2.7/py_compile.pyc \
		$(TARGET_DIR)/lib/python2.7/inspect.pyc \
		$(TARGET_DIR)/lib/python2.7/getopt.pyc \
		$(TARGET_DIR)/lib/python2.7/urlparse.pyc \
		$(TARGET_DIR)/lib/python2.7/compileall.pyc \
		$(TARGET_DIR)/lib/python2.7/repr.pyc \
		$(TARGET_DIR)/lib/python2.7/httplib.pyc \
		$(TARGET_DIR)/lib/python2.7/_weakrefset.pyc \
		$(TARGET_DIR)/lib/python2.7/copy_reg.pyc \
		$(TARGET_DIR)/lib/python2.7/md5.pyc \
		$(TARGET_DIR)/lib/python2.7/threading.pyc \
		$(TARGET_DIR)/lib/python2.7/new.pyc \
		$(TARGET_DIR)/lib/python2.7/dis.pyc \
		$(TARGET_DIR)/lib/python2.7/rfc822.pyc \
		$(TARGET_DIR)/lib/python2.7/codecs.pyc \
		$(TARGET_DIR)/lib/python2.7/xmlrpclib.pyc \
		$(TARGET_DIR)/lib/python2.7/mimetools.pyc \
		$(TARGET_DIR)/lib/python2.7/struct.pyc \
		$(TARGET_DIR)/lib/python2.7/xmllib.pyc \
		$(TARGET_DIR)/lib/python2.7/sha.pyc \
		$(TARGET_DIR)/lib/python2.7/heapq.pyc \
		$(TARGET_DIR)/lib/python2.7/tempfile.pyc \
		$(TARGET_DIR)/lib/python2.7/weakref.pyc \
		$(TARGET_DIR)/lib/python2.7/urllib2.pyc \
		$(TARGET_DIR)/lib/python2.7/linecache.pyc \
		$(TARGET_DIR)/lib/python2.7/ssl.pyc \
		$(TARGET_DIR)/lib/python2.7/keyword.pyc \
		$(TARGET_DIR)/lib/python2.7/ConfigParser.pyc \
		$(TARGET_DIR)/lib/python2.7/stat.pyc \
		$(TARGET_DIR)/lib/python2.7/socket.pyc \
		$(TARGET_DIR)/lib/python2.7/stringprep.pyc \
		$(TARGET_DIR)/lib/python2.7/urllib.pyc \
		$(TARGET_DIR)/lib/python2.7/collections.pyc \
		$(TARGET_DIR)/lib/python2.7/genericpath.pyc \
		$(TARGET_DIR)/lib/python2.7/pickle.pyc \
		$(TARGET_DIR)/lib/python2.7/os.pyc \
		$(TARGET_DIR)/lib/python2.7/types.pyc \
		$(TARGET_DIR)/lib/python2.7/Queue.pyc \
		$(TARGET_DIR)/lib/python2.7/token.pyc \
		$(TARGET_DIR)/lib/python2.7/base64.pyc \
		$(TARGET_DIR)/lib/python2.7/fnmatch.pyc \
		$(TARGET_DIR)/lib/python2.7/optparse.pyc \
		$(TARGET_DIR)/lib/python2.7/string.pyc \
		$(TARGET_DIR)/lib/python2.7/pkgutil.pyc \
		$(TARGET_DIR)/lib/python2.7/sre.pyc \
		$(TARGET_DIR)/lib/python2.7/bisect.pyc \
		$(TARGET_DIR)/lib/python2.7/glob.pyc \
		$(TARGET_DIR)/lib/python2.7/warnings.pyc \
		$(TARGET_DIR)/lib/python2.7/posixpath.pyc \
		$(TARGET_DIR)/lib/python2.7/UserDict.pyc \
		$(TARGET_DIR)/lib/python2.7/sre_compile.pyc \
		$(TARGET_DIR)/lib/python2.7/popen2.pyc \
		$(TARGET_DIR)/lib/python2.7/sysconfig.pyc \
		$(TARGET_DIR)/lib/python2.7/sre_parse.pyc \
		$(TARGET_DIR)/lib/python2.7/traceback.pyc \
		$(TARGET_DIR)/lib/python2.7/random.pyc \
		$(TARGET_DIR)/lib/python2.7/subprocess.pyc

INSTALL_LIB_ENCODINGS_FILE=$(TARGET_DIR)/lib/python2.7/encodings/*.pyc

INSTALL_LIB_XML_FILE=$(TARGET_DIR)/lib/python2.7/xml/__init__.pyc

INSTALL_LIB_XML_DOM_FILE= $(TARGET_DIR)/lib/python2.7/xml/dom/minidom.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/expatbuilder.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/pulldom.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/__init__.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/domreg.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/NodeFilter.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/xmlbuilder.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/dom/minicompat.pyc

INSTALL_LIB_XML_SAX_FILE= $(TARGET_DIR)/lib/python2.7/xml/sax/saxutils.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/sax/_exceptions.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/sax/__init__.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/sax/handler.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/sax/expatreader.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/sax/xmlreader.pyc

INSTALL_LIB_XML_PARSERS_FILE= $(TARGET_DIR)/lib/python2.7/xml/parsers/__init__.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/parsers/expat.pyc

INSTALL_LIB_XML_ETREE_FILE= $(TARGET_DIR)/lib/python2.7/xml/etree/ElementTree.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/etree/ElementPath.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/etree/__init__.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/etree/ElementInclude.pyc \
		$(TARGET_DIR)/lib/python2.7/xml/etree/cElementTree.pyc 

INSTALL_LIB_DYNLOAD_FILE= $(TARGET_DIR)/lib/python2.7/lib-dynload/array.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_bisect.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_collections.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_functools.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_hashlib.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_heapq.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_random.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_socket.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_ssl.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/_struct.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/binascii.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/cStringIO.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/fcntl.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/grp.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/itertools.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/math.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/operator.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/pyexpat.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/select.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/strop.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/syslog.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/time.so \
		$(TARGET_DIR)/lib/python2.7/lib-dynload/unicodedata.so

INSTALL_LIB_CONFIG_FILE = $(TARGET_DIR)/lib/python2.7/config/Makefile

build:
	if [ ! -e .firsttime ] ; then echo MUST DO FIRSTTIME BUILD FIRST; $(MAKE) firsttime; fi
	@echo DOING UPDATE
	@echo UPDATING tags file
	make -C $(SOURCE_DIR) HOSTPYTHON=$(HOST_COMMAND_DIR)/hostpython HOSTPGEN=$(HOST_COMMAND_DIR)/Parser/hostpgen BLDSHARED=$(CC)" -shared" CROSS_COMPILE=$(TOOLCHAIN_PREFIX) CROSS_COMPILE_TARGET=yes HOSTARCH=$(ARCH)-linux BUILDARCH=`uname -m`-`uname -s`-gnu
	make -C $(SOURCE_DIR) install HOSTPYTHON=$(HOST_COMMAND_DIR)/hostpython BLDSHARED=$(CC)" -shared" CROSS_COMPILE=$(TOOLCHAIN_PREFIX) CROSS_COMPILE_TARGET=yes prefix=$(TARGET_DIR)
	mkdir -p $(INSTALL_DIR)/bin/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/lib-dynload/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/config/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/encodings/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/xml/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/xml/dom/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/xml/sax/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/xml/parsers/
	mkdir -p $(INSTALL_DIR)/lib/python2.7/xml/etree/
	mkdir -p $(INSTALL_DIR)/include/python2.7/
	install -m 755 $(SOURCE_DIR)/python $(INSTALL_DIR)/bin/
	install -m 644 $(INSTALL_LIB_FILE) $(INSTALL_DIR)/lib/python2.7/
	install -m 644 $(INSTALL_LIB_ENCODINGS_FILE) $(INSTALL_DIR)/lib/python2.7/encodings/
	install -m 755 $(INSTALL_LIB_DYNLOAD_FILE) $(INSTALL_DIR)/lib/python2.7/lib-dynload/
	install -m 644 $(INSTALL_LIB_XML_FILE) $(INSTALL_DIR)/lib/python2.7/xml/
	install -m 644 $(INSTALL_LIB_XML_DOM_FILE) $(INSTALL_DIR)/lib/python2.7/xml/dom/
	install -m 644 $(INSTALL_LIB_XML_SAX_FILE) $(INSTALL_DIR)/lib/python2.7/xml/sax/
	install -m 644 $(INSTALL_LIB_XML_PARSERS_FILE) $(INSTALL_DIR)/lib/python2.7/xml/parsers/
	install -m 644 $(INSTALL_LIB_XML_ETREE_FILE) $(INSTALL_DIR)/lib/python2.7/xml/etree/
	install -m 644 $(INSTALL_LIB_CONFIG_FILE) $(INSTALL_DIR)/lib/python2.7/config/
	install -m 644 $(INSTALL_INCLUDE_FILE) $(INSTALL_DIR)/include/python2.7/

firsttime:  clean
	echo Did first time inits >.firsttime
	@echo DONE with firsttime
	tar -xf $(SOURCE_PACKAGE) && cd $(SOURCE_DIR) && for fn in ../*.patch; do patch -p1 <$${fn}; done
	cd $(SOURCE_DIR) && ./configure CC=gcc AR=ar RANLIB=ranlib --without-cxx-main --without-threads --prefix=$(HOST_DIR)
	cd $(SOURCE_DIR) && make python Parser/pgen
	cd $(SOURCE_DIR) && make sharedmods
	cd $(SOURCE_DIR) && make install
	cd $(SOURCE_DIR) && mkdir $(HOST_COMMAND_DIR) -p && mv python $(HOST_COMMAND_DIR)/hostpython
	cd $(SOURCE_DIR) && mkdir $(HOST_COMMAND_DIR)/Parser/ -p && mv Parser/pgen $(HOST_COMMAND_DIR)/Parser/hostpgen
	cd $(SOURCE_DIR) && make distclean
	cd $(SOURCE_DIR) && ./configure --host=$(TOOLCHAIN_PREFIX) --build=$(uname -m)-$(uname -s)-gnu --prefix=/

distclean: clean

clean:
	@echo DOING CLEAN
	-rm -rf install tags .firsttime host
	-find . -name '.deps' -exec rm -rf '{}' ';'
	-find . -name '.obj' -exec rm -rf '{}' ';'
	-find . -name '*.log' -exec rm -rf '{}' ';'
	-(cd $(SOURCE_DIR) && make distclean)
	@echo All cleaned

