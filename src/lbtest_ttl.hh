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
#include "lbdata.hh"

using namespace std;

class LBTestTTL;

/**
 *
 *
 **/
class TTLEngine
{
  class PktData
  {
  public:
    PktData(string iface, int rtt) : _iface(iface),_rtt(rtt) {}
    string _iface;
    int _rtt;
  };

public:
  TTLEngine() : 
    _debug(true),
    _initialized(false),
    _received(false),
    _packet_id(32767),
    _min_port_id(32767),
    _max_port_id(55000)
  {}

  void
  init();

  int
  process(LBHealth &health,LBTestTTL *data);

  int
  recv(LBHealth &health,LBTestTTL *data);

private:
  void
  send(int sock, const string &iface, const string &target_addr, unsigned short packet_id, string address, int ttl, unsigned short port);

  int
  receive(int sock);

  unsigned short
  udp_checksum(unsigned char ucProto, char *pPacket, int iLength, unsigned long ulSourceAddress, unsigned long ulDestAddress);

  unsigned short
  in_checksum(unsigned short *pAddr, int iLen);

  unsigned short
  get_new_packet_id();

private:
  bool _debug;
  bool _initialized;
  bool _received;
  unsigned short _packet_id;
  unsigned short _min_port_id;
  unsigned long _max_port_id;

  map<int,PktData> _results;
};

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
    _port(0)
  {}
  LBTestTTL(bool debug, unsigned short ttl, unsigned short port) : 
    LBTest(debug),
    _ttl(ttl),
    _port(port)
  {}
  ~LBTestTTL() {}

  void
  init() {_engine.init();this->LBTest::init();}

  void
  send(LBHealth &health) {_engine.process(health,this);}

  int
  recv(LBHealth &health) {return _engine.recv(health,this);}

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

private:
  static TTLEngine _engine; //singleton
  bool _debug;
  unsigned short _ttl;
  unsigned short _port;
};

#endif //__LBTEST_TTL_HH__
