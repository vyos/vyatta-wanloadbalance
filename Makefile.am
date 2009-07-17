dhcphookdir = /etc/dhcp3/dhclient-exit-hooks.d
cfgdir = $(datadir)/vyatta-cfg/templates
bin_sudo_usersdir = $(bindir)/sudo-users
curverdir = $(sysconfdir)/config-migrate/current

initddir	 = /etc/init.d
etcdir           = /etc

AM_CPPFLAGS = -I src -Wall -DDEBUG

CLEANFILES = src/main.o src/lbdata.o src/lbdatafactory.o src/loadbalance.o src/rl_str_proc.o src/lbpathtest.o src/lbtest_icmp.o src/lboutput.o

sbin_SCRIPTS = scripts/vyatta-wanloadbalance.pl
sbin_SCRIPTS += scripts/vyatta-wanloadbalance.init

bin_sudo_users_SCRIPTS  = scripts/vyatta-wlb-status
bin_sudo_users_SCRIPTS  += scripts/vyatta-wlb-connection.pl

dhcphook_SCRIPTS = scripts/dhcp/dhcp-wanlb

curver_DATA = cfg-version/wanloadbalance@1

src_wan_lb_SOURCES = src/main.cc
src_wan_lb_SOURCES += src/lbdata.cc
src_wan_lb_SOURCES += src/lbdatafactory.cc
src_wan_lb_SOURCES += src/loadbalance.cc
src_wan_lb_SOURCES += src/rl_str_proc.cc
src_wan_lb_SOURCES += src/lbpathtest.cc
src_wan_lb_SOURCES += src/lbtest_icmp.cc
src_wan_lb_SOURCES += src/lbdecision.cc
src_wan_lb_SOURCES += src/lboutput.cc

sbin_PROGRAMS = src/wan_lb

cpiop = find  . ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 | \
	cpio -0pd

install-exec-hook:
	mkdir -p $(DESTDIR)$(dhcphookdir)
	mkdir -p $(DESTDIR)$(cfgdir)
	cd templates; $(cpiop) $(DESTDIR)$(cfgdir)
	find etc ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 |  cpio -0pd $(DESTDIR)