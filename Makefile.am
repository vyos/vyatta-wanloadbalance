dhcphookdir = /etc/dhcp/dhclient-exit-hooks.d
ppphookdir = /etc/ppp/ip-up.d
pppwanlbdir = /var/run/load-balance/ppp
cfgdir = $(datadir)/vyatta-cfg/templates
opdir = $(datadir)/vyatta-op/templates
bin_sudo_usersdir = $(bindir)/sudo-users
curverdir = $(sysconfdir)/config-migrate/current

AM_CPPFLAGS = -I src -Wall -DDEBUG

CLEANFILES = src/main.o src/lbdata.o src/lbdatafactory.o src/loadbalance.o src/rl_str_proc.o src/lbpathtest.o src/lbtest_icmp.o src/lbtest_ttl.o src/lboutput.o src/lbtest.o

sbin_SCRIPTS = scripts/vyatta-wanloadbalance.init

bin_sudo_users_SCRIPTS  = scripts/vyatta-wlb-status
bin_sudo_users_SCRIPTS  += scripts/vyatta-wlb-connection.pl

dhcphook_SCRIPTS = scripts/dhcp/04-dhcp-wanlb
ppphook_SCRIPTS = scripts/ppp/04-ppp-wanlb

curver_DATA = cfg-version/wanloadbalance@3

src_wan_lb_SOURCES = src/main.cc
src_wan_lb_SOURCES += src/lbdata.cc
src_wan_lb_SOURCES += src/lbdatafactory.cc
src_wan_lb_SOURCES += src/loadbalance.cc
src_wan_lb_SOURCES += src/rl_str_proc.cc
src_wan_lb_SOURCES += src/lbpathtest.cc
src_wan_lb_SOURCES += src/lbtest_icmp.cc
src_wan_lb_SOURCES += src/lbtest_ttl.cc
src_wan_lb_SOURCES += src/lbtest_user.cc
src_wan_lb_SOURCES += src/lbdecision.cc
src_wan_lb_SOURCES += src/lboutput.cc
src_wan_lb_SOURCES += src/lbtest.cc

sbin_PROGRAMS = src/wan_lb

cpiop = find  . ! -regex '\(.*~\|.*\.bak\|.*\.swp\|.*\#.*\#\)' -print0 | \
	cpio -0pd

install-exec-hook:
	mkdir -p $(DESTDIR)$(dhcphookdir)
	mkdir -p $(DESTDIR)$(pppwanlbdir)
	mkdir -p $(DESTDIR)$(ppphookdir)
	mkdir -p $(DESTDIR)$(opdir)
	cd templates-op; $(cpiop) $(DESTDIR)$(opdir)
