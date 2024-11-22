/*
 * Module: lbdecision.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <iostream>
#include "lbdata.hh"
#include "lbdecision.hh"

#define IPT_MARK_OFFSET 0xc8

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
  if (lbdata._disable_source_nat == false) {
    execute(string("iptables -t nat -N WANLOADBALANCE"), stdout);
    execute(string("iptables -t nat -F WANLOADBALANCE"), stdout);
    execute(string("iptables -t nat -D VYATTA_PRE_SNAT_HOOK -j WANLOADBALANCE"), stdout);
    execute(string("iptables -t nat -I VYATTA_PRE_SNAT_HOOK 1 -j WANLOADBALANCE"), stdout);
  }
  //set up the conntrack table
  execute(string("iptables -t raw -N WLB_CONNTRACK"), stdout);
  execute(string("iptables -t raw -F WLB_CONNTRACK"), stdout);
  execute(string("iptables -t raw -A WLB_CONNTRACK -j ACCEPT"), stdout);

  execute(string("iptables -t raw -D PREROUTING -j WLB_CONNTRACK"), stdout);

  int index = find_iptables_index("raw","PREROUTING","VYATTA_CT_PREROUTING_HOOK");
  ++index;
  sprintf(buf,"%d",index);
  execute(string("iptables -t raw -I PREROUTING ") + buf + " -j WLB_CONNTRACK", stdout);


  if (lbdata._enable_local_traffic == true) {
    execute(string("iptables -t raw -D OUTPUT -j WLB_CONNTRACK"), stdout);

    int index = find_iptables_index("raw","OUTPUT","VYATTA_CT_OUTPUT_HOOK");
    ++index;
    sprintf(buf,"%d",index);
    execute(string("iptables -t raw -I OUTPUT ") + buf + " -j WLB_CONNTRACK", stdout);

  }
  //set up mangle table
  execute(string("iptables -t mangle -N WANLOADBALANCE_PRE"), stdout);
  execute(string("iptables -t mangle -F WANLOADBALANCE_PRE"), stdout);
  execute(string("iptables -t mangle -A WANLOADBALANCE_PRE -j ACCEPT"), stdout);
  execute(string("iptables -t mangle -D PREROUTING -j WANLOADBALANCE_PRE"), stdout);
  execute(string("iptables -t mangle -I PREROUTING 1 -j WANLOADBALANCE_PRE"), stdout);
  if (lbdata._enable_local_traffic == true) {
    execute(string("iptables -t mangle -N WANLOADBALANCE_OUT"), stdout);
    execute(string("iptables -t mangle -F WANLOADBALANCE_OUT"), stdout);
    execute(string("iptables -t mangle -A WANLOADBALANCE_OUT -j ACCEPT"), stdout);
    execute(string("iptables -t mangle -D OUTPUT -j WANLOADBALANCE_OUT"), stdout);
    execute(string("iptables -t mangle -I OUTPUT 1 -j WANLOADBALANCE_OUT"), stdout);
  }

  LBData::InterfaceHealthIter iter = lbdata._iface_health_coll.begin();
  while (iter != lbdata._iface_health_coll.end()) {
    string iface = iter->first;
    
    int ct = iter->second._interface_index + IPT_MARK_OFFSET;

    sprintf(buf,"%d",ct);

    execute(string("iptables -t mangle -N ISP_") + iface, stdout);
    execute(string("iptables -t mangle -F ISP_") + iface, stdout);
    execute(string("iptables -t mangle -A ISP_") + iface + " -j CONNMARK --set-mark " + buf, stdout);
    execute(string("iptables -t mangle -A ISP_") + iface + " -j MARK --set-mark " + buf, stdout);

    //NOTE, WILL NEED A WAY TO CLEAN UP THIS RULE ON RESTART...
    execute(string("iptables -t mangle -A ISP_") + iface + " -j ACCEPT", stdout);

    if (lbdata._sticky_inbound_connections == true) {
      //Mark incoming connections so that return packets go back on the same interface
      execute(string("iptables -t mangle -N ISP_") + iface + "_IN", stdout);
      execute(string("iptables -t mangle -F ISP_") + iface + "_IN", stdout);
      execute(string("iptables -t mangle -A ISP_") + iface + "_IN -j CONNMARK --set-mark " + buf, stdout);
      execute(string("iptables -t mangle -I PREROUTING -i ") + iface + " -m state --state NEW -j ISP_" + iface + "_IN", stdout);
    }

    //need to force the entry on restart as the configuration may have changed.
    if (iter->second._nexthop == "dhcp") {
      if (iter->second._dhcp_nexthop.empty() == false) {
	execute(string("ip route replace table ") + buf + " default dev " + iface + " via " + iter->second._dhcp_nexthop, stdout);
      }
    }
    else {
      execute(string("ip route replace table ") + buf + " default dev " + iface + " via " + iter->second._nexthop, stdout);
    }

    execute(string("ip rule delete table ") + buf, stdout);

    char hex_buf[40];
    sprintf(hex_buf,"%#X",ct);
    execute(string("ip rule add fwmark ") + hex_buf + " table " + buf, stdout);

    if (lbdata._disable_source_nat == false) {
      string new_addr = fetch_iface_addr(iface);
      int err = execute(string("iptables -t nat -A WANLOADBALANCE -m connmark --mark ") + buf + " -j SNAT --to-source " + new_addr, stdout);
      if (err == 0) {
	iter->second._address = new_addr;
      }
      sleep(1); //when creating the first entry it appears that snat will kill an existing connection if multiple commands are issues too quickly
    }
    ++iter;
  }
  execute("ip route flush cache", stdout);
}

/**
 * Need to do two things here, check the interfaces for new nexthops and update the routing tables,
 * and get a new address to update the source nat with.
 *
 **/
void
LBDecision::update_paths(LBData &lbdata)
{
  string stdout;
  //first let's remove the entry
  LBData::InterfaceHealthIter iter = lbdata._iface_health_coll.begin();
  while (iter != lbdata._iface_health_coll.end()) {
    if (iter->second._is_active == true) {
      string iface = iter->first;
      string new_addr = fetch_iface_addr(iface);
      char buf[20];
      sprintf(buf,"%d",iter->second._interface_index + IPT_MARK_OFFSET);
      
      //now let's update the nexthop here in the route table
      if (iter->second._nexthop == "dhcp") {
	if (iter->second._dhcp_nexthop.empty() == false) {
	  insert_default(iter->second, iter->second._dhcp_nexthop);
	}
      }
      else {
	insert_default(iter->second, iter->second._nexthop);
      }
      
      if (lbdata._disable_source_nat == false) {
	if (new_addr != iter->second._address) {
	  int err = 0;
	  if (iter->second._address.empty() == false) {
	    err = execute(string("iptables -t nat -D WANLOADBALANCE -m connmark --mark ") + buf + " -j SNAT --to-source " + iter->second._address, stdout);
	  }
	  if (new_addr.empty() == false) {
	    err |= execute(string("iptables -t nat -A WANLOADBALANCE -m connmark --mark ") + buf + " -j SNAT --to-source " + new_addr, stdout);
	  }
	  if (err == 0) { //only set if both are 0
	    iter->second._address = new_addr;
	  }
	}
      }
    }
    ++iter;
  }
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

  update_paths(lb_data);

  //first determine if we need to alter the rule set
  map<string,string> state_changed_coll;
  state_changed_coll = lb_data.state_changed();
  if (state_changed_coll.empty() == true) {
    return;
  }
  else { 
    //state has changed execute script now
    
    map<string,string>::iterator iter = state_changed_coll.begin();
    while (iter != state_changed_coll.end()) {
      //set state
      //set interface
      if (lb_data._hook.empty() == false) {
	setenv("WLB_INTERFACE_NAME",iter->first.c_str(),1);
	setenv("WLB_INTERFACE_STATE",iter->second.c_str(),1);
	
	syslog(LOG_WARNING, "executing script: %s",lb_data._hook.c_str());
	
	execute(lb_data._hook, stdout);
	//unset state
	//unset interface
	unsetenv("WLB_INTERFACE_NAME");
	unsetenv("WLB_INTERFACE_STATE");
      }
      ++iter;
    }
  }

  //then if we do, flush all
  execute("iptables -t mangle -F WANLOADBALANCE_PRE", stdout);
  if (lb_data._enable_local_traffic == true) {
    execute("iptables -t mangle -F WANLOADBALANCE_OUT", stdout);
    execute("iptables -t mangle -A WANLOADBALANCE_OUT -m mark ! --mark 0 -j ACCEPT", stdout); //avoid packets set in prerouting table
    execute("iptables -t mangle -A WANLOADBALANCE_OUT --proto icmp --icmp-type any -j ACCEPT", stdout); //avoid packets set in prerouting table
    execute("iptables -t mangle -A WANLOADBALANCE_OUT --source 127.0.0.1/8 --destination 127.0.0.1/8 -j ACCEPT", stdout); //avoid packets set in prerouting table
  }

  //new request, bug 4112. flush conntrack tables if configured
  if (lb_data._flush_conntrack == true) {
    //execute("conntrack -F", stdout);
    // Bug T1311 with "conntrack -F" and "conntrack-sync"
    execute("conntrack --delete", stdout);
    execute("conntrack -F expect", stdout);
  }

  //and compute the new set and apply
  LBData::LBRuleIter iter = lb_data._lb_rule_coll.begin();
  while (iter != lb_data._lb_rule_coll.end()) {
    //NEED TO HANDLE APPLICATION SPECIFIC DETAILS
    string app_cmd = get_application_cmd(iter->second);
    string app_cmd_local = get_application_cmd(iter->second,true,iter->second._exclude);

    if (iter->second._exclude == true) {
      execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " -j ACCEPT", stdout);
      if (lb_data._enable_local_traffic == true) {
	execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " -j ACCEPT", stdout);
      }
    }
    else {
      map<string,float> weights = get_new_weights(lb_data,iter->second);
      
      if (weights.empty()) {
	//no rules here!
      }
      else {
	char rule_str[20];
	sprintf(rule_str,"%d",iter->first);

	if (iter->second._limit) {
	  string limit_cmd = get_limit_cmd(iter->second);
	  execute(string("iptables -t mangle -N WANLOADBALANCE_PRE_LIMIT_") + rule_str, stdout);
	  execute(string("iptables -t mangle -F WANLOADBALANCE_PRE_LIMIT_") + rule_str, stdout);
	  execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " " + limit_cmd + " -j WANLOADBALANCE_PRE_LIMIT_" + rule_str, stdout);

	  if (lb_data._enable_local_traffic == true) {
	    execute(string("iptables -t mangle -N WANLOADBALANCE_OUT_LIMIT_") + rule_str, stdout);
	    execute(string("iptables -t mangle -F WANLOADBALANCE_OUT_LIMIT_") + rule_str, stdout);	  
	    execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " " + limit_cmd + " -j WANLOADBALANCE_OUT_LIMIT_" + rule_str, stdout);
	  }
	}

	char fbuf[20],dbuf[80];
	map<string,float>::iterator w_iter = weights.begin();
	for (w_iter = weights.begin(); w_iter != (--weights.end()); w_iter++) {
	  sprintf(fbuf,"%f",w_iter->second);
	  sprintf(dbuf,"%s",w_iter->first.c_str());
	  if (iter->second._enable_source_based_routing) {
	    if (iter->second._limit) {
	      //fill in limit statement here
	      execute(string("iptables -t mangle -A WANLOADBALANCE_PRE_LIMIT_") + rule_str + " -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      if (lb_data._enable_local_traffic == true) {
		execute(string("iptables -t mangle -A WANLOADBALANCE_OUT_LIMIT_") + rule_str + " -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      }
	    }
	    else {
	      execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      if (lb_data._enable_local_traffic == true) {
		execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      }
	    }
	  }
	  else {
	    if (iter->second._limit) {
	      //fill in limit statement here
	      execute(string("iptables -t mangle -A WANLOADBALANCE_PRE_LIMIT_") + rule_str + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      if (lb_data._enable_local_traffic == true) {
		execute(string("iptables -t mangle -A WANLOADBALANCE_OUT_LIMIT_") + rule_str + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      }
	    }
	    else {
	      execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      if (lb_data._enable_local_traffic == true) {
		execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " -m state --state NEW -m statistic --mode random --probability " + fbuf + " -j ISP_" + dbuf, stdout);
	      }
	    }
	  }
	}
	sprintf(dbuf,"%s",(--weights.end())->first.c_str());
	if (iter->second._enable_source_based_routing) {
	  if (iter->second._limit) {
	    //fill in limit statement here
	    execute(string("iptables -t mangle -A WANLOADBALANCE_PRE_LIMIT_") + rule_str + " -j ISP_" + dbuf, stdout);
	    execute(string("iptables -t mangle -A WANLOADBALANCE_PRE_LIMIT_") + rule_str + " -j ACCEPT", stdout);
	    if (lb_data._enable_local_traffic == true) {
	      execute(string("iptables -t mangle -A WANLOADBALANCE_OUT_LIMIT_") + rule_str + " -j ISP_" + dbuf, stdout);
	      execute(string("iptables -t mangle -A WANLOADBALANCE_OUT_LIMIT_") + rule_str + " -j ACCEPT", stdout);
	    }
	  }
	  else {
	    execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " -j ISP_" + dbuf, stdout);
	    if (lb_data._enable_local_traffic == true) {
	      execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " -j ISP_" + dbuf, stdout);
	    }
	  }
	  
	}
	else {
	  if (iter->second._limit) {
	    //fill in limit statement here
	    execute(string("iptables -t mangle -A WANLOADBALANCE_PRE_LIMIT_") + rule_str + " -m state --state NEW -j ISP_" + dbuf, stdout);
	    if (lb_data._enable_local_traffic == true) {
	      execute(string("iptables -t mangle -A WANLOADBALANCE_OUT_LIMIT_") + rule_str + " -m state --state NEW -j ISP_" + dbuf, stdout);
	    }
	  }
	  else {
	    execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " -m state --state NEW -j ISP_" + dbuf, stdout);
	    if (lb_data._enable_local_traffic == true) {
	      execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " -m state --state NEW -j ISP_" + dbuf, stdout);
	    }
	  }
	}
	execute(string("iptables -t mangle -A WANLOADBALANCE_PRE ") + app_cmd + " -j CONNMARK --restore-mark", stdout);
	if (lb_data._enable_local_traffic == true) {
	  execute(string("iptables -t mangle -A WANLOADBALANCE_OUT ") + app_cmd_local + " -j CONNMARK --restore-mark", stdout);
	}
      }
    }
    ++iter;
  }
}

/**
 *
 *
 **/
void
LBDecision::shutdown(LBData &data)
{
  string stdout;

  //then if we do, flush all
  execute("iptables -t mangle -D PREROUTING -j WANLOADBALANCE_PRE", stdout);
  execute("iptables -t mangle -F WANLOADBALANCE_PRE", stdout);
  execute("iptables -t mangle -X WANLOADBALANCE_PRE", stdout);
  if (data._enable_local_traffic == true) {
    execute("iptables -t mangle -D OUTPUT -j WANLOADBALANCE_OUT", stdout);
    execute("iptables -t mangle -F WANLOADBALANCE_OUT", stdout);
    execute("iptables -t mangle -X WANLOADBALANCE_OUT", stdout);
  }
  LBData::LBRuleIter iter = data._lb_rule_coll.begin();
  while (iter != data._lb_rule_coll.end()) {
    if (iter->second._limit) {
      char rule_str[20];
      sprintf(rule_str,"%d",iter->first);
      execute(string("iptables -t mangle -F WANLOADBALANCE_PRE_LIMIT_") + rule_str,stdout);
      execute(string("iptables -t mangle -X WANLOADBALANCE_PRE_LIMIT_") + rule_str,stdout);
      if (data._enable_local_traffic == true) {
	execute(string("iptables -t mangle -F WANLOADBALANCE_OUT_LIMIT_") + rule_str,stdout);
	execute(string("iptables -t mangle -X WANLOADBALANCE_OUT_LIMIT_") + rule_str,stdout);
      }
    }
    ++iter;
  }

  //clear out nat as well
  execute("iptables -t nat -F WANLOADBALANCE", stdout);
  execute("iptables -t nat -D VYATTA_PRE_SNAT_HOOK -j WANLOADBALANCE", stdout);

  //clear out conntrack hooks
  execute(string("iptables -t raw -D PREROUTING -j WLB_CONNTRACK"), stdout);
  if (data._enable_local_traffic == true) {
    execute(string("iptables -t raw -D OUTPUT -j WLB_CONNTRACK"), stdout);
  }
  execute(string("iptables -t raw -F WLB_CONNTRACK"), stdout);
  execute(string("iptables -t raw -X WLB_CONNTRACK"), stdout);

  //remove the policy entries
  LBData::InterfaceHealthIter h_iter = data._iface_health_coll.begin();
  while (h_iter != data._iface_health_coll.end()) {
    char buf[40];
    sprintf(buf,"%d",h_iter->second._interface_index + IPT_MARK_OFFSET);
    
    execute(string("ip rule del table ") + buf, stdout);
    execute(string("ip route flush table ") + buf, stdout);

    //need to delete ip rule here as well!

    //clean up mangle final entries here
    execute(string("iptables -t mangle -F ISP_") + h_iter->first,stdout);
    execute(string("iptables -t mangle -X ISP_") + h_iter->first,stdout);

    if (data._sticky_inbound_connections == true) {
      execute(string("iptables -t mangle -D PREROUTING -i ") + h_iter->first + " -m state --state NEW -j ISP_" + h_iter->first + "_IN", stdout);
      execute(string("iptables -t mangle -F ISP_") + h_iter->first + "_IN",stdout);
      execute(string("iptables -t mangle -X ISP_") + h_iter->first + "_IN",stdout);
    }

    ++h_iter;
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
      while ((read_len = getline(&buf, &len, f)) != (size_t)-1) {
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
map<string,float> 
LBDecision::get_new_weights(LBData &data, LBRule &rule)
{
  map<string,float> weights;
  int group = 0;
  LBRule::InterfaceDistIter iter = rule._iface_dist_coll.begin();
  while (iter != rule._iface_dist_coll.end()) {
    if (_debug) {
      cout << "LBDecision::get_new_weights(): " << iter->first << " is active: " << (data.is_active(iter->first) ? "true" : "false") << endl;
    }

    int ct = 0;
    LBData::InterfaceHealthIter h_iter = data._iface_health_coll.find(iter->first);
    if (h_iter != data._iface_health_coll.end()) {
      ct = h_iter->second._interface_index;
    }

    if (rule._failover == true) { //add single entry if active
      if (data.is_active(iter->first)) {
	//select the active interface that has the highest weight
	if (iter->second > group) {
	  map<string,float>::iterator w_iter = weights.begin();
	  while (w_iter != weights.end()) {
	    w_iter->second = 0.; //zero out previous weight
	    ++w_iter;
	  }
	}
	weights.insert(pair<string,float>(iter->first,iter->second));
	group = iter->second;
      }
      else {
	weights.insert(pair<string,float>(iter->first,0.));	
      }
    }
    else {
      if (data.is_active(iter->first)) {
	weights.insert(pair<string,float>(iter->first,iter->second));
	group += iter->second;
      }
      else {
	weights.insert(pair<string,float>(iter->first,0.));
      }
    }
    ++iter;
  }

  if (group == 0) {
    weights.erase(weights.begin(),weights.end());
  }
  else {
    //now weight the overall distribution
    map<string,float>::iterator w_iter = weights.begin();
    while (w_iter != weights.end()) {
      float w = 0.;
      if (w_iter->second > 0.) { //can only be an integer value here
	w = float(w_iter->second) / float(group);
      }
      group -= (int)w_iter->second;   //I THINK THIS NEEDS TO BE ADJUSTED TO THE OVERALL REMAINING VALUES. which is this...
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
LBDecision::get_application_cmd(LBRule &rule, bool local, bool exclude)
{
  string filter;

  if (rule._in_iface.empty() == false) {
    if (local == true) {
      if (exclude == false) {
	filter += " ! -o " + rule._in_iface + " ";
      }
    }
    else {
      filter += "-i " + rule._in_iface + " ";
    }
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
    bool negate_flag = false;
    string tmp(rule._d_addr);
    if (tmp.find("!") != string::npos) {
      negate_flag = true;
      tmp = tmp.substr(1,tmp.length()-1);
    }

    if (tmp.find("-") != string::npos) {
      if (negate_flag) {
	filter += "-m iprange ! --dst-range " + tmp + " ";
      }
      else {
	filter += "-m iprange --dst-range " + tmp + " ";
      }
    }
    else {
      if (negate_flag) {
	filter += "! --destination  " + tmp + " ";
      }
      else {
	filter += "--destination " + tmp + " ";
      }
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
LBDecision::insert_default(LBHealth &h, string &nexthop)
{
  //if found will return something of the form:
  // "default via 10.3.0.1 dev eth0"

  //retrieve route entry
  string stdout;
  char buf[40];
  sprintf(buf,"%d",h._interface_index + IPT_MARK_OFFSET);
  string default_route = string("ip route replace table ") + buf + " default dev " + h._interface + " via " + nexthop;
  string showcmd("ip route show table ");
  showcmd += string(buf);
  execute(showcmd,stdout,true);
  if (stdout.empty() == false) {
    //compare string:
    if (stdout.find(nexthop) == string::npos || stdout.find(h._interface) == string::npos) { //compare expected string
      int err = execute(default_route,stdout); //apply entry because this doesn't match
      if (err != 0) {
	syslog(LOG_WARNING, string("failure to insert default route on active path with this command: " + default_route + ", resp: " + stdout).c_str());
      }
    }
  }
  else {
    int err = execute(default_route,stdout); //apply entry because this doesn't match
    if (err != 0) {
      syslog(LOG_WARNING, string("failure to insert default route on active path with this command: " + default_route + ", resp: " + stdout).c_str());
    }
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

/**
 * Builds out the limit matching criteria
 **/
string
LBDecision::get_limit_cmd(LBRule &rule)
{
  string cmd;
  if (!rule._limit) {
    return cmd;
  }
  //needs to be of the form:
  //-m limit [!] --limit 1/second --limit-burst 5 
  cmd = "-m limit ";
  if (rule._limit_mode) {
    cmd += "! ";
  }
  cmd += string("--limit ") + rule._limit_rate + "/";
  if (rule._limit_period == LBRule::K_SECOND) {
    cmd += "second ";
  }
  else if (rule._limit_period == LBRule::K_MINUTE) {
    cmd += "minute ";
  }
  else {
    cmd += "hour ";
  }
  
  cmd += string("--limit-burst ") + rule._limit_burst;
  return cmd;
}

/**
 *
 **/
int
LBDecision::find_iptables_index(string location, string table, string name)
{
  string stdout;
  string cmd = "iptables -t " + location + " -L " + table;
  int err = execute(cmd, stdout, true);
  if (err != 0) {
    return 1;
  }
  
  size_t loc = stdout.find(name);
  string found_str = stdout.substr(0,loc);
  //now count the number of carriage returns
  loc = 0;
  int ct = 0;
  while ((loc = found_str.find("\n",loc)) != string::npos) {
    ++loc;
    ++ct;
  }
  return ct-1; //offset from headers on command
}
