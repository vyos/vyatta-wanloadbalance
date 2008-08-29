/*
 * Module: lbdatafactory.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
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
#include "lbdatafactory.hh"

using namespace std;


LBDataFactory::LBDataFactory(bool debug) :
  _debug(debug)
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

    unsigned int pos = line.find("#");
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
	    unsigned int end_pos = line.rfind("\"");
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

  if (path[0] == "disable-source-nat") {
    process_disablesourcenat(l_key,l_value);
  }
  else if (path[0] == "health") {
    if (l_key == "interface") {
      process_health(l_key,l_value);
    }
    else {
      process_health_interface(l_key,l_value);
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
LBDataFactory::process_health(const string &key, const string &value) 
{
  if (value.empty() == false) {
    LBData::InterfaceHealthIter iter = _lb_data._iface_health_coll.find(key);
    if (iter == _lb_data._iface_health_coll.end()) {
      _lb_data._iface_health_coll.insert(pair<string,LBHealth>(value,LBHealth()));
    }
    _health_iter = _lb_data._iface_health_coll.find(value);
  }
}


void
LBDataFactory::process_health_interface(const string &key, const string &value) 
{
  if (key == "target") {
    _health_iter->second._ping_target = value;
  }
  else if (key == "success-ct") {
    int num = strtoul(value.c_str(), NULL, 10);
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
    int num = strtoul(value.c_str(), NULL, 10);
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
  else if (key == "ping-resp") {
    int num = strtoul(value.c_str(), NULL, 10);
    if (num > 0) {
      _health_iter->second._ping_resp_time = num;
    }
    else {
      if (_debug) {
	cerr << "illegal ping-resp specified: " << value << endl;
      }
      syslog(LOG_ERR, "wan_lb: illegal ping-resp specified in configuration file: %s", value.c_str());
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
  _rule_iter->second._exclude = true;
}

void
LBDataFactory::process_rule_failover(const string &key, const string &value)
{
  _rule_iter->second._failover = true;
}

void
LBDataFactory::process_rule_source(const string &key, const string &value)
{
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



LBData
LBDataFactory::get()
{
  return _lb_data;
}

