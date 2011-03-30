/*
 * Module: lbtest_user.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBTEST_USER_HH__
#define __LBTEST_USER_HH__

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
class LBTestUser : public LBTest 
{
public:
  LBTestUser(bool debug) : 
    LBTest(debug)
  {}
  LBTestUser(bool debug, string &script) :
    LBTest(debug),
    _script(script)
  {}
  ~LBTestUser() {}

  void
  send(LBHealth &health);

  std::string
  get_script() const {return _script;}

  void
  set_script(std::string &script) {_script = script;}

  string
  dump();

  string
  name() {return string("user");}

  //override, don't need base support for these.
  void
  init() {_status_line=name();}
  void
  start() {}
  int
  recv(LBHealth &health) {return (_state != LBTest::K_SUCCESS) ? -1 : 1;}


private: //methods
  int
  system_out(const string &cmd, string &out);

private: //variables
  string _script;
};

#endif //__LBTEST_USER_HH__
