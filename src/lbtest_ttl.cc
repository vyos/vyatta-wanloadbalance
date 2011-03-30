/*
 * Module: lbtest_ttl.cc
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

using namespace std;

/**
 *
 *
 **/
void
LBTestTTL::send(LBHealth &health)
{
  if (_debug) {
    cout << "LBTestTTL::process()" << endl;
  }
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
  if (_debug) {
    cout << "LBTestTTL::process(): sending ttl test for: " << health._interface << " for " << target << endl;
  }
  _packet_id = get_new_packet_id();
    
  send(_send_raw_sock, health._interface,target,_packet_id,health._address,get_ttl(),get_port());
  _results.insert(pair<int,PktData>(_packet_id,PktData(health._interface,-1)));
  return;
}





/**
 *
 *
 **/
void
LBTestTTL::send(int send_sock, const string &iface, const string &target_addr, unsigned short packet_id, string saddress, int ttl, unsigned short port)
{
  if (_debug) {
    cout << "LBTestTTL::send(): packet_id: " << packet_id << ", ttl: " <<  ttl << ", port: " << port << endl;
  }

  if (saddress.empty()) {
    if (_debug) {
      cout << "LBTestTTL::send() source address is empty" << endl;
    }
    return;
  }
  
  if (_debug) {
    cout << "LBTestTTL::send() source address is: " << saddress << endl;
  }

  int err;
  sockaddr_in taddr;
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
      cerr << "LBTestTTL::send() Error in resolving hostname" << endl;
    }
    syslog(LOG_ERR, "wan_lb: error in resolving configured hostname: %s", target_addr.c_str());
    return;
  }

  // bind a socket to a device name (might not work on all systems):
  if (setsockopt(send_sock, SOL_SOCKET, SO_BINDTODEVICE, iface.c_str(), iface.size()) != 0) {
    cout << "send sock: " << send_sock << endl;
    if (_debug) {
      cerr << "LBTestTTL::send() failure to bind to interface: " << iface << endl;
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
    cout << "LBTestTTL::send(): send " << err << " bytes" << endl;
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
 * LatencyTool::inChecksum()
 * Performs ip checksum computation
 *
 **/
unsigned short
LBTestTTL::in_checksum(unsigned short *addr, int len)
{
  int sum = 0;
  unsigned short answer = 0;
  unsigned short *w = addr;
  int remain = len;

  while(remain > 1)
    {
      sum += *w++;
      remain -= sizeof(unsigned short);
    }
  if(remain==1)
    {
      *(u_char *)(&answer)=*(u_char*)w;
      sum += answer;
    }
  sum = (sum>>16) + (sum&0xffff);
  sum += (sum>>16);
  answer = ~sum;
  return(answer);
}


/**
 * LatencyTool::UDPChecksum()
 * UDP check computation
 *
 **/
unsigned short
LBTestTTL::udp_checksum(unsigned char proto, char *packet, int length, unsigned long source_address, unsigned long dest_address)
{
   struct PseudoHdr
   {
     struct in_addr source_addr;
     struct in_addr dest_addr;
     unsigned char place_holder;
     unsigned char protocol;
     unsigned short length;
   } PseudoHdr;
   
   struct PseudoHdr pseudoHdr;
   char *temp_packet;
   unsigned short answer;
   pseudoHdr.protocol = proto;
   pseudoHdr.length = htons(length);
   pseudoHdr.place_holder = 0;
   pseudoHdr.source_addr.s_addr = source_address;
   pseudoHdr.dest_addr.s_addr = dest_address;
   
   if((temp_packet = (char*)malloc(sizeof(PseudoHdr) + length)) == NULL)
     {
       cerr << "ActionDropConn::UDPChecksum(), error in malloc" << endl;
       //throw an exception
       return 0;
     }
   
   memcpy(temp_packet, &pseudoHdr, sizeof(PseudoHdr));
   memcpy((temp_packet + sizeof(PseudoHdr)), packet, length);
   
   answer = (unsigned short)in_checksum((unsigned short*)temp_packet,
					 (length + sizeof(PseudoHdr)));
   free(temp_packet);
   return answer;
}

/**
 *
 *
 **/
unsigned short
LBTestTTL::get_new_packet_id()
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
