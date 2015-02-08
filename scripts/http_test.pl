#!/usr/bin/perl
#
# Module: http_test.pl
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2009, 2010 Vyatta, Inc.
# All Rights Reserved.                                                                                                                                                              
# Authors: Michael Larson
# Date: 2011
# Description: wanloadbalance script sample. 
# Tests whether google.com responds to simple http GET request.
#

use strict;
use warnings;
use POSIX;
use JSON;
use Data::Dumper;
use URI::Escape;

my $iface = $ENV{WLB_SCRIPT_IFACE};

my $code;
my $body;

#let's see if we can reach google out this interface
my @out = `curl -s -m 3 --interface $iface -i -X GET www.google.com`;
#now process output, for http status code and for response body
foreach my $out (@out) {
    if ($out =~ /^HTTP\/[\d.]+\s+(\d+)\s+.*$/) {
        $code = $1;
    } elsif ($out =~ /^\r/ || defined $body) {
        $body .= $out;
    }
}

#success is if http response code is 200 and body is returned
if (defined($code) && $code == 200 && defined($body)) {
    #WLB SUCCESS
    exit(0);
}

#WLB FAILURE
exit(1);
