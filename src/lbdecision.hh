/*
 * Module: lbdecision.hh
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
  LBDecision();
  ~LBDecision();

  void
  init(LBData &lbdata);

  void
  run(LBData &lbdata);

  void
  shutdown();

private:
  void
  execute(string cmd);

  map<int,float> 
  get_new_weights(LBData &data, LBRule &rule);

  string
  get_application_cmd(LBRule &rule);

private:
  InterfaceMarkColl _iface_mark_coll;
};

#endif //__LBDECISION_HH__
