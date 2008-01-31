cfgdir = $(datadir)/vyatta-cfg/templates

initddir	 = /etc/init.d

AM_CPPFLAGS = -I src -Wall -DDEBUG

CLEANFILES = src/main.o src/lbdata.o src/lbdatafactory.o src/loadbalance.o src/rl_str_proc.o src/lbpathtest.o src/lboutput.o

initd_SCRIPTS = scripts/vyatta-wanloadbalance.init
sbin_SCRIPTS = scripts/vyatta-wanloadbalance.pl

src_wan_lb_SOURCES = src/main.cc
src_wan_lb_SOURCES += src/lbdata.cc
src_wan_lb_SOURCES += src/lbdatafactory.cc
src_wan_lb_SOURCES += src/loadbalance.cc
src_wan_lb_SOURCES += src/rl_str_proc.cc
src_wan_lb_SOURCES += src/lbpathtest.cc
src_wan_lb_SOURCES += src/lbdecision.cc
src_wan_lb_SOURCES += src/lboutput.cc

sbin_PROGRAMS = src/wan_lb

cpiop = find  . ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 | \
	cpio -0pd

install-exec-hook:
	mkdir -p $(DESTDIR)$(cfgdir)
	cd templates; $(cpiop) $(DESTDIR)$(cfgdir)
