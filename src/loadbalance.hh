/*
 * Module: loadbalance.hh
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
  LoadBalance();
  ~LoadBalance();

  bool set_conf(const string &filename);

  void init();

  bool start_cycle();

  void health_test();

  void apply_rules();

  void output();

  //temporary stand-in for now...
  void sleep() {::sleep(5);}

 private:
  LBDataFactory _lbdata_factory;
  LBData _lbdata;
  LBPathTest _ph;
  LBDecision _decision;
  LBOutput _output;
  int _cycle_interval;
};

#endif //__LOADBALANCE_HH__
