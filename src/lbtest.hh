/*
 * Module: lbtest.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBTEST_HH__
#define __LBTEST_HH__

#include <assert.h>
#include <map>
#include <set>
#include <vector>
#include <string>

using namespace std;

class LBHealth;

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
class LBTest {
public:
  typedef enum {K_NONE,K_SUCCESS,K_FAILURE} TestState;
public:
  LBTest(bool debug) : 
    _debug(debug),
    _resp_time(5),
    _state(K_NONE)
  {init();}

  virtual ~LBTest();

  virtual void
  init();

  virtual void
  start();
  
  virtual void
  send(LBHealth &health) = 0;

  virtual string
  dump() = 0;

  int
  recv(LBHealth &health);

  virtual string
  name() = 0;

private:
  int
  receive(int recv_sock);


public:
  bool _debug;
  string _target;
  int    _resp_time;
  int _state;

  static int _recv_icmp_sock;
  static int _send_raw_sock;
  static int _send_icmp_sock;
  static bool _initialized;
  static bool _received;
  static int _packet_id;
  static map<int,PktData> _results;
};

#endif //__LBTEST_HH__
