/*
 * Module: lbdatafactory.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <stdio.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include "rl_str_proc.hh"
#include "lbdata.hh"
#include "lbtest_icmp.hh"
#include "lbtest_ttl.hh"
#include "lbtest_user.hh"
#include "lbdatafactory.hh"

using namespace std;


LBDataFactory::LBDataFactory(bool debug) :
  _debug(debug),
  _lb_health(),
  _interface_index(0),
  _current_test_rule_number(0)
{
}

LBDataFactory::~LBDataFactory()
{

}

bool
LBDataFactory::load(const string &conf_file)
{
  //open file
  FILE *fp = fopen(conf_file.c_str(), "r");
  if (fp == NULL) {
    if (_debug) {
      cerr << "Error opening configuration file: " << conf_file << endl;
    }
    syslog(LOG_ERR, "wan_lb: configuration file not found: %s", conf_file.c_str());
    return false;
  }

  //read line by line and populate vect
  char str[1025];
  int depth(0);
  vector<string> path(32);
  while (fgets(str, 1024, fp) != 0) {
    string line(str);

    size_t pos = line.find("#");
    line = line.substr(0,pos);
    
    string key,value;

    StrProc tokens(line, " ");
    for (int i = 0; i < tokens.size(); ++i) {
      string symbol = tokens.get(i);

      if (symbol != "{" && symbol != "}") {
	if (key.empty()) {
	  key = symbol;
	}
	else if (value.empty()) {
	  if ((pos = line.find("\"")) != string::npos) {
	    size_t end_pos = line.rfind("\"");
	    symbol = line.substr(pos+1,end_pos-pos-1);
	  }
	  value = symbol;
	}
	path[depth] = key;
      }
      else if (symbol == "{") {
	++depth;
      }
      else if (symbol == "}") {
	path[depth] = string("");
	--depth;
      }
    }
    if (tokens.size() != 0) {
      process(path,depth,key,value);
    }
    if (depth > 31 || depth < 0) {
      if (_debug) {
	cerr << "configuration error: malformed configuration file: brackets" << endl;
      }
      syslog(LOG_ERR, "wan_lb: malformed configuration file: brackets");
      return false;
    }
  }

  fclose(fp);
  if (depth != 0) {
    if (_debug) {
      cerr << "configuration error: mismatched brackets in configuration file" << endl;
    }
    syslog(LOG_ERR, "wan_lb: configuration error due to mismatched brackets");
    return false;
  }

  //insert default test if empty
  LBData::InterfaceHealthIter iter = _lb_data._iface_health_coll.begin();
  while (iter != _lb_data._iface_health_coll.end()) {
    if (iter->second._test_coll.empty()) {
      LBTestICMP *test = new LBTestICMP(_debug);
      iter->second._test_coll.insert(pair<int,LBTest*>(1,test));
      //add default ping test...
    }
    ++iter;
  }

  if (_debug) {
    _lb_data.dump();
  }
  return true;
}


void
LBDataFactory::process(const vector<string> &path, int depth, const string &key, const string &value)
{
  string l_key, l_value;
  std::transform(key.begin(), key.end(), std::back_inserter(l_key),
		 static_cast < int(*)(int) > (std::tolower));
  std::transform(value.begin(), value.end(), std::back_inserter(l_value),
		 static_cast < int(*)(int) > (std::tolower));

  if (_debug) {
    cout << "LBDataFactory::process(" << depth << "): " << key << ":" << value << endl;
  }

  if (path[0] == "disable-source-nat") {
    process_disablesourcenat(l_key,l_value);
  }
  else if (path[0] == "enable-local-traffic") {
    process_enablelocaltraffic(l_key,l_value);
  }
  else if (path[0] == "sticky-connections") {
	if (l_value == "inbound") {
      process_stickyinboundconnections(l_key,l_value);
    }
  }  
  else if (path[0] == "flush-conntrack") {
    process_flushconntrack(l_key,l_value);
  }
  else if (path[0] == "hook") {
    process_hook(l_key,l_value);
  }
  else if (path[0] == "health") {
    if (depth == 2 && key == "interface") {
      process_health(l_key,l_value);
    }
    else if (depth == 2) {
      process_health_interface(l_key,l_value);
    }
    else if (depth == 3) {
      process_health_interface_rule(l_key,l_value);
    }
    else if (depth == 4 && key == "type") {
      process_health_interface_rule_type(l_key,l_value);
    }
    else if (depth == 4 && key == "target") {
      process_health_interface_rule_type_target(l_key,l_value);
    }
    else if (depth == 4 && key == "resp-time") {
      process_health_interface_rule_type_resptime(l_key,l_value);
    }
    else if (depth == 4 && key == "ttl") {
      process_health_interface_rule_type_udp(l_key,l_value);
    }
    else if (depth == 4 && key == "test-script") {
      process_health_interface_rule_type_user(l_key,l_value);
    }
  }
  else if (path[0] == "rule") {
    if (depth > 0 && path[1] == "source") {
      process_rule_source(l_key,l_value);
    }
    else if (depth > 0 && path[1] == "destination") {
      process_rule_destination(l_key,l_value);
    }
    else if (depth > 0 && path[1] == "inbound-interface") {
      process_rule_inbound_interface(l_key,l_value);
    }
    else if (depth > 1 && path[1] == "interface") {
      process_rule_interface(l_key,l_value);
    }
    else if (depth > 0 && path[1] == "protocol") {
      process_rule_protocol(l_key,l_value);
    }
    else if (depth > 0 && path[1] == "exclude") {
      process_rule_exclude(l_key,l_value);
    }
    else if (depth > 0 && path[1] == "failover") {
      process_rule_failover(l_key,l_value);
    }
    else if (depth > 0 && path[1] == "per-packet-balancing") {
      process_rule_enablesourcebasedrouting(l_key,l_value);
    }
    else if (depth > 1 && path[1] == "limit") {
      process_rule_limit(l_key,l_value);
    }
    else {
      process_rule(l_key,l_value);
    }
  }
}

void
LBDataFactory::process_disablesourcenat(const string &key, const string &value)
{
  _lb_data._disable_source_nat = true;
}

void
LBDataFactory::process_enablelocaltraffic(const string &key, const string &value)
{
  _lb_data._enable_local_traffic = true;
}

void
LBDataFactory::process_stickyinboundconnections(const string &key, const string &value)
{
  _lb_data._sticky_inbound_connections = true;
}

void
LBDataFactory::process_flushconntrack(const string &key, const string &value)
{
  _lb_data._flush_conntrack = true;
}

void
LBDataFactory::process_hook(const string &key, const string &value)
{
  if (value.empty() == false) {
    _lb_data._hook = value;
  }
}

void
LBDataFactory::process_health(const string &key, const string &value) 
{
  if (value.empty() == false) {
    LBData::InterfaceHealthIter iter = _lb_data._iface_health_coll.find(key);
    if (iter == _lb_data._iface_health_coll.end()) {
      _lb_data._iface_health_coll.insert(pair<string,LBHealth>(value,LBHealth(++_interface_index,const_cast<string&>(value))));
    }
    _health_iter = _lb_data._iface_health_coll.find(value);
  }
}

void
LBDataFactory::process_health_interface(const string &key, const string &value) 
{
  if (key == "success-ct") {
    int num = strtoul(value.c_str(), NULL, 10000);
    if (num > 0) {
      _health_iter->second._success_ct = num;
    }
    else {
      if (_debug) {
	cerr << "illegal success-ct specified: " << value << endl;
      }
      syslog(LOG_ERR, "wan_lb: illegal success-ct specified in configuration file: %s", value.c_str());
    }
  }
  else if (key == "failure-ct") {
    int num = strtoul(value.c_str(), NULL, 10000);
    if (num > 0) {
      _health_iter->second._failure_ct = num;
    }
    else {
      if (_debug) {
	cerr << "illegal failure-ct specified: " << value << endl;
      }
      syslog(LOG_ERR, "wan_lb: illegal failure-ct specified in configuration file: %s", value.c_str());
    }
  }
  else if (key == "nexthop") {
    _health_iter->second._nexthop = value;
  }
  else if (key == "health") {
    //nothing
  }
  else {
    if (_debug) {
      cout << "LBDataFactory::process_health(): " << "don't understand this symbol: " << key << endl;
    }
    //nothing
  }

}

void
LBDataFactory::process_health_interface_rule_type_target(const string &key, const string &value)
{
  if (_test_iter != _health_iter->second._test_coll.end()) {
    _test_iter->second->_target = value;
  }
}

void
LBDataFactory::process_health_interface_rule_type_resptime(const string &key, const string &value)
{
  if (_test_iter != _health_iter->second._test_coll.end()) {
    _test_iter->second->_resp_time = strtoul(value.c_str(), NULL, 10);
  }
}

void
LBDataFactory::process_health_interface_rule_type_udp(const string &key, const string &value)
{
  if (_test_iter != _health_iter->second._test_coll.end()) {
    if (key == "ttl") {
      unsigned short val = (unsigned short)strtol(value.c_str(),NULL,10);
      ((LBTestTTL*)_test_iter->second)->set_ttl(val);
    }
    else if (key == "port") {
      unsigned short val = (unsigned short)strtol(value.c_str(),NULL,10);
      ((LBTestTTL*)_test_iter->second)->set_port(val);
    }
  }
}

void
LBDataFactory::process_health_interface_rule_type_user(const string &key, const string &value)
{
  if (_test_iter != _health_iter->second._test_coll.end()) {
    if (key == "test-script") {
      ((LBTestUser*)_test_iter->second)->set_script((string&)value);
    }
  }
}

void
LBDataFactory::process_health_interface_rule_type(const string &key, const string &value)
{
  if (value == "ping") {
    LBTestICMP *test = new LBTestICMP(_debug);
    _health_iter->second._test_coll.insert(pair<int,LBTest*>(_current_test_rule_number,test));
  }
  else if (value == "udp") {
    LBTestTTL *test = new LBTestTTL(_debug);
    _health_iter->second._test_coll.insert(pair<int,LBTest*>(_current_test_rule_number,test));
  }
  else if (value == "user-defined") {
    LBTestUser *test = new LBTestUser(_debug);
    _health_iter->second._test_coll.insert(pair<int,LBTest*>(_current_test_rule_number,test));
  }
  _test_iter = _health_iter->second._test_coll.find(_current_test_rule_number);
}


void
LBDataFactory::process_health_interface_rule(const string &key, const string &value) 
{
  _current_test_rule_number = strtoul(value.c_str(), NULL, 10);
}

void
LBDataFactory::process_rule(const string &key, const string &value)
{
  if (key.empty()) {
    return;
  }
  if (_debug) {
    cout << "LBDataFactor::process_rule(): " << key << ", " << value << endl;
  }
  int num = strtoul(value.c_str(), NULL, 10);
  if (num > 0) {
    _lb_data._lb_rule_coll.insert(pair<int,LBRule>(num,LBRule()));
  }
  else {
    if (_debug) {
      cerr << "Rule number: illegal value" << endl;
    }
    syslog(LOG_ERR, "wan_lb: illegal rule number: %s", value.c_str());
    return;
  }
  _rule_iter = _lb_data._lb_rule_coll.find(num);
}

void
LBDataFactory::process_rule_protocol(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  if (key == "protocol") {
    if (strcasecmp(value.c_str(),"ALL") == 0) {
      _rule_iter->second._proto = "all";
    }
    else if (strcasecmp(value.c_str(),"ICMP") == 0) {
      _rule_iter->second._proto = "icmp";      
    }
    else if (strcasecmp(value.c_str(), "UDP") == 0) {
      _rule_iter->second._proto = "udp";      
    }
    else if (strcasecmp(value.c_str(),"TCP") == 0) {
      _rule_iter->second._proto = "tcp";      
    }
    else {
      if (_debug) {
	cerr << "protocol not recognized: " << key << ", " << value << endl;
      }
      syslog(LOG_ERR, "wan_lb: illegal protocol specified: %s", value.c_str());
    }
  }
}

void
LBDataFactory::process_rule_exclude(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  _rule_iter->second._exclude = true;
}

void
LBDataFactory::process_rule_failover(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  _rule_iter->second._failover = true;
}

void
LBDataFactory::process_rule_enablesourcebasedrouting(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  _rule_iter->second._enable_source_based_routing = true;
}

void
LBDataFactory::process_rule_source(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  if (key == "address") {
    _rule_iter->second._s_addr = value;
  }
  else if (key == "port-ipt") {
    _rule_iter->second._s_port_ipt = value;
  }
  else if (key == "port") {
    _rule_iter->second._s_port = value;
  }
}

void
LBDataFactory::process_rule_destination(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  if (key == "address") {
    _rule_iter->second._d_addr = value;
  }
  else if (key == "port-ipt") {
    _rule_iter->second._d_port_ipt = value;
  }
  else if (key == "port") {
    _rule_iter->second._d_port = value;
  }
}

void
LBDataFactory::process_rule_inbound_interface(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  if (_debug) {
    cout << "LBDataFactory::process_rule_inbound_interface(): " << key << ", " << value << endl;
  }
  if (key == "inbound-interface") {
    _rule_iter->second._in_iface = value;
  }
  else {
    if (_debug) {
      cerr << "LBDataFactory::process_rule(): " << "don't understand this symbol: " << key << endl;
    }
  }
}



void
LBDataFactory::process_rule_interface(const string &key, const string &value)
{
  if (_debug) {
    cout << "LBDataFactory::process_rule_interface(): " << key << ", " << value << endl;
  }
  if (key == "interface") {
    _rule_iter->second._iface_dist_coll.insert(pair<string,int>(value,0));
    _rule_iface_iter = _rule_iter->second._iface_dist_coll.find(value);
  }
  else if (key == "weight") {
    int num = strtoul(value.c_str(), NULL, 10);
    if (num > 0) {
      _rule_iface_iter->second = num;
    }
    else {
      if (_debug) {
	cerr << "illegal interface weight specified: " << value << endl;
      }
      syslog(LOG_ERR, "wan_lb: illegal interface weight specified in configuration file: %s", value.c_str());
    }
  }
  else {
    if (_debug) {
      cerr << "LBDataFactory::process_rule(): " << "don't understand this symbol: " << key << endl;
    }
  }
}

void
LBDataFactory::process_rule_limit(const string &key, const string &value)
{
  if (_rule_iter == _lb_data._lb_rule_coll.end()) {
    return;
  }
  
  _rule_iter->second._limit = true;
  
  if (key == "burst") {
    _rule_iter->second._limit_burst = value;
  }
  else if (key == "rate") {    
    _rule_iter->second._limit_rate = value;
  }
  else if (key == "period") {
    if (value == "second") {
      _rule_iter->second._limit_period = LBRule::K_SECOND;
    }
    else if (value == "minute") {
      _rule_iter->second._limit_period = LBRule::K_MINUTE;
    }
    else if (value == "hour") {
      _rule_iter->second._limit_period = LBRule::K_HOUR;
    }
  }
  else if (key == "threshold") {
    if (value == "true") {
      _rule_iter->second._limit_mode = true;
    }
    else {
      _rule_iter->second._limit_mode = false;
    }
  }
}

LBData
LBDataFactory::get()
{
  return _lb_data;
}

