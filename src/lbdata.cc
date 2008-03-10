/*
 * Module: lbdata.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <sys/time.h>
#include <time.h>
#include <iostream>

#include "lbdata.hh"

int LBHealthHistory::_buffer_size = 10;

/**
 *
 *
 **/
void
LBHealth::put(int rtt)
{
  int activity_ct = _hresults.push(rtt);

  if (rtt == -1) {
    if (activity_ct >= _failure_ct) {
      if (_is_active == true) {
	_state_changed = true;
      }
      _is_active = false;
    }
  }
  else {
    if (activity_ct >= _success_ct) {
      if (_is_active == false) {
	_state_changed = true;
      }
      _is_active = true;
    }
  }
}

/**
 *
 *
 **/
LBHealthHistory::LBHealthHistory(int buffer_size) :
  _last_success(0),
  _last_failure(0),
  _failure_count(0),
  _index(0)
{
  _resp_data.resize(10);

  for (int i = 0; i < _buffer_size; ++i) {
    _resp_data[i] = 0;
  }
}



/**
 *
 *
 **/
int
LBHealthHistory::push(int rtt)
{
  struct timeval tv;
  gettimeofday(&tv,NULL);

  if (rtt == -1) {
    _last_failure = tv.tv_sec;
    ++_failure_count;
  }
  else {
    _failure_count = 0;
    _last_success = tv.tv_sec;
  }

  _resp_data[_index % _buffer_size] = rtt;
  ++_index;

  //compute count of sequence of same responses
  int ct = 0;
  int start_index = (_index - 1) % _buffer_size;
  for (int i = 0; i < _buffer_size; ++i) {
    int index = start_index - i;
    if (index < 0) {
      //handle wrap around of index here
      index = _buffer_size - index;
    }

    if (_resp_data[index] == -1 && rtt == -1) {
      ++ct;
    }
    else if (_resp_data[index] != -1 && rtt != -1) {
      ++ct;
    }
    else {
      break;
    }
  }
  
  return ct;
}

/**
 *
 *
 **/
bool
LBData::is_active(const string &iface) 
{
  InterfaceHealthIter iter = _iface_health_coll.find(iface);
  if (iter != _iface_health_coll.end()) {
    return iter->second._is_active;
  }
  return false;
}

/**
 *
 *
 **/
void
LBData::dump()
{

  cout << "health" << endl;
  LBData::InterfaceHealthIter h_iter = _iface_health_coll.begin();
  while (h_iter != _iface_health_coll.end()) {
    cout << "  " << h_iter->first << endl;
    cout << "    " << h_iter->second._success_ct << endl;
    cout << "    " << h_iter->second._failure_ct << endl;
    cout << "    " << h_iter->second._ping_target << endl;
    cout << "    " << h_iter->second._ping_resp_time << endl;
    ++h_iter;
  }

  cout << endl << "wan" << endl;
  LBData::LBRuleIter r_iter = _lb_rule_coll.begin();
  while (r_iter != _lb_rule_coll.end()) {
    cout << "  rule: " << r_iter->first << endl;
    cout << "    " << r_iter->second._proto << endl;
    cout << "    " << r_iter->second._s_addr << endl;
    cout << "    " << r_iter->second._s_net << endl;
    cout << "    " << r_iter->second._s_port << endl;

    cout << "    " << r_iter->second._d_addr << endl;
    cout << "    " << r_iter->second._d_net << endl;
    cout << "    " << r_iter->second._d_port << endl;
    
    LBRule::InterfaceDistIter ri_iter = r_iter->second._iface_dist_coll.begin();
    while (ri_iter != r_iter->second._iface_dist_coll.end()) {
      cout << "      interface: " << ri_iter->first << endl;
      cout << "      weight: " << ri_iter->second << endl;
      ++ri_iter;
    }
    ++r_iter;
  }
  cout << "end dump" << endl;
}

/**
 *
 *
 **/
bool
LBData::state_changed()
{
  LBData::InterfaceHealthIter h_iter = _iface_health_coll.begin();
  while (h_iter != _iface_health_coll.end()) {
    if (h_iter->second.state_changed()) {
      return true;
    }
    ++h_iter;
  }
  return false;
}

/**
 *
 *
 **/
void
LBData::reset_state_changed()
{
  LBData::InterfaceHealthIter h_iter = _iface_health_coll.begin();
  while (h_iter != _iface_health_coll.end()) {
    h_iter->second._state_changed = false;
    ++h_iter;
  }
}
