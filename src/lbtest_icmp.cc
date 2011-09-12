/*
 * Module: lbtest_icmp.cc
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

using namespace std;

/**
 *
 *
 **/
void
LBTestICMP::send(LBHealth &health)
{
  //iterate over packets and send
  string target = _target;
  if (target.empty()) {
    if (health._nexthop == "dhcp") {
      target = health._dhcp_nexthop;
    }
    else {
      target = health._nexthop;
    }
  }
  
  //set the status line to be used when the show command is invoked
  _status_line = name() + "\t" + string("Target: ") + target;

  //don't have target yet...
  if (target.empty()) {
    return;
  }

  //instead of prefix to avoid this compilier warning:
  //warning: operation on ‘LBTest::_packet_id’ may be undefined
  _packet_id = int((_packet_id + 1) % int(32767));
  if (_debug) {
    cout << "ICMPEngine::start(): sending ping test for: " << health._interface << " for " << target << " id: " << _packet_id << endl;
  }
  send(_send_icmp_sock, health._interface, target, _packet_id);
  _results.insert(pair<int,PktData>(_packet_id,PktData(health._interface,-1)));
  return;
}

/**
 *
 *
 **/
void
LBTestICMP::send(int send_sock, const string &iface, const string &target_addr, int packet_id)
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
string
LBTestICMP::status()
{
  //set the status line to be used when the show command is invoked
  return name() + "\t" + string("Target: ") + _target;
}



/**
 *
 *
 **/
unsigned short
LBTestICMP::in_checksum(const unsigned short *buffer, int length) const
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


