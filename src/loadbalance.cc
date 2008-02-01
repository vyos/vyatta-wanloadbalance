/*
 * Module: loadbalance.cc
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
#include <string>
#include "lbpathtest.hh"
#include "loadbalance.hh"

using namespace std;

/**
 *
 **/
LoadBalance::LoadBalance(bool debug, string &output_path) :
  _debug(debug),
  _lbdata_factory(debug),
  _ph(debug),
  _decision(debug),
  _output(debug, output_path),
  _cycle_interval(5000)
{
}

/**
 *
 **/
LoadBalance::~LoadBalance()
{
  _decision.shutdown();
}

/**
 *
 **/
bool
LoadBalance::set_conf(const string &conf) 
{
  _lbdata_factory.load(conf);
  _lbdata = _lbdata_factory.get();
  if (_lbdata.error()) {
    return false;
  }

  return true;
}

/**
 *
 **/
void
LoadBalance::init()
{
  _decision.init(_lbdata);
}

/**
 *
 **/
bool
LoadBalance::start_cycle()
{
  _lbdata.reset_state_changed();
  return true;
}

/**
 *
 **/
void
LoadBalance::health_test()
{
  _ph.start(_lbdata);
}

/**
 *
 **/
void
LoadBalance::apply_rules()
{
  _decision.run(_lbdata);
}

/**
 *
 **/
void
LoadBalance::output()
{
  _output.write(_lbdata);
}

