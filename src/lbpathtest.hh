/*
 * Module: lbpathtest.hh
 *
 * **** License ****
 * Version: VPL 1.0
 *
 * The contents of this file are subject to the Vyatta Public License
 * Version 1.0 ("License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.vyatta.com/vpl
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * This code was originally developed by Vyatta, Inc.
 * Portions created by Vyatta are Copyright (C) 2007 Vyatta, Inc.
 * All Rights Reserved.
 *
 * Author: Michael Larson
 * Date: 2007
 * Description:
 *
 * **** End License ****
 *
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
