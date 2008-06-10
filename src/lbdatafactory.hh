/*
 * Module: lbdatafactory.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBCONFLOADER_HH__
#define __LBCONFLOADER_HH__

#include <string>
#include <vector>

#include "lbdata.hh"

using namespace std;

class LBDataFactory {
public:
  typedef vector<string> ConfColl;
  typedef vector<string>::iterator ConfIter;

public:
  LBDataFactory(bool debug);
  ~LBDataFactory();

  bool
  load(const string &conf_file);

  LBData
  get();

private:
  //parsing goes on here
  void 
  tokenize(const string& str,
	   vector<string>& tokens,
	   const string& delimiters = " ");

  void
  process(const vector<string> &path, int depth, const string &key, const string &value);

  void
  process_health(const string &key, const string &value);

  void
  process_health_interface(const string &key, const string &value);

  void
  process_rule(const string &key, const string &value);

  void
  process_rule_protocol(const string &key, const string &value);

  void
  process_rule_source(const string &key, const string &value);

  void
  process_rule_destination(const string &key, const string &value);

  void
  process_rule_interface(const string &key, const string &value);

  void
  process_rule_inbound_interface(const string &key, const string &value);

private:
  bool _debug;
  LBHealth _lb_health;
  LBRule _lb_rule;
  LBData _lb_data;

  LBData::LBRuleIter _rule_iter;
  LBData::InterfaceHealthIter _health_iter;
  LBRule::InterfaceDistIter _rule_iface_iter;
};

#endif //__LBCONFLOADER_HH__
