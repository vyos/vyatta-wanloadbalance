/*
 * Module: lbpathtest.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBPATHTEST_HH__
#define __LBPATHTEST_HH__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include "lbdata.hh"

using namespace std;

class PktData
{
public:
  PktData(string iface, int rtt) : _iface(iface),_rtt(rtt) {}
  string _iface;
  int _rtt;
};

class LBPathTest
{
public:
  LBPathTest(bool debug);
  ~LBPathTest();

  void
  start(LBData &lb_data);

private:
  void
  send(const string &iface, const string &target_addr, int packet_id);

  int
  receive();

  unsigned short
  in_checksum(const unsigned short *buf, int lenght) const;

private:
  bool _debug;
  int _send_sock;
  int _recv_sock;
  int _packet_id;
};


#endif //__LBPATHTEST_HH__
