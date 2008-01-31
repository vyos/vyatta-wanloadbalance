/*
 * Module: lboutput.cc
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
#include <sys/time.h>
#include <time.h>

#include <iostream>

#include "lbdata.hh"
#include "lboutput.hh"

void
LBOutput::write(const LBData &lbdata)
{
  timeval tv;
  gettimeofday(&tv,NULL);

  //dump out the health data
  LBData::InterfaceHealthConstIter iter = lbdata._iface_health_coll.begin();
  while (iter != lbdata._iface_health_coll.end()) {
    cout << iter->first << " "; //interface
    cout << string(iter->second._is_active ? "true" : "false") << " "; //status
    cout << tv.tv_sec - iter->second._last_success << " "; //last success
    cout << tv.tv_sec - iter->second._last_failure << " "; //last failure
    ++iter;
  }

  //dump out the application data
  LBData::LBRuleConstIter r_iter = lbdata._lb_rule_coll.begin();
  while (r_iter != lbdata._lb_rule_coll.end()) {
    if (_debug) {
      cout << "squirt out results here." << endl;
    }
    ++r_iter;
  }
}
