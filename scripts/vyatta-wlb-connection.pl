#!/usr/bin/perl
#
# Module: vyatta-show-wlb-connection.pl
#
# **** License ****
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2007 Vyatta, Inc.
# All Rights Reserved.
#
# Author: Michael Larson
# Date: December 2008
# Description: Script to display wlb connection information
#
# **** End License ****
#

use lib "/opt/vyatta/share/perl5/";

#examine /var/run/load-balance/wlb.conf for disable-source-nat
if (!open($CONFFILE, "<", "/var/run/load-balance/wlb.conf")) {
    return;
}
$nat_source_disabled = 0;
$_ = <$CONFFILE>;
if (/disable-source-nat/) {
    $nat_source_disabled = 1;
    if (!open($FILE, "<", "/proc/net/nf_conntrack")) {
        return;
    }
} else {
    if (!open($FILE, "/usr/sbin/conntrack -L -n|")) {
        return;
    }
}

print "Type\tState\t\tSrc\t\t\tDst\t\t\tPackets\tBytes\n";

@line = <$FILE>;
foreach (@line) {
    $_ =~ s/\[\S+\]\s//;

    my $proto,$tmp,$state,$src,$dst,$sport,$dport,$pkts,$bytes;

    if($nat_source_disabled){
        if (/tcp/) {
            ($tmp,$tmp,$proto,$tmp,$tmp,$state,$src,$dst,$sport,$dport,$pkts,$bytes) = split(' ',$_);
        } elsif (/udp/) {
            $state = "";
            ($tmp,$tmp,$proto,$tmp,$tmp,$src,$dst,$sport,$dport,$pkts,$bytes) = split(' ',$_);
        }
    } else {
         if (/tcp/) {
            ($proto,$tmp,$tmp,$state,$src,$dst,$sport,$dport,$pkts,$bytes) = split(' ',$_);
        } elsif (/udp/) {
            $state = "";
            ($proto,$tmp,$tmp,$src,$dst,$sport,$dport,$pkts,$bytes) = split(' ',$_);
        }
    }

    ($tmp,$src) = split('=',$src);
    ($tmp,$dst) = split('=',$dst);
    ($tmp,$dport) = split('=',$dport);
    ($tmp,$sport) = split('=',$sport);
    ($tmp,$pkts) = split('=',$pkts);
    ($tmp,$bytes) = split('=',$bytes);

    my $snet = sprintf("%s:%s%-10s",$src,$sport);
    $snet = substr($snet,0,18);

    my $dnet = sprintf("%s:%s%-10s",$dst,$dport);
    $dnet = substr($dnet,0,18);

    $state = sprintf("%s%-12s",$state);
    $state = substr($state,0,12);

    #mark=[1-9]
    if (/ mark=[1-9]/) {
        print "$proto\t$state\t$snet\t$dnet\t$pkts\t$bytes\n";
    }
}

#now dump out results
