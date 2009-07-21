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
#include "lbdata.hh"

using namespace std;

class LBTestICMP;

/**
 *
 *
 **/
class PktData
{
public:
  PktData(string iface, int rtt) : _iface(iface),_rtt(rtt) {}
  string _iface;
  int _rtt;
};


/**
 *
 *
 **/
class ICMPEngine
{
public:
  ICMPEngine() : 
    _debug(true),
    _initialized(false),
    _packet_id(0) 
  {}

  void
  init();

  int
  process(LBHealth &health,LBTestICMP *data);

  int
  recv(LBHealth &health,LBTestICMP *data);

private:
  void
  send(int sock, const string &iface, const string &target_addr, int packet_id);

  int
  receive(int sock);

  unsigned short
  in_checksum(const unsigned short *buf, int lenght) const;

private:
  bool _debug;
  bool _initialized;
  int _packet_id;
  map<int,PktData> _results;
};

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
  init() {_engine.init();this->LBTest::init();}

  void
  send(LBHealth &health) {_engine.process(health,this);}

  int
  recv(LBHealth &health) {return _engine.recv(health,this);}

  string
  dump();

private:
  static ICMPEngine _engine; //singleton
  bool _debug;
};

#endif //__LBTEST_ICMP_HH__
