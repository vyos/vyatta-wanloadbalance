/*
 * Module: lbpathtest.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
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
#include <errno.h>
#include <string.h>
#include <memory>
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <algorithm>
#include "lbdata.hh"
#include "lbtest_icmp.hh"

ICMPEngine LBTestICMP::_engine;

using namespace std;

/**
 *
 *
 **/
void
ICMPEngine::init() 
{
  if (_initialized == false) {
    _results.erase(_results.begin(),_results.end());
  }
  if (_debug) {
    cout << "ICMPEngine::init(): initializing test system" << endl;
  }
  _initialized = true;
  _received = false;
}

/**
 *
 *
 **/
int
ICMPEngine::process(LBHealth &health,LBTestICMP *data)
{
  //iterate over packets and send
  string target = data->_target;
  if (target.empty()) {
    if (health._nexthop == "dhcp") {
      target = health._dhcp_nexthop;
    }
    else {
      target = health._nexthop;
    }
  }
  
  //don't have target yet...
  if (target.empty()) {
    return -1;
  }
  _packet_id = ++_packet_id % 32767;
  if (_debug) {
    cout << "ICMPEngine::start(): sending ping test for: " << health._interface << " for " << target << " id: " << _packet_id << endl;
  }
  send(data->_send_icmp_sock, health._interface, target, _packet_id);
  _results.insert(pair<int,PktData>(_packet_id,PktData(health._interface,-1)));
}

/**
 *
 *
 **/
int
ICMPEngine::recv(LBHealth &health,LBTestICMP *data) 
{
  _initialized = false;
  if (_received == false) {
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
    //let's pull off all of the results...
    while (cur_time < timeout && pending_result_ct != 0) {
      int id = receive(data->_recv_icmp_sock);
      if (_debug) {
	cout << "ICMPEngine::recv(): " << id << endl;
      }
      
      //update current time for comparison
      struct sysinfo si;
      sysinfo(&si);
      timeval recv_time;
      gettimeofday(&recv_time,NULL);
      cur_time = si.uptime;
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
	//time in milliseconds below
	r_iter->second._rtt = abs(msecs) / 1000 + 1000 * secs;
	--pending_result_ct;
      }
    }
    if (_debug) {
      cout << "ICMPEngine::recv(): finished heath test" << endl;
    }
    _received = true;
  }

  //now let's just look the packet up since we are through with the receive option
  map<int,PktData>::iterator r_iter = _results.begin();
  data->_state = LBTest::K_FAILURE;
  while (r_iter != _results.end()) {

    if (r_iter->second._iface == health._interface) {
      if (r_iter->second._rtt < data->_resp_time) {
	data->_state = LBTest::K_SUCCESS;
	if (_debug) {
	  cout << "ICMPEngine::recv(): success for " << r_iter->second._iface << " : " << r_iter->second._rtt << endl;
	}
	int rtt = r_iter->second._rtt;
	_results.erase(r_iter);
	return rtt;
      }
      else {
	if (_debug) {
	  cout << "ICMPEngine::recv(): failure for " << r_iter->second._iface << " : " << r_iter->second._rtt << endl;
	}
	_results.erase(r_iter);
	return -1;
      }
    }
    ++r_iter;
  }
  
  if (_debug) {
    cout << "ICMPEngine::recv(): failure for " << health._interface << " : unable to find interface" << endl;
  }
  return -1;
}

/**
 *
 *
 **/
void
ICMPEngine::send(int send_sock, const string &iface, const string &target_addr, int packet_id)
{
  int err;
  sockaddr_in taddr;
  timeval send_time;
  icmphdr *icmp_hdr;
  int icmp_pktsize = 40;
  char buffer[icmp_pktsize];

  if (iface.empty() || target_addr.empty()) {
    return;
  }

  //convert target_addr to ip addr
  struct hostent *h = gethostbyname(target_addr.c_str());
  if (h == NULL) {
    if (_debug) {
      cerr << "ICMPEngine::send() Error in resolving hostname" << endl;
    }
    syslog(LOG_ERR, "wan_lb: error in resolving configured hostname: %s", target_addr.c_str());
    return;
  }

  // bind a socket to a device name (might not work on all systems):
  if (setsockopt(send_sock, SOL_SOCKET, SO_BINDTODEVICE, iface.c_str(), iface.size()) != 0) {
    syslog(LOG_ERR, "wan_lb: failure to bind to interface: %s", iface.c_str());
    return; //will allow the test to time out then
  }

  icmp_hdr = (struct icmphdr *)buffer;
  icmp_hdr->type = ICMP_ECHO;
  icmp_hdr->code = 0;
  icmp_hdr->checksum = 0;
  icmp_hdr->un.echo.id = htons(getpid());
  icmp_hdr->un.echo.sequence = 0;
  int length = sizeof(buffer);

  //we'll put in time of packet sent for the heck of it, may
  //want to use this in future tests, feel free to remove.
  gettimeofday(&send_time, (struct timezone*)NULL);
  char* datap = &buffer[8];
  memcpy(datap, (char*)&send_time.tv_sec, sizeof(send_time.tv_sec));
  datap = &buffer[12];
  memcpy(datap, (char*)&send_time.tv_usec, sizeof(send_time.tv_usec));
  datap = &buffer[16];
  memcpy(datap, (char*)&packet_id, sizeof(packet_id)); //packet id
  datap = &buffer[18];
  int val(icmp_pktsize);
  memcpy(datap, (char*)&val, 2); //packet id

  icmp_hdr->un.echo.sequence = 1;
  icmp_hdr->checksum = 0;
  icmp_hdr->checksum = in_checksum((unsigned short *)icmp_hdr,length>>1);

  struct in_addr ia;
  memcpy(&ia, h->h_addr_list[0], sizeof(ia));
  unsigned long addr = ia.s_addr;

  taddr.sin_addr.s_addr = addr;
  taddr.sin_family = AF_INET;
  bzero(&(taddr.sin_zero), 8);

  //need to direct this packet out a specific interface!!!!!!!!!!!!!
  err = sendto(send_sock, buffer, icmp_pktsize, 0, (struct sockaddr*)&taddr, sizeof(taddr));
  if (_debug) {
    cout << "ICMPEngine::send(): sendto: " << err << ", packet id: " << packet_id << endl;
  }
  if(err < 0) {
    if (_debug) {
      if (errno == EBADF)
	cout << "EBADF" << endl;
      else if (errno == ENOTSOCK)
	cout << "ENOTSOCK" << endl;
      else if (errno == EFAULT)
	cout << "EFAULT" << endl;
      else if (errno == EMSGSIZE)
	cout << "EMSGSIZE" << endl;
      else if (errno == EWOULDBLOCK)
	cout << "EWOULDBLOCK" << endl;
      else if (errno == EAGAIN)
	cout << "EAGAIN" << endl;
      else if (errno == ENOBUFS)
	cout << "ENOBUFS" << endl;
      else if (errno == EINTR)
	cout << "EINTR" << endl;
      else if (errno == ENOMEM)
	cout << "ENOMEM" << endl;
      else if (errno == EACCES)
	cout << "EACCES" << endl;
      else if (errno == EINVAL)
	cout << "EINVAL" << endl;
      else if (errno == EPIPE)
	cout << "EPIPE" << endl;
      else
	cout << "unknown error: " << errno << endl;
    }
    syslog(LOG_ERR, "wan_lb: error on sending icmp packet: %d", errno);
  }
}

/**
 *
 *
 **/
int
ICMPEngine::receive(int recv_sock)
{
  int icmp_pktsize = 40;
  char resp_buf[icmp_pktsize];
  icmphdr *icmp_hdr;
  timeval wait_time;
  fd_set readfs;
  int ret;

  FD_ZERO(&readfs);
  FD_SET(recv_sock, &readfs);

  wait_time.tv_usec = 0;
  wait_time.tv_sec = 3; //3 second timeout

  if (_debug) {
    cout << "ICMPEngine::receive(): start" << endl;
  }

  while (select(recv_sock+1, &readfs, NULL, NULL, &wait_time) != 0) {
    ret = ::recv(recv_sock, &resp_buf, icmp_pktsize, 0);
    if (ret != -1) {
      if (_debug) {
	cout << "ICMPEngine::receive(): recv: " << ret << endl;
      }
      icmp_hdr = (struct icmphdr *)(resp_buf + sizeof(iphdr));
      if (icmp_hdr->type == ICMP_ECHOREPLY) {
	//process packet data
	char* data;
	int id = 0; 
	data = (char*)(&resp_buf) + 36;
	memcpy(&id, data, sizeof(unsigned short));
	if (_debug) {
	  cout << "ICMPEngine::receive(): " << id << endl;
	}
	return id;
      }
    }
  }
  return -1;
}

/**
 *
 *
 **/
unsigned short
ICMPEngine::in_checksum(const unsigned short *buffer, int length) const
{
  unsigned long sum;
  for (sum=0; length>0; length--) {
    sum += *buffer++;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

/**
 *
 *
 **/
string
LBTestICMP::dump()
{
  char buf[20];
  sprintf(buf,"%u",_resp_time);
  return (string("target: ") + _target + ", resp_time: " + buf);
}
