# $Id: Makefile.am 18136 2011-02-25 21:29:45Z rsh $
# Process this file with automake to produce Makefile.in
#
AM_CPPFLAGS = -DDATADIR=\"$(pkgdatadir)\"
#
bin_PROGRAMS = arp-scan
#
dist_bin_SCRIPTS = 
#
dist_check_SCRIPTS = check-run1 check-packet check-decode check-host-list
#
dist_man_MANS = 
#
arp_scan_SOURCES = arp-scan.c arp-scan.h error.c wrappers.c utils.c hash.c hash.h obstack.c obstack.h mt19937ar.c
arp_scan_LDADD = $(LIBOBJS)
#
dist_pkgdata_DATA = 
#
TESTS = $(dist_check_SCRIPTS)
EXTRA_DIST = pkt-simple-request.dat pkt-custom-request.dat pkt-custom-request-padding.dat pkt-custom-request-llc.dat pkt-custom-request-vlan.dat pkt-simple-response.pcap pkt-padding-response.pcap pkt-vlan-response.pcap pkt-llc-response.pcap pkt-net1921681-response.pcap pkt-trailer-response.pcap pkt-vlan-llc-response.pcap
