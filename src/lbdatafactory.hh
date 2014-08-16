/*
 * Module: lbdatafactory.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBDATALOADER_HH__
#define __LBDATALOADER_HH__

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
  process_disablesourcenat(const string &key, const string &value);

  void
  process_enablelocaltraffic(const string &key, const string &value);

  void
  process_stickyinboundconnections(const string &key, const string &value);
  
  void
  process_flushconntrack(const string &key, const string &value);

  void
  process_health(const string &key, const string &value);

  void
  process_hook(const string &key, const string &value);

  void
  process_health_interface(const string &key, const string &value);

  void
  process_health_interface_rule(const string &key, const string &value);

  void
  process_health_interface_rule_type(const string &key, const string &value);

  void
  process_health_interface_rule_type_target(const string &key, const string &value);

  void
  process_health_interface_rule_type_resptime(const string &key, const string &value);

  void
  process_health_interface_rule_type_udp(const string &key, const string &value);

  void
  process_health_interface_rule_type_user(const string &key, const string &value);

  void
  process_rule(const string &key, const string &value);

  void
  process_rule_exclude(const string &key, const string &value);

  void
  process_rule_failover(const string &key, const string &value);

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

  void
  process_rule_enablesourcebasedrouting(const string &key, const string &value);

  void
  process_rule_limit(const string &key, const string &value);

private:
  bool _debug;
  LBHealth _lb_health;
  LBRule _lb_rule;
  LBData _lb_data;
  int _interface_index;
  int _current_test_rule_number;

  LBData::LBRuleIter _rule_iter;
  LBData::InterfaceHealthIter _health_iter;
  LBRule::InterfaceDistIter _rule_iface_iter;
  LBHealth::TestIter _test_iter;
};

#endif //__LBDATALOADER_HH__
