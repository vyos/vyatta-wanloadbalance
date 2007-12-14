/*
 * Module: lbdatafactory.hh
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
  LBDataFactory();
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

private:
  LBHealth _lb_health;
  LBRule _lb_rule;
  LBData _lb_data;

  LBData::LBRuleIter _rule_iter;
  LBData::InterfaceHealthIter _health_iter;
  LBRule::InterfaceDistIter _rule_iface_iter;
};

#endif //__LBCONFLOADER_HH__
