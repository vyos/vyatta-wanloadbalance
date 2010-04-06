/*
 * Module: lbdecision.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBDECISION_HH__
#define __LBDECISION_HH__

#include <map>
#include <string>
#include "lbdata.hh"

using namespace std;

class LBDecision
{
public:
  LBDecision(bool debug);
  ~LBDecision();

  void
  init(LBData &lbdata);

  void
  update_paths(LBData &lbdata);

  void
  run(LBData &lbdata);

  void
  shutdown(LBData &lbdata);

private:
  int
  execute(string cmd, string &stdout, bool read = false);

  void
  insert_default(LBHealth &h, string &nexthop);

  map<string,float> 
  get_new_weights(LBData &data, LBRule &rule);

  string
  get_application_cmd(LBRule &rule, bool local = false, bool exclude = false);

  string
  fetch_iface_addr(const string &iface);

  string
  fetch_iface_nexthop(const string &iface);

  string
  get_limit_cmd(LBRule &rule);

private:
  bool _debug;
};

#endif //__LBDECISION_HH__
