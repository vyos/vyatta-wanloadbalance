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
  typedef map<string,int> InterfaceMarkColl;
  typedef map<string,int>::iterator InterfaceMarkIter;

public:
  LBDecision(bool debug);
  ~LBDecision();

  void
  init(LBData &lbdata);

  void
  run(LBData &lbdata);

  void
  shutdown();

private:
  int
  execute(string cmd, string &stdout, bool read = false);

  void
  insert_default(string cmd, int table);

  map<int,float> 
  get_new_weights(LBData &data, LBRule &rule);

  string
  get_application_cmd(LBRule &rule);

  string
  fetch_iface_addr(const string &iface);

private:
  bool _debug;
  InterfaceMarkColl _iface_mark_coll;
};

#endif //__LBDECISION_HH__
