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
  init() {}

  void
  send(LBHealth &health);

  string
  dump();

  string
  name() {return string("ping");}

private:
  void
  send(int sock, const string &iface, const string &target_addr, int packet_id);

  unsigned short
  in_checksum(const unsigned short *buf, int lenght) const;

private:
  bool _debug;
};

#endif //__LBTEST_ICMP_HH__
