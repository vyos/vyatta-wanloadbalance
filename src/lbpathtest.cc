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
#include "lbpathtest.hh"

using namespace std;

LBPathTest::LBPathTest(bool debug) : 
  _debug(debug),
  _send_sock(0),
  _recv_sock(0),
  _packet_id(0)
{
  struct protoent *ppe = getprotobyname("icmp");
  _send_sock = socket(PF_INET, SOCK_RAW, ppe->p_proto);
  if (_send_sock < 0){
    if (_debug) {
      cerr << "LBPathTest::LBPathTest(): no send sock: " << _send_sock << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to acquired socket");
    _send_sock = 0;
    return;
  }

  //set options for broadcasting.
  int val = 1;
  setsockopt(_send_sock, SOL_SOCKET, SO_BROADCAST, &val, 4);
  setsockopt(_send_sock, SOL_SOCKET, SO_REUSEADDR, &val, 4);

  struct sockaddr_in addr;
  memset( &addr, 0, sizeof( struct sockaddr_in ));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = 0;

  _recv_sock = socket(PF_INET, SOCK_RAW, ppe->p_proto);
  if (_recv_sock < 0) {
    if (_debug) {
      cerr << "LBPathTest::LBPathTest(): no recv sock: " << _recv_sock << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to acquired socket");
    _recv_sock = 0;
    return;
  }
  if (bind(_recv_sock, (struct sockaddr*)&addr, sizeof(addr))==-1) {
    if (_debug) {
      cerr << "failed on bind" << endl;
    }
    syslog(LOG_ERR, "wan_lb: failed to bind recv sock");
  }
}

LBPathTest::~LBPathTest()
{
  if (_recv_sock)
    close(_recv_sock);

  if (_send_sock)
    close(_send_sock);
}

void
LBPathTest::start(LBData &lb_data)
{
  if (_debug) {
    cout << "LBPathTest::start(): starting health test. client ct: " << lb_data._iface_health_coll.size() << endl;
  }

  map<int,PktData> results;

  struct timeval send_time;
  gettimeofday(&send_time,NULL);

  int ct = 0;
  //iterate over packets and send
  LBData::InterfaceHealthIter iter = lb_data._iface_health_coll.begin();
  while (iter != lb_data._iface_health_coll.end()) {
    if (_debug) {
      cout << "LBPathTest::start(): sending ping test for: " << iter->first << " for " << iter->second._ping_target << endl;
    }
    _packet_id = ++_packet_id % 32767;
    send(iter->first, iter->second._ping_target, _packet_id);
    results.insert(pair<int,PktData>(_packet_id,PktData(iter->first,-1)));

    ++ct;
    ++iter;
  }

  //use gettimeofday to calculate time to millisecond

  //use sysinfo to make sure we don't get stuck in a loop with timechange
  struct sysinfo si;
  sysinfo(&si);
  //for now hardcode to 5 second overall timeout
  int timeout = si.uptime + 5; //seconds
  int cur_time = si.uptime;

  //then iterate over recv socket and receive and record
  while (ct > 0 && cur_time < timeout) {
    int id = receive();
    if (_debug) {
      cout << "LBPathTest::start(): " << id << endl;
    }
    //update current time for comparison
    sysinfo(&si);
    timeval recv_time;
    gettimeofday(&recv_time,NULL);
    cur_time = si.uptime;
    map<int,PktData>::iterator r_iter = results.find(id);
    if (r_iter != results.end()) {

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
      int rtt = abs(msecs) / 1000 + 1000 * secs;

      LBData::InterfaceHealthIter iter = lb_data._iface_health_coll.find(r_iter->second._iface);
      if (iter != lb_data._iface_health_coll.end()) {
	//check to see if this returned in the configured time, otherwise apply timeout
	if (_debug) {
	  cout << "LBPathTest::start(): received pkt: " << iter->first << ", rtt: " << rtt << endl;
	}
	if (rtt < iter->second._ping_resp_time) {
	  iter->second.put(rtt);
	}
	else {
	  iter->second.put(-1);
	}
      }
      results.erase(r_iter);
      --ct;
    }
  }
  
  //we're done waiting, mark the rest as non-responsive
  map<int,PktData>::iterator r_iter = results.begin();
  while (r_iter != results.end()) {
    LBData::InterfaceHealthIter iter = lb_data._iface_health_coll.find(r_iter->second._iface);
    if (iter != lb_data._iface_health_coll.end()) {
      iter->second.put(-1);
    }
    ++r_iter;
  }

  if (_debug) {
    cout << "LBPathTest::start(): finished heath test" << endl;
  }
}

void
LBPathTest::send(const string &iface, const string &target_addr, int packet_id)
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

  // bind a socket to a device name (might not work on all systems):
  setsockopt(_send_sock, SOL_SOCKET, SO_BINDTODEVICE, iface.c_str(), iface.size());

  //convert target_addr to ip addr
  struct hostent *h = gethostbyname(target_addr.c_str());
  if (h == NULL) {
    if (_debug) {
      cerr << "LBPathTest::send() Error in resolving hostname" << endl;
    }
    syslog(LOG_ERR, "wan_lb: error in resolving configured hostname: %s", target_addr.c_str());
    return;
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
  err = sendto(_send_sock, buffer, icmp_pktsize, 0, (struct sockaddr*)&taddr, sizeof(taddr));
  if (_debug) {
    cout << "lbpathtest: sendto: " << err << ", packet id: " << packet_id << endl;
  }
  if(err < 0)
  {
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

int
LBPathTest::receive()
{
  int icmp_pktsize = 40;
  char resp_buf[icmp_pktsize];
  icmphdr *icmp_hdr;
  timeval wait_time;
  fd_set readfs;
  int ret;

  FD_ZERO(&readfs);
  FD_SET(_recv_sock, &readfs);

  wait_time.tv_usec = 0;
  wait_time.tv_sec = 3; //3 second timeout

  while (select(_recv_sock+1, &readfs, NULL, NULL, &wait_time) != 0)
  {
    ret = recv(_recv_sock, &resp_buf, icmp_pktsize, 0);
    if (ret != -1)
    {
      icmp_hdr = (struct icmphdr *)(resp_buf + sizeof(iphdr));
      if (icmp_hdr->type == ICMP_ECHOREPLY)
      {
	if (_debug) {
	  cout << "LBPathTest::receive(): " << endl;
	}
	//process packet data
	  char* data;
	  int id = 0; 
	  data = (char*)(&resp_buf) + 36;
	  memcpy(&id, data, sizeof(unsigned short));
	  return id;
      }
    }
  }
  return -1;
}

unsigned short
LBPathTest::in_checksum(const unsigned short *buffer, int length) const
{
  unsigned long sum;
  for (sum=0; length>0; length--)
    sum += *buffer++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

