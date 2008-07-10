/*
 * Module: lbdecision.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/ioctl.h>
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

  string stdout;
  //set up special nat rules
  execute(string("iptables -t nat -N WANLOADBALANCE"), stdout);
  execute(string("iptables -t nat -F WANLOADBALANCE"), stdout);
  execute(string("iptables -t nat -D POSTROUTING -j WANLOADBALANCE"), stdout);
  execute(string("iptables -t nat -A POSTROUTING -j WANLOADBALANCE"), stdout);

  //set up the conntrack table
  execute(string("iptables -t raw -N NAT_CONNTRACK"), stdout);
  execute(string("iptables -t raw -F NAT_CONNTRACK"), stdout);
  execute(string("iptables -t raw -A NAT_CONNTRACK -j ACCEPT"), stdout);
  execute(string("iptables -t raw -D PREROUTING 1"), stdout);
  execute(string("iptables -t raw -I PREROUTING 1 -j NAT_CONNTRACK"), stdout);


  LBData::InterfaceHealthIter iter = lbdata._iface_health_coll.begin();
  while (iter != lbdata._iface_health_coll.end()) {
    string iface = iter->first;

    sprintf(buf,"%d",ct);

    execute(string("iptables -t mangle -N ISP_") + buf, stdout);
    execute(string("iptables -t mangle -F ISP_") + buf, stdout);
    execute(string("iptables -t mangle -A ISP_") + buf + " -j CONNMARK --set-mark " + buf, stdout);
    execute(string("iptables -t mangle -A ISP_") + buf + " -j MARK --set-mark " + buf, stdout);

    //NOTE, WILL NEED A WAY TO CLEAN UP THIS RULE ON RESTART...
    execute(string("iptables -t mangle -A ISP_") + buf + " -j ACCEPT", stdout);

    insert_default(string("ip route replace table ") + buf + " default dev " + iface + " via " + iter->second._nexthop, ct);
    _iface_mark_coll.insert(pair<string,int>(iface,ct));
    
    execute(string("ip rule delete table ") + buf, stdout);

    char hex_buf[40];
    sprintf(hex_buf,"%X",ct);
    execute(string("ip rule add fwmark ") + hex_buf + " table " + buf, stdout);
    
    execute(string("iptables -t nat -A WANLOADBALANCE -m connmark --mark ") + buf + " -j SNAT --to-source " + fetch_iface_addr(iface), stdout);

    ++ct;
    ++iter;
  }
  execute("ip route flush cache", stdout);
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

  string stdout;

  if (_debug) {
    cout << "LBDecision::run(), state changed, applying new rule set" << endl;
  }

  //now reapply the routing tables.
  LBData::InterfaceHealthIter h_iter = lb_data._iface_health_coll.begin();
  while (h_iter != lb_data._iface_health_coll.end()) {
    string route_str;
    InterfaceMarkIter m_iter = _iface_mark_coll.find(h_iter->first);
    if (m_iter != _iface_mark_coll.end()) {
      route_str = m_iter->second;
      
      if (h_iter->second._is_active == true) {
	char buf[40];
	sprintf(buf,"%d",m_iter->second);
	insert_default(string("ip route replace table ") + buf + " default dev " + h_iter->first + " via " + h_iter->second._nexthop, m_iter->second);
      }
      else {
	//right now replace route, but don't delete until race condition is resolved

	//	execute(string("ip route delete ") + route_str);
      }
    }
    ++h_iter;
  }

  //first determine if we need to alter the rule set
  if (!lb_data.state_changed()) {
    return;
  }

  //then if we do, flush all
  execute("iptables -t mangle -F PREROUTING", stdout);

  //and compute the new set and apply
  LBData::LBRuleIter iter = lb_data._lb_rule_coll.begin();
  while (iter != lb_data._lb_rule_coll.end()) {
    map<int,float> weights = get_new_weights(lb_data,iter->second);
    map<int,float>::iterator w_iter = weights.begin();
    //NEED TO HANDLE APPLICATION SPECIFIC DETAILS
    string app_cmd = get_application_cmd(iter->second);

    char fbuf[20],dbuf[20];
    if (weights.empty()) {
      //no rules here!
    }
    else if (weights.size() == 1) {
      sprintf(dbuf,"%d",w_iter->first);
      execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -j ISP_" + dbuf, stdout);
      execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -j CONNMARK --restore-mark", stdout);
    }
    else {
      map<int,float>::iterator w_end = weights.end();
      --w_end;
      while (w_iter != w_end) {      
	sprintf(fbuf,"%f",w_iter->second);
	sprintf(dbuf,"%d",w_iter->first);
	execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	++w_iter;
      }
      //last one is special case, the catch all rule
      ++w_iter;
      sprintf(dbuf,"%d",w_iter->first);
      execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -m state --state NEW -j ISP_" + dbuf, stdout);
      execute(string("iptables -t mangle -A PREROUTING ") + app_cmd + " -j CONNMARK --restore-mark", stdout);
    }
    ++iter;
    continue;
  }
}

/**
 *
 *
 **/
void
LBDecision::shutdown()
{
  string stdout;

  //then if we do, flush all
  execute("iptables -t mangle -F PREROUTING", stdout);

  //remove the policy entries
  InterfaceMarkIter iter = _iface_mark_coll.begin();
  while (iter != _iface_mark_coll.end()) {
    char buf[40];
    sprintf(buf,"%d",iter->second);
    
    execute(string("ip rule del table ") + buf, stdout);

    //need to delete ip rule here as well!

    ++iter;
  }
}

/**
 *
 **/
int
LBDecision::execute(std::string cmd, std::string &stdout, bool read)
{
  int err = 0;

  if (_debug) {
    cout << "LBDecision::execute(): applying command to system: " << cmd << endl;
    syslog(LOG_DEBUG, "LBDecision::execute(): applying command to system: %s",cmd.c_str());
  }
 
  string dir = "w";
  if (read == true) {
    dir = "r";
  }
  FILE *f = popen(cmd.c_str(), dir.c_str());
  if (f) {
    if (read == true) {
      fflush(f);
      char *buf = NULL;
      size_t len = 0;
      size_t read_len = 0;
      while ((read_len = getline(&buf, &len, f)) != -1) {
	stdout += string(buf) + " ";
      }

      if (buf) {
	free(buf);
      }
    }
    err = pclose(f);
  }
  return err;
}

/**
 *
 **/
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
    else {
      weights.insert(pair<int,float>(ct,0.));
    }
    ++ct;
    ++iter;
  }

  if (group == 0) {
    weights.erase(weights.begin(),weights.end());
  }
  else {
    //now weight the overall distribution
    map<int,float>::iterator w_iter = weights.begin();
    while (w_iter != weights.end()) {
      float w = 0.;
      if (w_iter->second > 0.) { //can only be an integer value here
	w = float(w_iter->second) / float(group);
      }
      group -= w_iter->second;   //I THINK THIS NEEDS TO BE ADJUSTED TO THE OVERALL REMAINING VALUES. which is this...
      if (w < .01) {
	weights.erase(w_iter++);
	continue;
      }
      w_iter->second = w;
      ++w_iter;
    }
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


/**
 * Check for the presence of a route entry in the policy table. Note this 
 * should be replaced by netlink in the next release.
 **/
void
LBDecision::insert_default(string cmd, int table)
{
  string stdout;
  char buf[40];
  string showcmd("ip route show table ");
  sprintf(buf,"%d",table);
  showcmd += string(buf);
  execute(showcmd,stdout,true);

  //  cout << "LBDecision::insert_default(stdout): '" << stdout << "'" << endl;

  if (stdout.empty() == true) {
    //    cout << "LBDecision::insert_default(cmd): " << cmd << endl;
    execute(cmd,stdout);
  }
}


/**
 * Fetch interface configuration 
 **/
string
LBDecision::fetch_iface_addr(const string &iface)
{
  struct ifreq ifr;
  int fd;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    syslog(LOG_ERR, "Error obtaining socket");
    return string("");
  }
  
  int ct = 2;
  //try twice to retrieve this before failing
  while (ct > 0) {
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ);
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
      struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
      struct in_addr in;
      in.s_addr = sin->sin_addr.s_addr;
      char *tmp_buf = inet_ntoa(in);
      close(fd);
      return string(tmp_buf);
    }
    usleep(500 * 1000); //.5 second sleep
    --ct;
  }
  close(fd);
  return string("");
}


