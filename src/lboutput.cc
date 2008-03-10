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
LBOutput::write(const LBData &lbdata)
{
  timeval tv;
  gettimeofday(&tv,NULL);

  string wlb_out = _output_path + "/wlb.out";
  string wlb_app_out = _output_path + "/wlb_app.out";

  //open file
  FILE *fp = fopen(wlb_out.c_str(), "w");
  if (fp == NULL) {
    if (_debug) {
      cerr << "Error opening output file: " << wlb_out << endl;
    }
    syslog(LOG_ERR, "wan_lb: error ordering output file %s", wlb_out.c_str());
    return;
  }

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

  string line("Interface\tStatus\tLast Success\tLast Failure\tNum Failure\n");
  fputs(line.c_str(),fp);
  iter = lbdata._iface_health_coll.begin();

  timeval cur_t;
  gettimeofday(&cur_t,NULL);

  while (iter != lbdata._iface_health_coll.end()) {
    line = string(iter->first) + "\t\t" + string(iter->second._is_active?"active":"failed") + "\t";
    char btmp[256];
    string time_buf;
    
    unsigned long diff_t;
    if (iter->second.last_success() > 0) {
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

    if (iter->second.last_success() == 0) {
      line += string("n/a") + string("\t\t");
    }
    else {
      line += time_buf + string("\t");
      if (time_buf.size() < 6) {
	line += string("\t");
      }
    }

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

    if (iter->second.last_failure() == 0) {
      line += string("n/a") + string("\t\t");
    }
    else {
      line += time_buf + string("\t");
      if (time_buf.size() < 6) {
	line += string("\t");
      }
    }

    //now failure count
    sprintf(btmp, "%ld", iter->second._hresults._failure_count);
    line += string(btmp);

    line += "\n";

    fputs(line.c_str(),fp);
    ++iter;
  }
  fclose(fp);


  //dump out the application data
  LBData::LBRuleConstIter r_iter = lbdata._lb_rule_coll.begin();
  while (r_iter != lbdata._lb_rule_coll.end()) {
    if (_debug) {
      cout << "squirt out results here." << endl;
    }
    ++r_iter;
  }
}
