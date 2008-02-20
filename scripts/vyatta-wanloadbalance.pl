#!/usr/bin/perl -w
#
# Module: vyatta-wanloadbalance.pl
# 
# **** License ****
# Version: VPL 1.0
# 
# The contents of this file are subject to the Vyatta Public License
# Version 1.0 ("License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.vyatta.com/vpl
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
# 
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2008 Vyatta, Inc.
# All Rights Reserved.
# 
# Author: Michael Larson
# Date: January 2008
# Description: Writes exclusion list for linkstatus
# 
# **** End License ****
#
use lib "/opt/vyatta/share/perl5/";
use VyattaConfig;
use VyattaMisc;

use warnings;
use strict;
use POSIX;
use File::Copy;

sub write_health {
#open conf
    my $config = new VyattaConfig;

    $config->setLevel("load-balancing wan interface-health");
    my @eths = $config->listNodes();
    
    print FILE_LCK "health {\n";

    foreach my $ethNode (@eths) {
	
	print FILE_LCK "\tinterface " . $ethNode . " {\n";
	
	my $option = $config->returnValue("$ethNode failure-count");
	if (defined $option) {
	    print FILE_LCK "\t\tfailure-ct  " . $option . "\n";
	}
	
	$option = $config->returnValue("$ethNode ping");
	if (defined $option) {
	    print FILE_LCK "\t\ttarget  " . $option . "\n";
	}
	
	$option = $config->returnValue("$ethNode resp-time");
	if (defined $option) {
	    print FILE_LCK "\t\tping-resp  " . $option . "\n";
	}
	
	$option = $config->returnValue("$ethNode success-count");
	if (defined $option) {
	    print FILE_LCK "\t\tsuccess-ct  " . $option . "\n";
	}
	print FILE_LCK "\t}\n";
    }
    print FILE_LCK "}\n\n";
}

sub write_rules {
    my $config = new VyattaConfig;

    $config->setLevel('load-balancing wan rule');
    my @rules = $config->listNodes();
    
    #destination
    foreach my $rule (@rules) {
	print FILE_LCK "rule " . $rule . " {\n";
	
	$config->setLevel('load-balancing wan rule');

	my $protocol = $config->returnValue("$rule protocol");
	if (defined $protocol) {
	    print FILE_LCK "\tprotocol " . $protocol . "\n"
	}
	else {
	    $protocol = "";
	}

	#destination
	print FILE_LCK "\tdestination {\n";
	my $option = $config->returnValue("$rule destination address");
	if (defined $option) {
	    print FILE_LCK "\t\taddress " . $option . "\n";
	}

	$option = $config->returnValue("$rule destination network");
	if (defined $option) {
	    print FILE_LCK "\t\tnetwork " . $option . "\n";
	}

	$option = $config->returnValue("$rule destination port");
	if (defined $option) {
	    if ($protocol ne "tcp" && $protocol ne "udp") {
		print "Please specify protocol tcp or udp when configuring ports\n";
		exit 2;
	    }
	    print FILE_LCK "\t\tport " . $option . "\n";
	}

	print FILE_LCK "\t}\n";

	#source
	$config->setLevel('load-balancing wan rule');


	print FILE_LCK "\tsource {\n";
	$option = $config->returnValue("$rule source address");
	if (defined $option) {
	    print FILE_LCK "\t\taddress " . $option . "\n";
	}

	$option = $config->returnValue("$rule source network");
	if (defined $option) {
	    print FILE_LCK "\t\tnetwork " . $option . "\n";
	}

	$option = $config->returnValue("$rule source port");
	if (defined $option) {
	    if ($protocol ne "tcp" && $protocol ne "udp") {
		print "Please specify protocol tcp or udp when configuring ports\n";
		exit 2;
	    }
	    print FILE_LCK "\t\tport " . $option . "\n";
	}
	print FILE_LCK "\t}\n";

	#interface
	$config->setLevel("load-balancing wan rule $rule interface");
	my @eths = $config->listNodes();
	
	foreach my $ethNode (@eths) {
	    print FILE_LCK "\tinterface " . $ethNode . " {\n";
	    
	    $option = $config->returnValue("$ethNode weight");
	    if (defined $option) {
		print FILE_LCK "\t\tweight  " . $option . "\n";
	    }
	    print FILE_LCK "\t}\n";
	}
	print FILE_LCK "}\n";
    }
}



####main
my $conf_file = '/var/load-balance/wlb.conf';
my $conf_lck_file = '/var/load-balance/wlb.conf.lck';

#open file
open(FILE, "<$conf_file") or die "Can't open wlb config file"; 
open(FILE_LCK, "+>$conf_lck_file") or die "Can't open wlb lock file";

write_health();

write_rules();

close FILE;
close FILE_LCK;

copy ($conf_lck_file,$conf_file);
unlink($conf_lck_file);


#finally kick the process
system "/opt/vyatta/sbin/vyatta-wanloadbalance.init restart $conf_file";

exit 0;
