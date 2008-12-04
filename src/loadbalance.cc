/*
 * Module: loadbalance.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
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
  _decision.shutdown(_lbdata);
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


/**
 *
 **/
//temporary stand-in for now...
void 
LoadBalance::sleep() 
{
  ::sleep(5);
}

