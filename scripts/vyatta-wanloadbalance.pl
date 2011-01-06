#!/usr/bin/perl -w
#
# Module: vyatta-wanloadbalance.pl
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation.
# 
# **** End License ****
#
use lib "/opt/vyatta/share/perl5/";
use Vyatta::Config;
use Vyatta::Misc;
use Vyatta::TypeChecker;
use Getopt::Long;

use warnings;
use strict;
use POSIX;
use File::Copy;

sub write_health {
#open conf
    my $config = new Vyatta::Config;

    my $valid = "false";

    if ($config->exists("load-balancing wan disable-source-nat")) {
	print FILE_LCK "disable-source-nat\n";
    }

    if ($config->exists("load-balancing wan enable-local-traffic")) {
	print FILE_LCK "enable-local-traffic\n";
    }

    if ($config->exists("load-balancing wan flush-connections")) {
	print FILE_LCK "flush-conntrack\n";
    }


    my $hook = $config->returnValue("load-balancing wan hook");
    if (defined $hook) {
	print FILE_LCK "hook \"" . $hook . "\"\n";
    }

    print FILE_LCK "health {\n";

    $config->setLevel("load-balancing wan interface-health");
    my @eths = $config->listNodes();
    foreach my $ethNode (@eths) {
	$config->setLevel("load-balancing wan interface-health");
	
	print FILE_LCK "\tinterface " . $ethNode . " {\n";
	
	my $option = $config->returnValue("$ethNode failure-count");
	if (defined $option) {
	    print FILE_LCK "\t\tfailure-ct  " . $option . "\n";
	}

	$option = $config->returnValue("$ethNode success-count");
	if (defined $option) {
	    print FILE_LCK "\t\tsuccess-ct  " . $option . "\n";
	}

	$option = $config->returnValue("$ethNode nexthop");
	if (defined $option) {
	    print FILE_LCK "\t\tnexthop " . $option . "\n";
	    $valid = "true";
	}
	else {
	    print "ERROR: nexthop must be specified\n";
	    exit 1;
	}
	
	$config->setLevel("load-balancing wan interface-health $ethNode test");
	my @rules = $config->listNodes();
	foreach my $rule (@rules) {
	    print FILE_LCK "\t\trule " . $rule . " {\n";
	    
	    my $test_type = $config->returnValue("$rule type");
	    if ((defined $test_type) && ($test_type eq "ttl")) {
		print FILE_LCK "\t\t\ttype udp {\n";
		my $ttl_limit = $config->returnValue("$rule ttl-limit");
		if (defined $ttl_limit) {
		    print FILE_LCK "\t\t\t\tttl $ttl_limit\n";
		}
	    }
	    else {
		print FILE_LCK "\t\t\ttype ping {\n";
	    }
	    
	    my $option = $config->returnValue("$rule target");
	    if (defined $option) {
		print FILE_LCK "\t\t\t\ttarget  " . $option . "\n";
	    }
	    
	    $option = $config->returnValue("$rule resp-time");
	    if (defined $option) {
		print FILE_LCK "\t\t\t\tresp-time  " . $option*1000 . "\n";
	    }
	    print FILE_LCK "\t\t\t}\n";
	    
	    print FILE_LCK "\t\t}\n";
	}
	print FILE_LCK "\t}\n";
    }
    print FILE_LCK "}\n\n";
    
    if ($valid eq "false") {
	print "WARNING: A valid WAN load-balance configuration requires an interface with a nexthop\n";
    }
    return $valid;
}

sub write_rules {
    my $config = new Vyatta::Config;

    my $outbound_defined = "false";

    $config->setLevel('load-balancing wan rule');
    my @rules = $config->listNodes();
    
    #destination
    foreach my $rule (@rules) {
	print FILE_LCK "rule " . $rule . " {\n";
	
	my $exclude = "false";
	
	$config->setLevel('load-balancing wan rule');

	if ($config->exists("$rule exclude")) {
	    $exclude = "true";
	    print FILE_LCK "\texclude\n";
	}

	if ($config->exists("$rule failover")) {
	    print FILE_LCK "\tfailover\n";
	}

	if ($config->exists("$rule enable-source-based-routing")) {
	    print FILE_LCK "\tenable-source-based-routing\n";
	}

	if ($config->exists("$rule failover") && $config->exists("$rule exclude")) {
	    print "ERROR: failover cannot be configured with exclude\n";
	    exit 1;
	}

	if ($config->exists("$rule limit") && $config->exists("$rule exclude")) {
	    print "ERROR: limit cannot be used with exclude\n";
	    exit 1;
	}

	if ($config->exists("$rule limit")) {
	    print FILE_LCK "\tlimit {\n";
	    my $limit_burst = $config->returnValue("$rule limit burst");
	    if (defined $limit_burst) {
		print FILE_LCK "\t\tburst " . $limit_burst . "\n";
	    }
	    
	    my $limit_rate = $config->returnValue("$rule limit rate");
	    if (defined $limit_rate) {
		print FILE_LCK "\t\trate " . $limit_rate . "\n";
	    }
	    
	    my $limit_period = $config->returnValue("$rule limit period");
	    if (defined $limit_period) {
		print FILE_LCK "\t\tperiod " . $limit_period . "\n";
	    }
	    
	    my $limit_thresh = $config->returnValue("$rule limit threshold");
	    if (defined $limit_thresh) {
		print FILE_LCK "\t\tthresh " . $limit_thresh . "\n";
	    }
	    print FILE_LCK "\t}\n";
	}	    

	my $protocol = $config->returnValue("$rule protocol");
	if (defined $protocol) {
	    print FILE_LCK "\tprotocol " . $protocol . "\n";
	}
	else {
	    $protocol = "";
	}

	#destination
	print FILE_LCK "\tdestination {\n";
	my $daddr = $config->returnValue("$rule destination address");
	if (defined $daddr) {
	    if (Vyatta::TypeChecker::validate_iptables4_addr($daddr) eq "1") {
		print FILE_LCK "\t\taddress \"" . $daddr . "\"\n";
	    }
	    else {
		print "Error in destination address configuration\n";
		exit 1;
	    }
	}

	my $option = $config->returnValue("$rule destination port");
	if (defined $option) {
	    my $can_use_port;
	    my $port_str;
	    my $port_err;

	    if ($protocol eq "tcp" || $protocol eq "udp") {
		$can_use_port = "yes";
	    }
	    ($port_str, $port_err) = Vyatta::Misc::getPortRuleString($option, $can_use_port, "d", $protocol);
	    if (defined $port_str) {
		print FILE_LCK "\t\tport-ipt \"" . $port_str . "\"\n";
	    }
	    else {
		print $port_err;
		exit 1;
	    }
	}

	print FILE_LCK "\t}\n";

	#source
	$config->setLevel('load-balancing wan rule');


	print FILE_LCK "\tsource {\n";
	my $saddr = $config->returnValue("$rule source address");
	if (defined $saddr) {
	    if (Vyatta::TypeChecker::validate_iptables4_addr($saddr) eq "1") {
		print FILE_LCK "\t\taddress \"" . $saddr . "\"\n";
	    }
	    else {
		print "Error in source address configuration\n";
		exit 1;
	    }
	}

	$option = $config->returnValue("$rule source port");
	if (defined $option) {
	    my $can_use_port;
	    my $port_str;
	    my $port_err;

	    if ($protocol eq "tcp" || $protocol eq "udp") {
		$can_use_port = "yes";
	    }
	    ($port_str, $port_err) = Vyatta::Misc::getPortRuleString($option, $can_use_port, "s", $protocol);
	    if (defined $port_str) {
		print FILE_LCK "\t\tport-ipt \"" . $port_str . "\"\n";
	    }
	    else {
		print $port_err;
		exit 1;
	    }
	}
	print FILE_LCK "\t}\n";

	#inbound-interface
	my $inbound = $config->returnValue("$rule inbound-interface");
	if (defined $inbound) {
	    print FILE_LCK "\tinbound-interface " . $inbound . "\n"
	}
	else {
	    print "ERROR: inbound-interface must be specified\n";
	    exit 1;
	}

	#interface
	$config->setLevel("load-balancing wan rule $rule interface");
	my @eths = $config->listNodes();

	if ($#eths < 0 && $exclude eq "false") {
	    print "WARNING: rule $rule will be inactive because no (outbound) interfaces have been defined for this rule\n";
	}
	elsif ($#eths >= 0 && $exclude eq "true") {
	    print "WARNING: interfaces (outbound) are not used when exclude has been defined for rule $rule\n";
	}

	foreach my $ethNode (@eths) {
	    if ($inbound eq $ethNode) {
		print "WARNING: inbound interface is the same as the outbound interface\n";
	    }

	    if ($exclude ne "true") {
		$outbound_defined = "true";
	    }

	    print FILE_LCK "\tinterface " . $ethNode . " {\n";
	    
	    $option = $config->returnValue("$ethNode weight");
	    if (defined $option) {
		print FILE_LCK "\t\tweight  " . $option . "\n";
	    }
	    print FILE_LCK "\t}\n";
	}
	print FILE_LCK "}\n";
    }

    if ($outbound_defined eq "false") {
	print "WARNING: At least one rule with an (outbound) interface must be defined for WAN load balancing to be active\n";
	#allow this configuration, just generate the warning
	return "true";
    }
    return $outbound_defined;
}

my $nexthop;

sub usage {
    exit 1;
}

GetOptions("valid-nexthop=s" => \$nexthop,
    ) or usage();

####main
my $conf_file = '/var/run/load-balance/wlb.conf';
my $conf_lck_file = '/var/run/load-balance/wlb.conf.lck';

####are we just validating?
if (defined $nexthop) {
    my $rc = Vyatta::TypeChecker::validateType('ipv4', $nexthop, 1);
    if (!$rc && $nexthop ne "dhcp") {
	exit 1;
    }
    exit 0;
}

#open file
open(FILE_LCK, "+>$conf_lck_file") or die "Can't open wlb lock file";

my $success = write_health();
if ($success eq "false") {
    exit 1;
}

$success = write_rules();
if ($success eq "false") {
    exit 1;
}

close FILE_LCK;

copy ($conf_lck_file,$conf_file);
unlink($conf_lck_file);


#finally kick the process
system "/opt/vyatta/sbin/vyatta-wanloadbalance.init restart $conf_file 2>/dev/null";

exit 0;
