/*
 * Module: lbpathtest.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <string.h>
#include <memory>
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "lbdata.hh"
#include "lbpathtest.hh"

using namespace std;

/**
 *
 *
 **/
LBPathTest::LBPathTest(bool debug) : 
  _debug(debug)
{
}

/**
 *
 *
 **/
LBPathTest::~LBPathTest()
{
}

/**
 *
 *
 **/
void
LBPathTest::start(LBData &lb_data)
{
  //iterate through the tests until success per interface or complete
  if (_debug) {
    cout << "LBPathTest::start(): init" << endl;
  }

  set<LBHealth*> coll;

  //iterate over the health interfaces, until success or test cases exhausted per interface
  LBData::InterfaceHealthIter iter = lb_data._iface_health_coll.begin();
  while (iter != lb_data._iface_health_coll.end()) {
    coll.insert(&(iter->second));
      iter->second.start_new_test_cycle();
    ++iter;
  }
  
  while (!coll.empty()) {
    set<LBHealth*>::iterator i = coll.begin();
    while (i != coll.end()) {
      (*i)->start_new_test();
      ++i;
    }
    
    if (_debug) {
      cout << "LBPathTest::start(): sending " << coll.size() << " tests" << endl;
    }
    //send all interface tests together
    i = coll.begin();
    while (i != coll.end()) {
      (*i)->send_test();
      ++i;
    }
    
    if (_debug) {
      cout << "LBPathTest::start(): waiting on recv" << endl;
    }
    //wait on responses
    i = coll.begin();
    while (i != coll.end()) {
      int resp = (*i)->recv_test();
      if (_debug) {
	cout << "LBPathTest::start() interface: " << (*i)->_interface << " response value: " << (*i)->_hresults.get_last_resp() << endl;
      }
      if (resp == 0) { //means this interface has either exhausted its test cases or success
	coll.erase(i++); 
      }
      else {
	++i;
      }
    }
  }
}

