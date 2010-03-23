/*
 * Module: lbtest.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <sys/time.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <iostream>

#include "rl_str_proc.hh"
#include "lbdata.hh"
#include "lbtest.hh"

int LBTest::_send_icmp_sock = 0;
int LBTest::_send_raw_sock = 0;
int LBTest::_recv_icmp_sock = 0;
bool LBTest::_initialized = false;
bool LBTest::_received = false;
int LBTest::_packet_id = 0;
map<int,PktData> LBTest::_results;

/**
 *
 *
 **/
void
LBTest::init()
{
  if (_initialized == true) {
    return;
  }
  _initialized = true;

  if (_debug) {
    cout << "LBTest::init()" << endl;
  }

  struct protoent *ppe = getprotobyname("icmp");
  _send_icmp_sock = socket(PF_INET, SOCK_RAW, ppe->p_proto);
  if (_send_icmp_sock < 0){
    if (_debug) {
      cerr << "LBTest::init(): no send sock: " << _send_icmp_sock << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to acquired socket");
    _send_icmp_sock = 0;
    return;
  }

  //set options for broadcasting.
  int val = 1;
  //  setsockopt(_send_icmp_sock, SOL_SOCKET, SO_BROADCAST, &val, 4);
  setsockopt(_send_icmp_sock, SOL_SOCKET, SO_REUSEADDR, &val, 4);

  _send_raw_sock = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
  if (_send_raw_sock < 0){
    if (_debug) {
      cerr << "LBTest::init(): no send sock: " << _send_raw_sock << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to acquired socket");
    _send_raw_sock = 0;
    return;
  }

  cout << "send raw sock: " << _send_raw_sock << endl;

  //set options for broadcasting.
  //  setsockopt(_send_raw_sock, SOL_SOCKET, SO_BROADCAST, &val, 4);
  setsockopt(_send_raw_sock, SOL_SOCKET, SO_REUSEADDR, &val, 4);

  struct sockaddr_in addr;
  memset( &addr, 0, sizeof( struct sockaddr_in ));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = 0;

  _recv_icmp_sock = socket(PF_INET, SOCK_RAW, ppe->p_proto);
  if (_recv_icmp_sock < 0) {
    if (_debug) {
      cerr << "LBTest::init(): no recv sock: " << _recv_icmp_sock << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to acquired socket");
    _recv_icmp_sock = 0;
    return;
  }
  if (bind(_recv_icmp_sock, (struct sockaddr*)&addr, sizeof(addr))==-1) {
    if (_debug) {
      cerr << "failed on bind" << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to bind recv sock");
  }
}

/**
 *
 *
 **/
LBTest::~LBTest()
{
  if (_recv_icmp_sock != 0) {
    close(_recv_icmp_sock);
    _recv_icmp_sock = 0;
  }

  if (_send_raw_sock != 0) {
    close(_send_raw_sock);
    _send_raw_sock = 0;
  }

  if (_send_icmp_sock != 0) {
    close(_send_icmp_sock);
    _send_icmp_sock = 0;
  }

}

/**
 *
 *
 **/
void
LBTest::start()
{
  _received = false;
  _results.erase(_results.begin(),_results.end());
}

/**
 *
 *
 **/
int
LBTest::recv(LBHealth &health)
{
  if (_received == false) {
    //let's erase the results first
    //use gettimeofday to calculate time to millisecond
    //then iterate over recv socket and receive and record
    //use sysinfo to make sure we don't get stuck in a loop with timechange
    struct timeval send_time;
    gettimeofday(&send_time,NULL);
    struct sysinfo si;
    sysinfo(&si);
    //for now hardcode to 5 second overall timeout
    unsigned long timeout = si.uptime + 5; //seconds
    unsigned long cur_time = si.uptime;

    int pending_result_ct = _results.size();
    while (cur_time < timeout && pending_result_ct != 0) {
      int id = receive(_recv_icmp_sock);
      if (_debug) {
	cout << "LBTest::recv(): " << id << endl;
      }
      //update current time for comparison
      struct sysinfo si;
      sysinfo(&si);
      cur_time = si.uptime;
      timeval recv_time;
      gettimeofday(&recv_time,NULL);
      map<int,PktData>::iterator r_iter = _results.find(id);
      if (r_iter != _results.end()) {
	//calculate time in milliseconds
	int secs = 0;
	int msecs = recv_time.tv_usec - send_time.tv_usec;
	if (msecs < 0) {
	  secs = recv_time.tv_sec - send_time.tv_sec - 1;
	}
	else {
	  secs = recv_time.tv_sec - send_time.tv_sec;
	}

	if (_debug) {
	  printf("LBTest::recv(): usecs: %d, secs: %d\n",msecs,secs);
	}

	//time in milliseconds below
	r_iter->second._rtt = (abs(msecs) / 1000) + 1000 * secs;
	--pending_result_ct;
      }
    }
    if (_debug) {
      cout << "LBTest::recv(): finished heath test" << endl;
    }
    _received = true;
  }
  //now let's just look the packet up since we are through with the receive option
  map<int,PktData>::iterator r_iter = _results.begin();
  _state = LBTest::K_FAILURE;
  while (r_iter != _results.end()) {
    if (r_iter->second._iface == health._interface) {

      if (r_iter->second._rtt >= 0 && r_iter->second._rtt < _resp_time) {
	_state = LBTest::K_SUCCESS;
	if (_debug) {
	  cout << "LBTest::recv(): success for " << r_iter->second._iface << " : " << r_iter->second._rtt << endl;
	}
	int rtt = r_iter->second._rtt;
	_results.erase(r_iter);
	return rtt;
      }
      else {
	if (_debug) {
	  cout << "LBTest::recv(): failure for " << r_iter->second._iface << " : " << r_iter->second._rtt << endl;
	}
	_results.erase(r_iter);
	return -1;
      }
    }
    ++r_iter;
  }
  
  if (_debug) {
    cout << "LBTest::recv(): failure for " << health._interface << " : unable to find interface" << endl;
  }
  return -1;
}




/**
 *
 *
 **/
int
LBTest::receive(int recv_sock)
{
  timeval wait_time;
  int icmp_pktsize = 40;
  char resp_buf[icmp_pktsize];
  icmphdr *icmp_hdr;
  fd_set readfs;

  FD_ZERO(&readfs);
  FD_SET(recv_sock, &readfs);

  wait_time.tv_usec = 0;
  wait_time.tv_sec = 3; //3 second timeout

  if (_debug) {
    cout << "LBTest::receive(): start" << endl;
  }

  //NEW-OLD STUFF HERE
   
  if (select(recv_sock+1, &readfs, NULL, NULL, &wait_time) != 0) {
    int bytes_recv = ::recv(recv_sock, &resp_buf, 56, 0);
    if (bytes_recv != -1) {
      if (_debug) {
	cout << "LBTest::receive() received: " << bytes_recv << endl;
      }
      icmp_hdr = (struct icmphdr *)(resp_buf + sizeof(iphdr));
      if (icmp_hdr->type == ICMP_TIME_EXCEEDED) {
	//process packet data
	char* data;
	int id = 0; 
	data = (char*)(&resp_buf) + 49;
	memcpy(&id, data, sizeof(unsigned short));
	if (_debug) {
	  cout << "LBTest::receive(): " << id << endl;
	}
	return id;
      }
      else if (icmp_hdr->type == ICMP_ECHOREPLY) {
	char* data;
	int id = 0; 
	data = (char*)(&resp_buf) + 36;
	memcpy(&id, data, sizeof(unsigned short));
	if (_debug) {
	  cout << "LBTest::receive(): " << id << endl;
	}
	return id;
      }
    }
    else {
      cerr << "LBTest::receive(): error from recvfrom" << endl;
    }
  }
  return -1;
}


