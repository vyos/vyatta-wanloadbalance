/*
 * Module: loadbalance.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LOADBALANCE_HH__
#define __LOADBALANCE_HH__

#include <time.h>

#include <string>
#include <map>

#include "lbdatafactory.hh"
#include "lbpathtest.hh"
#include "lbdecision.hh"
#include "lboutput.hh"
#include "lbdata.hh"


using namespace std;

class LoadBalance
{
 public:
  LoadBalance(bool debug, string &output_path);
  ~LoadBalance();

  bool set_conf(const string &filename);

  void init();

  bool start_cycle();

  void update_paths();

  void health_test();

  void apply_rules();

  void output();

  void sleep();

 private:
  bool _debug;
  LBDataFactory _lbdata_factory;
  LBData _lbdata;
  LBPathTest _ph;
  LBDecision _decision;
  LBOutput _output;
  int _cycle_interval;
};

#endif //__LOADBALANCE_HH__
