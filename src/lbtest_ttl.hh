/*
 * Module: lbtest_ttl.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBTEST_TTL_HH__
#define __LBTEST_TTL_HH__

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
class LBTestTTL : public LBTest 
{
public:
  LBTestTTL(bool debug) : 
    LBTest(debug),
    _ttl(0),
    _port(0),
    _min_port_id(32767),
    _max_port_id(55000)
  {}
  LBTestTTL(bool debug, unsigned short ttl, unsigned short port) : 
    LBTest(debug),
    _ttl(ttl),
    _port(port),
    _min_port_id(32767),
    _max_port_id(55000)
  {}
  ~LBTestTTL() {}

  void
  init();

  void
  send(LBHealth &health);

  unsigned short
  get_ttl() const {return _ttl;}

  unsigned short
  get_port() const {return _port;}

  void
  set_ttl(unsigned short ttl) {_ttl = ttl;}

  void
  set_port(unsigned short port) {_port = port;}

  string
  dump();

  string
  name() {return string("ttl");}

private:
  unsigned short
  udp_checksum(unsigned char ucProto, char *pPacket, int iLength, unsigned long ulSourceAddress, unsigned long ulDestAddress);

  unsigned short
  in_checksum(unsigned short *pAddr, int iLen);

  void
  send(int sock, const string &iface, const string &target_addr, unsigned short packet_id, string address, int ttl, unsigned short port);

  unsigned short
  get_new_packet_id();



private:
  unsigned short _ttl;
  unsigned short _port;
  unsigned short _min_port_id;
  unsigned long _max_port_id;
};

#endif //__LBTEST_TTL_HH__
