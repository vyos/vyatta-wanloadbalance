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

class LBPathTest
{
public:
  LBPathTest(bool debug);
  ~LBPathTest();

  void
  start(LBData &lb_data);

private:
  bool _debug;
};


#endif //__LBPATHTEST_HH__
