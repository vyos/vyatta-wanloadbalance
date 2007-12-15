initddir	 = /etc/init.d

AM_CPPFLAGS = -I src -Wall -DDEBUG
sbin_SCRIPTS =

initd_SCRIPTS	= etc/init.d/vyatta-ofr

CLEANFILES = src/main.o src/lbdata.o src/lbdatafactory.o src/loadbalance.o src/rl_str_proc.o src/lbpathtest.o src/lboutput.o

sbin_SCRIPTS = scripts/wlb.init

sbin_PROGRAMS = src/wan_lb
src_wan_lb_SOURCES = src/main.cc
src_wan_lb_SOURCES += src/lbdata.cc
src_wan_lb_SOURCES += src/lbdatafactory.cc
src_wan_lb_SOURCES += src/loadbalance.cc
src_wan_lb_SOURCES += src/rl_str_proc.cc
src_wan_lb_SOURCES += src/lbpathtest.cc
src_wan_lb_SOURCES += src/lbdecision.cc
src_wan_lb_SOURCES += src/lboutput.cc


cpiop = find  . ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 | \
	cpio -0pd
