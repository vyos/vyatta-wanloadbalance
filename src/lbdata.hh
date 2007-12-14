/*
 * Module: lbdata.hh
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
#ifndef __LBDATA_HH__
#define __LBDATA_HH__

#include <map>
#include <set>
#include <vector>
#include <string>

using namespace std;

class LBRule {
 public:
  typedef map<string, int> InterfaceDistColl;
  typedef map<string, int>::iterator InterfaceDistIter;

  typedef enum {ALL,ICMP,UDP,TCP} Protocol;

  LBRule() :
    _proto("all")
      {}

 public:
  string _proto;
  string _s_addr;
  string _s_net;
  string _s_port_num;
  string _s_port_name;

  string _d_addr;
  string _d_net;
  string _d_port_num;
  string _d_port_name;

  InterfaceDistColl _iface_dist_coll;
};


class LBHealthHistory {
public:
  LBHealthHistory(int buffer_size);

  //push in the ping response for this...
  int push(int rtt);


public:
  //results of health testing
  unsigned long _last_success;
  unsigned long _last_failure;

  static int _buffer_size;
  vector<int> _resp_data;
  int _index;
};

class LBHealth {
 public:
  LBHealth() :
    _success_ct(0),
    _failure_ct(0),
    _ping_resp_time(0),
    _hresults(10),
    _is_active(true),
    _state_changed(true),
    _last_success(0),
    _last_failure(0)
      {}

  void put(int rtt);

  bool 
  state_changed() {return _state_changed;}

  int _success_ct;
  int _failure_ct;
  string _ping_target;
  int _ping_resp_time;
  LBHealthHistory _hresults;
  bool _is_active;
  bool _state_changed;
  unsigned long _last_success;
  unsigned long _last_failure;
};


class LBData {
 public:
  typedef map<int,LBRule> LBRuleColl;
  typedef map<int,LBRule>::iterator LBRuleIter;
  typedef map<int,LBRule>::const_iterator LBRuleConstIter;
  typedef map<string,LBHealth> InterfaceHealthColl;
  typedef map<string,LBHealth>::iterator InterfaceHealthIter;
  typedef map<string,LBHealth>::const_iterator InterfaceHealthConstIter;

  LBData() {}

  bool
  error() {return false;}

  bool
  is_active(const string &iface);

  bool
  state_changed();

  void
  reset_state_changed();

  void
  dump();

 public:
  string _filename;

  LBRuleColl _lb_rule_coll;
  InterfaceHealthColl _iface_health_coll;
};

#endif //__LBDATA_HH__
