/*
 * Module: lbtest_icmp.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBTEST_ICMP_HH__
#define __LBTEST_ICMP_HH__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include "lbtest.hh"

using namespace std;

/**
 *
 *
 **/
class LBTestICMP : public LBTest 
{
public:
  LBTestICMP(bool debug) : LBTest(debug) {}
  ~LBTestICMP() {}

  void
  init() {_status_line=name();}

  void
  send(LBHealth &health);

  string
  dump();

  string
  name() {return string("ping");}

  string
  status();

private:
  void
  send(int sock, const string &iface, const string &target_addr, int packet_id);

  unsigned short
  in_checksum(const unsigned short *buf, int length) const;
};

#endif //__LBTEST_ICMP_HH__
