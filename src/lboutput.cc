/*
 * Module: lboutput.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <sys/time.h>
#include <time.h>
#include <syslog.h>

#include <iostream>

#include "lbdata.hh"
#include "lboutput.hh"

void
LBOutput::shutdown()
{
  string wlb_out = _output_path + "/wlb.out";
  unlink(wlb_out.c_str());
}

void
LBOutput::write(const LBData &lbdata)
{
  timeval tv;
  gettimeofday(&tv,NULL);

  string wlb_out = _output_path + "/wlb.out";

  //open file
  FILE *fp = fopen(wlb_out.c_str(), "w");
  if (fp == NULL) {
    if (_debug) {
      cerr << "Error opening output file: " << wlb_out << endl;
    }
    syslog(LOG_ERR, "wan_lb: error ordering output file %s", wlb_out.c_str());
    return;
  }

  if (_debug) {
    //dump out the health data
    LBData::InterfaceHealthConstIter iter = lbdata._iface_health_coll.begin();
    while (iter != lbdata._iface_health_coll.end()) {
      if (_debug) {
	cout << iter->first << " "; //interface
	cout << string(iter->second._is_active ? "true" : "false") << " "; //status
	cout << tv.tv_sec - iter->second.last_success() << " "; //last success
	cout << tv.tv_sec - iter->second.last_failure() << " "; //last failure
	cout << endl;
      }
      ++iter;
    }
  }

  string space("  ");
  timeval cur_t;
  gettimeofday(&cur_t,NULL);
  LBData::InterfaceHealthConstIter iter = lbdata._iface_health_coll.begin();
  while (iter != lbdata._iface_health_coll.end()) {
    string line = string("Interface:  ") + iter->first + "\n";
    line += space + string("Status:  ") + string(iter->second._is_active?"active":"failed") + "\n";

    const time_t t = (time_t)(iter->second._last_time_state_changed);
    char *tbuf = ctime(&t);

    line += space + string("Last Status Change:  ") + string(tbuf);

    LBHealth::TestConstIter titer = iter->second._test_coll.begin();
    while (titer != iter->second._test_coll.end()) {
      string target = titer->second->_target;
      if (target.empty()) {
	if (iter->second._nexthop == "dhcp") {
	  target = iter->second._dhcp_nexthop;
	}
	else {
	  target = iter->second._nexthop;
	}
      }
      
      if (titer->second->_state == LBTest::K_NONE) {
	line += space + string("*Target:  ") + target + "\n";
      }
      else if (titer->second->_state == LBTest::K_FAILURE) {
	line += space + string("-Target:  ") + target + "\n";
      }
      else if (titer->second->_state == LBTest::K_SUCCESS) {
	line += space + string("+Target:  ") + target + "\n";
      }

      ++titer;
    }

    char btmp[256];
    string time_buf;
    
    unsigned long diff_t;
    //the last condition is to handle a system time change...
    if (iter->second.last_success() > 0 && (cur_t.tv_sec > iter->second.last_success())) {
      diff_t = cur_t.tv_sec - iter->second.last_success();
    }
    else {
      diff_t = 0;
    }

    unsigned long days,hours,mins,secs;

    days = diff_t / (60*60*24);
    if (days > 0) {
      diff_t -= days * (60*60*24);
      sprintf(btmp,"%ld",days);
      time_buf += string(btmp) + "d";
    }

    hours = diff_t / (60*60);
    if (hours > 0) {
      diff_t -= hours * (60*60);
      sprintf(btmp,"%ld",hours);
      time_buf += string(btmp) + "h";
    }

    mins = diff_t / (60);
    if (mins > 0) {
      diff_t -= mins * (60);
      sprintf(btmp,"%ld",mins);
      time_buf += string(btmp) + "m";
    }

    secs = diff_t;
    sprintf(btmp,"%ld",secs);
    time_buf += string(btmp) + "s";

    string success_time;
    if (iter->second.last_success() == 0) {
      success_time = string("n/a") + string("\t\t");
    }
    else {
      success_time = time_buf + string("\t");
    }

    line += space + space + string("Last Ping Success:  ") + success_time + "\n";

    time_buf = "";

    if (iter->second.last_failure() > 0) {
      diff_t = cur_t.tv_sec - iter->second.last_failure();
    }
    else {
      diff_t = 0;
    }

    days = diff_t / (60*60*24);
    if (days > 0) {
      diff_t -= days * (60*60*24);
      sprintf(btmp,"%ld",days);
      time_buf += string(btmp) + "d";
    }

    hours = diff_t / (60*60);
    if (hours > 0) {
      diff_t -= hours * (60*60);
      sprintf(btmp,"%ld",hours);
      time_buf += string(btmp) + "h";
    }

    mins = diff_t / (60);
    if (mins > 0) {
      diff_t -= mins * (60);
      sprintf(btmp,"%ld",mins);
      time_buf += string(btmp) + "m";
    }

    secs = diff_t;
    sprintf(btmp,"%ld",secs);
    time_buf += string(btmp) + "s";

    string failure_time;
    if (iter->second.last_failure() == 0) {
      failure_time = string("n/a") + string("\t\t");
    }
    else {
      failure_time = time_buf + string("\t");
    }

    line += space + space + string("Last Ping Failure:  ") + failure_time + "\n";


    //now failure count
    sprintf(btmp, "%ld", iter->second._hresults._failure_count);

    line += space + space + string("# Ping Failure(s):  ") + string(btmp) + "\n\n";

    fputs(line.c_str(),fp);
    ++iter;
  }
  fclose(fp);


  //dump out the application data
  if (_debug) {
    LBData::LBRuleConstIter r_iter = lbdata._lb_rule_coll.begin();
    while (r_iter != lbdata._lb_rule_coll.end()) {
      cout << "squirt out results here." << endl;
      ++r_iter;
    }
  }
}
