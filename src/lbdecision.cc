/*
 * Module: lbdecision.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <syslog.h>
#include <iostream>
#include "lbdata.hh"
#include "lbdecision.hh"

using namespace std;

/*
iptables -t mangle -N ISP1
iptables -t mangle -A ISP1 -j CONNMARK --set-mark 1
iptables -t mangle -A ISP1 -j MARK --set-mark 1
iptables -t mangle -A ISP1 -j ACCEPT

iptables -t mangle -N ISP2
iptables -t mangle -A ISP2 -j CONNMARK --set-mark 2
iptables -t mangle -A ISP2 -j MARK --set-mark 2
iptables -t mangle -A ISP2 -j ACCEPT


#THIS APPEARS TO ROUGHLY WORK BELOW, AND CAN BE SET UP WITH SPECIFIC FILTERS.
iptables -t mangle -A PREROUTING -i eth0 -m statistic --mode nth --every 2 --packet 0 -j ISP1
iptables -t mangle -A PREROUTING -i eth0 -j ISP2

#iptables -t mangle -A PREROUTING -i eth0 -m state --state NEW -m statistic --mode random --probability .01 -j MARK --set-mark 1
#iptables -t mangle -A PREROUTING -i eth0 -j MARK --set-mark 2

iptables -t raw -N NAT_CONNTRACK
iptables -t raw -A NAT_CONNTRACK -j ACCEPT
iptables -t raw -I PREROUTING 1 -j NAT_CONNTRACK
iptables -t raw -I OUTPUT 1 -j NAT_CONNTRACK
ip ro add table 10 default via 192.168.1.2  dev eth1
ip ru add fwmark 1 table 10
ip ro fl ca
ip ro add table 20 default via 192.168.2.2 dev eth2
ip ru add fwmark 2 table 20
ip ro fl ca 

*/


/**
 *
 *
 **/
LBDecision::LBDecision(bool debug) : 
  _debug(debug)
{

}

/**
 *
 *
 **/
LBDecision::~LBDecision()
{
  shutdown();
}

/**
 *
 *
 **/
void
LBDecision::init(LBData &lbdata)
{
  //here is where we set up iptables and policy routing for the interfaces
  /*
    iptables -t mangle -N ISP1
    iptables -t mangle -A ISP1 -j CONNMARK --set-mark 1
    iptables -t mangle -A ISP1 -j MARK --set-mark 1
    iptables -t mangle -A ISP1 -j ACCEPT
   */

  char buf[20];
  int ct = 1;

  /*
    do we need: 
iptables -t raw -N NAT_CONNTRACK
iptables -t raw -A NAT_CONNTRACK -j ACCEPT
iptables -t raw -I PREROUTING 1 -j NAT_CONNTRACK
iptables -t raw -I OUTPUT 1 -j NAT_CONNTRACK

if so then this stuff goes here!
   */


  //note: doesn't appear to clean up rule table, may need to individually erase each rule
  //  execute(string("ip rule flush"));

  LBData::InterfaceHealthIter iter = lbdata._iface_health_coll.begin();
  while (iter != lbdata._iface_health_coll.end()) {
    string iface = iter->first;

    sprintf(buf,"%d",ct);

    execute(string("iptables -t mangle -N ISP_") + buf);
    execute(string("iptables -t mangle -F ISP_") + buf);
    execute(string("iptables -t mangle -A ISP_") + buf + " -j CONNMARK --set-mark " + buf);
    //    execute(string("iptables -t mangle -A ISP_") + buf + " -j MARK --set-mark " + buf);

    //NOTE, WILL NEED A WAY TO CLEAN UP THIS RULE ON RESTART...
    execute(string("iptables -t mangle -A ISP_") + buf + " -j ACCEPT");


    execute(string("ip route replace table ") + buf + " default dev " + iface + " via " + iter->second._nexthop);
    execute(string("ip rule add fwmark ") + buf + " table " + buf);

    _iface_mark_coll.insert(pair<string,int>(iface,ct));
    ++ct;
    ++iter;
  }
  execute("ip route flush cache");
}


/**
 * only responsible for 

iptables -t mangle -A PREROUTING -i eth0 -m state --state NEW -m statistic --mode random --probability .01 -j MARK --set-mark 1
iptables -t mangle -A PREROUTING -i eth0 -j MARK --set-mark 2

 *
 *
 *
 **/
void
LBDecision::run(LBData &lb_data)
{
  if (_debug) {
    cout << "LBDecision::run(), starting decision" << endl;
  }

  //first determine if we need to alter the rule set
  if (!lb_data.state_changed()) {
    return;
  }

  if (_debug) {
    cout << "LBDecision::run(), state changed, applying new rule set" << endl;
  }

  //then if we do, flush all
  execute("iptables -t mangle -F PREROUTING");

  //and compute the new set and apply
  LBData::LBRuleIter iter = lb_data._lb_rule_coll.begin();
  while (iter != lb_data._lb_rule_coll.end()) {
    map<int,float> weights = get_new_weights(lb_data,iter->second);
    map<int,float>::iterator w_iter = weights.begin();
    map<int,float>::iterator w_end = weights.end();
    if (w_iter == w_end) {
      ++iter;
      continue;
    }      
    else {
      --w_end;
    }

    //NEED TO HANDLE APPLICATION SPECIFIC DETAILS
    string app_cmd = get_application_cmd(iter->second);

    char fbuf[20],dbuf[20];
    while (w_iter != w_end) {
      sprintf(fbuf,"%f",w_iter->second);
      sprintf(dbuf,"%d",w_iter->first);
      //      execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j CONNMARK --set-mark " + dbuf);
      execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf);

      ++w_iter;
    }
    //last one is special case, the catch all rule
    ++w_iter;
    sprintf(dbuf,"%d",w_iter->first);
    //    execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -j CONNMARK --set-mark " + dbuf);
    execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -j ISP_" + dbuf);
    execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -j CONNMARK --restore-mark");
    ++iter;
  }
}

/**
 *
 *
 **/
void
LBDecision::shutdown()
{
  char buf[20];

  //then if we do, flush all
  execute("iptables -t mangle -F PREROUTING");

  //remove the policy entries
  InterfaceMarkIter iter = _iface_mark_coll.begin();
  while (iter != _iface_mark_coll.end()) {
    sprintf(buf,"%d",iter->second);

    execute(string("ip rule del fwmark ") + buf);
    ++iter;
  }
}

/**
 *
 *
 **/
void
LBDecision::execute(string cmd)
{
  if (_debug) {
    cout << "LBDecision::execute(): applying command to system: " << cmd << endl;
    syslog(LOG_DEBUG, "LBDecision::execute(): applying command to system: %s",cmd.c_str());
  }
  
  FILE *f = popen(cmd.c_str(), "w");
  if (f) {
    if (pclose(f) != 0) {
      if (_debug) {
	cerr << "LBDecision::execute(): error executing command: " << cmd << endl;
      }
      syslog(LOG_ERR, "Error executing system command: %s", cmd.c_str());
    }
  }
  else {
    if (_debug) {
      cerr << "LBDecision::execute(): error executing command: " << cmd << endl;
    }
    syslog(LOG_ERR, "Error executing system command: %s", cmd.c_str());
  }
}

map<int,float> 
LBDecision::get_new_weights(LBData &data, LBRule &rule)
{
  map<int,float> weights;
  int group = 0;
  int ct = 1;
  LBRule::InterfaceDistIter iter = rule._iface_dist_coll.begin();
  while (iter != rule._iface_dist_coll.end()) {
    if (_debug) {
      cout << "LBDecision::get_new_weights(): " << iter->first << " is active: " << (data.is_active(iter->first) ? "true" : "false") << endl;
    }
    if (data.is_active(iter->first)) {
      weights.insert(pair<int,float>(ct,iter->second));
      group += iter->second;
    }
    ++ct;
    ++iter;
  }
  
  //now weight the overall distribution
  map<int,float>::iterator w_iter = weights.begin();
  while (w_iter != weights.end()) {
    float w = float(w_iter->second) / float(group);
    group -= w_iter->second;   //I THINK THIS NEEDS TO BE ADJUSTED TO THE OVERALL REMAINING VALUES. which is this...
    w_iter->second = w;
    ++w_iter;
  }

  return weights;
}

/**
 *
 *
 **/
string
LBDecision::get_application_cmd(LBRule &rule)
{
  string filter;

  if (rule._in_iface.empty() == false) {
    filter += "-i " + rule._in_iface + " ";
  }

  if (rule._proto.empty() == false) {
    filter += "--proto " + rule._proto + " ";
  }
  
  if (rule._proto == "icmp") {
    filter += "--icmp-type any ";
  }

  if (rule._s_addr.empty() == false) {
    bool negate_flag = false;
    string tmp(rule._s_addr);
    if (tmp.find("!") != string::npos) {
      negate_flag = true;
      tmp = tmp.substr(1,tmp.length()-1);
    }

    if (tmp.find("-") != string::npos) {
      if (negate_flag) {
	filter += "-m iprange ! --src-range " + tmp + " ";
      }
      else {
	filter += "-m iprange --src-range " + tmp + " ";
      }
    }
    else {
      if (negate_flag) {
	filter += "--source ! " + tmp + " ";
      }
      else {
	filter += "--source " + tmp + " ";
      }
    }
  }
  
  if (rule._d_addr.empty() == false) {
    string tmp(rule._d_addr);
    if (tmp.find("!") != string::npos) {
      tmp = "! " + tmp.substr(1,tmp.length()-1);
    }

    if (tmp.find("-") != string::npos) {
      filter += "-m iprange --dst-range " + tmp + " ";
    }
    else {
      filter += "--destination " + tmp + " ";
    }
  }

  if (rule._proto == "udp" || rule._proto == "tcp") {
    if (rule._s_port.empty() == false && rule._s_port_ipt.empty() == true) {
      filter += "-m multiport --source-port " + rule._s_port + " ";
    }
    else if (rule._s_port_ipt.empty() == false) {
      filter += rule._s_port_ipt + " ";
    }

    if (rule._d_port.empty() == false && rule._d_port_ipt.empty() == true) {
      filter += "-m multiport --destination-port " + rule._d_port + " ";
    }
    else if (rule._d_port_ipt.empty() == false) {
      filter += rule._d_port_ipt + " ";
    }
  }

  return filter;
}
