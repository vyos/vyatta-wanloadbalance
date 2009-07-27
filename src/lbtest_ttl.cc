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
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <algorithm>

#include "lbdata.hh"
#include "lbtest_ttl.hh"

TTLEngine LBTestTTL::_engine;

using namespace std;

/**
 *
 *
 **/
void
TTLEngine::init() 
{
  if (_initialized == false) {
    _results.erase(_results.begin(),_results.end());
  }
  if (_debug) {
    cout << "TTLEngine::init(): initializing test system" << endl;
  }
  _initialized = true;
  _received = false;
}

/**
 *
 *
 **/
int
TTLEngine::process(LBHealth &health,LBTestTTL *data)
{
  if (_debug) {
    cout << "TTLEngine::process()" << endl;
  }
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
  if (_debug) {
    cout << "TTLEngine::process(): sending ttl test for: " << health._interface << " for " << target << endl;
  }
  _packet_id = get_new_packet_id();
    
  send(data->_send_raw_sock, health._interface,target,_packet_id,health._address,data->get_ttl(),data->get_port());
  _results.insert(pair<int,PktData>(_packet_id,PktData(health._interface,-1)));
}

/**
 *
 *
 **/
int
TTLEngine::recv(LBHealth &health,LBTestTTL *data) 
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
    while (cur_time < timeout && pending_result_ct != 0) {
      int id = receive(data->_recv_icmp_sock);
      if (_debug) {
	cout << "TTLEngine::recv(): " << id << endl;
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
      cout << "TTLEngine::recv(): finished heath test" << endl;
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
	  cout << "TTLEngine::recv(): success for " << r_iter->second._iface << " : " << r_iter->second._rtt << endl;
	}
	int rtt = r_iter->second._rtt;
	_results.erase(r_iter);
	return rtt;
      }
      else {
	if (_debug) {
	  cout << "TTLEngine::recv(): failure for " << r_iter->second._iface << " : " << r_iter->second._rtt << endl;
	}
	_results.erase(r_iter);
	return -1;
      }
    }
    ++r_iter;
  }
  
  if (_debug) {
    cout << "EngineTTL::recv(): failure for " << health._interface << " : unable to find interface" << endl;
  }
  return -1;
}






/**
 *
 *
 **/
void
TTLEngine::send(int send_sock, const string &iface, const string &target_addr, unsigned short packet_id, string saddress, int ttl, unsigned short port)
{
  if (_debug) {
    cout << "TTLEngine::send(): packet_id: " << packet_id << ", ttl: " <<  ttl << ", port: " << port << endl;
  }

  if (saddress.empty()) {
    if (_debug) {
      cout << "TTLEngine::send() source address is empty" << endl;
    }
    return;
  }
  
  if (_debug) {
    cout << "TTLEngine::send() source address is: " << saddress << endl;
  }

  int err;
  sockaddr_in taddr;
  timeval send_time;
  char buffer[42];
  struct iphdr *ip;
  struct udphdr *udp;

  if (iface.empty() || target_addr.empty()) {
    return;
  }

  //convert target_addr to ip addr
  struct hostent *h = gethostbyname(target_addr.c_str());
  if (h == NULL) {
    if (_debug) {
      cerr << "TTLEngine::send() Error in resolving hostname" << endl;
    }
    syslog(LOG_ERR, "wan_lb: error in resolving configured hostname: %s", target_addr.c_str());
    return;
  }

  // bind a socket to a device name (might not work on all systems):
  if (setsockopt(send_sock, SOL_SOCKET, SO_BINDTODEVICE, iface.c_str(), iface.size()) != 0) {
    if (_debug) {
      cerr << "TTLEngine::send() failure to bind to interface: " << iface << endl;
    }
    syslog(LOG_ERR, "wan_lb: failure to bind to interface: %s", iface.c_str());
    return; //will allow the test to time out then
  }

  ip = (iphdr*)buffer;
   
  ip->ihl = 5;
  ip->version = 4;
  ip->tos = 0;
  ip->tot_len = sizeof(iphdr) + sizeof(udphdr);// + 8;
  ip->id = htons(getpid());
  ip->frag_off = 0;
  ip->ttl = ttl;
  ip->protocol = IPPROTO_UDP;
  ip->check = 0;
  in_addr_t addr = inet_addr(saddress.c_str()); //m_ulSourceAddr;
  ip->saddr =  addr;

  struct in_addr ia;
  memcpy(&ia, h->h_addr_list[0], sizeof(ia));
  ip->daddr = ia.s_addr;
  ip->check = in_checksum((unsigned short*)ip, sizeof(iphdr));

  udp = (udphdr*)(buffer + sizeof(iphdr));
  //now udp stuff
  udp->source = htons(packet_id);//was usid--not sure about this...
  udp->dest = htons(packet_id);
  udp->len = sizeof(udphdr);
  udp->check = 0;

  unsigned long data_length = 8; //bytes
   
  //copy in headers
  udp->check = udp_checksum(IPPROTO_UDP,(char*)udp, sizeof(struct udphdr)+data_length, ip->saddr, ip->daddr);

  //set destination port and address
  bzero(&(taddr), sizeof(taddr));
  taddr.sin_addr.s_addr = ia.s_addr;
  taddr.sin_family = AF_INET;
  taddr.sin_port = packet_id;
  err = sendto(send_sock, buffer, sizeof(iphdr)+sizeof(udphdr)+data_length, 0, (struct sockaddr*)&taddr, sizeof(taddr));
  if (_debug) {
    cout << "TTLEngine::send(): send " << err << " bytes" << endl;
  }
  if (_debug) {
    if(err < 0) {
	if (errno == EBADF)
	  cout << "EBADF" << endl;
	else if (errno == EBADF)
	  cout << "EBADF" << endl;
	else if (errno == ENOTSOCK)
	  cout << "ENOTSOCK" << endl;
	else if (errno == EFAULT)
	  cout << "EFAULT" << endl;
	else if (errno == EMSGSIZE)
	  cout << "EMSGSIZE" << endl;
	else if (errno == EWOULDBLOCK)
	  cout << "EWOULDBLOCK" << endl;
	else if (errno == ENOBUFS)
	  cout << "ENOBUFS" << endl;
	else if (errno == EINTR)
	  cout << "EINTR" << endl;
	else if (errno == ENOMEM)
	  cout << "ENOMEM" << endl;
	else if (errno == EINVAL)
	  cout << "EINVAL" << endl;
	else if (errno == EPIPE)
	  cout << "EPIPE" << endl;
	else
	  cout << "unknown error: " << errno << endl;
      }
  }
}

/**
 *
 *
 **/
int
TTLEngine::receive(int recv_sock)
{
  timeval wait_time;
  int icmp_pktsize = 40;
  char resp_buf[icmp_pktsize];
  icmphdr *icmp_hdr;
  struct sockaddr_in dest_addr;
  unsigned int addr_len;
  fd_set readfs;
  unsigned short packet_id = 0;

  FD_ZERO(&readfs);
  FD_SET(recv_sock, &readfs);

  wait_time.tv_usec = 0;
  wait_time.tv_sec = 3; //3 second timeout

  if (_debug) {
    cout << "TTLEngine::receive(): start" << endl;
  }

  //NEW-OLD STUFF HERE
   
  while (select(recv_sock+1, &readfs, NULL, NULL, &wait_time) != 0) {
    int bytes_recv = ::recv(recv_sock, &resp_buf, 56, 0);
    if (bytes_recv != -1) {
      if (_debug) {
	cout << "TTLEngine::receive() received: " << bytes_recv << endl;
      }
      icmp_hdr = (struct icmphdr *)(resp_buf + sizeof(iphdr));

      //      if (icmp_hdr->type == ICMP_ECHOREPLY) {
	//process packet data
	char* data;
	int id = 0; 
	data = (char*)(&resp_buf) + 49;
	memcpy(&id, data, sizeof(unsigned short));
	if (_debug) {
	  cout << "TTLEngine::receive(): " << id << endl;
	}
	return id;
	//      }
    }
    else {
      cerr << "TTLEngine::receive(): error from recvfrom" << endl;
    }
  }
  return -1;
}




/**
 * LatencyTool::inChecksum()
 * Performs ip checksum computation
 *
 **/
unsigned short
TTLEngine::in_checksum(unsigned short *pAddr, int iLen)
{
  int iSum = 0;
  unsigned short usAnswer = 0;
  unsigned short *pW = pAddr;
  int iRemain = iLen;

  while(iRemain > 1)
    {
      iSum += *pW++;
      iRemain -= sizeof(unsigned short);
    }
  if(iRemain==1)
    {
      *(u_char *)(&usAnswer)=*(u_char*)pW;
      iSum += usAnswer;
    }
  iSum = (iSum>>16) + (iSum&0xffff);
  iSum += (iSum>>16);
  usAnswer = ~iSum;
  return(usAnswer);
}


/**
 * LatencyTool::UDPChecksum()
 * UDP check computation
 *
 **/
unsigned short
TTLEngine::udp_checksum(unsigned char ucProto, char *pPacket, int iLength, unsigned long ulSourceAddress, unsigned long ulDestAddress)
{
   struct PsuedoHdr
   {
     struct in_addr sourceAddr;
     struct in_addr destAddr;
     unsigned char ucPlaceHolder;
     unsigned char ucProtocol;
     unsigned short usLength;
   } PsuedoHdr;
   
   struct PsuedoHdr psuedoHdr;
   char *pTempPacket;
   unsigned short usAnswer;
   psuedoHdr.ucProtocol = ucProto;
   psuedoHdr.usLength = htons(iLength);
   psuedoHdr.ucPlaceHolder = 0;
   psuedoHdr.sourceAddr.s_addr = ulSourceAddress;
   psuedoHdr.destAddr.s_addr = ulDestAddress;
   
   if((pTempPacket = (char*)malloc(sizeof(PsuedoHdr) + iLength)) == NULL)
     {
       cerr << "ActionDropConn::UDPChecksum(), error in malloc" << endl;
       //throw an exception
       return 0;
     }
   
   memcpy(pTempPacket, &psuedoHdr, sizeof(PsuedoHdr));
   memcpy((pTempPacket + sizeof(PsuedoHdr)), pPacket, iLength);
   
   usAnswer = (unsigned short)in_checksum((unsigned short*)pTempPacket,
					 (iLength + sizeof(PsuedoHdr)));
   
   free(pTempPacket);
   
   return usAnswer;
}

/**
 *
 *
 **/
unsigned short
TTLEngine::get_new_packet_id()
{
  if (_packet_id >= _max_port_id)
    _packet_id = _min_port_id;
  return ++_packet_id;
}

/**
 *
 *
 **/
string
LBTestTTL::dump()
{
  char buf[20];
  sprintf(buf,"%u",_resp_time);
  string foo = string("target: ") + _target + ", resp_time: " + buf;
  sprintf(buf,"%u",_ttl);
  foo += string(", ttl: ") + buf;
  sprintf(buf,"%u",_port);
  foo += string(", port: ") + buf;
  return foo;
}
