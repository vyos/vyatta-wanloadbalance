/**

 * Module: Hosts

 * Description: Host IP and last response time for all responding hosts

 * on this LAN segment. This tool uses a broadcast icmp test packet.

 *

 * Note that hosts can also be identified via ARP progressing through

 * all possible addresses on lan segment.

 *

 * Author: Michael Larson

 * email: mike(at)lrlart.com

 * Date: August 2004

 **/



#include <ostream.h>

#include <sys/time.h>

#include <sys/types.h>

#include <unistd.h>

#include <sys/socket.h>

#include <netinet/udp.h>

#include <netinet/in.h>

#include <netinet/ip.h>

#include <netinet/ip_icmp.h>

#include <errno.h>

#include <memory>

#include <time.h>

#include <sys/timeb.h>

#include <pthread.h>

#include <stdio.h>

#include <stdlib.h>

#include <iostream.h>

#include <string>

#include <algorithm>



#include "Hosts.hpp"

#include "HostsResult.hpp"

#include "HostsTool.hpp"



//constant initialization

const int HostsToolKonstants::packet_data_len_ = 40;

const int HostsToolKonstants::recv_timeout_ = 5;

const int HostsToolKonstants::ip_offset_ = 12;



/**

 * HostsTool::HostsTool()

 * Constructor. Builds socket for use in tests.

 *

 **/

HostsTool::HostsTool(Task<Test> *complete_task, unsigned long local_ip, unsigned long bc_addr) :

  ToolBase(complete_task),

  local_ip_(local_ip),

  send_sock_(0),

  recv_sock_(0),

  bc_addr_(bc_addr),

  packet_id_(0),

  test_in_progress_(false)

{

  sockaddr_in addr;



  struct protoent *ppe = getprotobyname("icmp");

  send_sock_ = socket(PF_INET, SOCK_RAW, ppe->p_proto);

  if (send_sock_ < 0)

    {

      cerr << "HostsTool::HostsTool(): no send sock: " << send_sock_ << endl;

      send_sock_ = 0;

      return;

    }



  //set options for broadcasting.

  int val = 1;

  setsockopt(send_sock_, SOL_SOCKET, SO_BROADCAST, &val, 4);

  setsockopt(send_sock_, SOL_SOCKET, SO_REUSEADDR, &val, 4);

   

  memset( &addr, 0, sizeof( struct sockaddr_in ));

  addr.sin_family = AF_INET;

  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  addr.sin_port = 0;

   

  recv_sock_ = socket(PF_INET, SOCK_RAW, ppe->p_proto);

  if (recv_sock_ < 0)

    {

      cerr << "HostsTool::HostsTool(): no recv sock: " << recv_sock_ << endl;

      recv_sock_ = 0;

      return;

    }

}



/**

 * HostsTool::~HostsTools()

 * Destructor, cleans up sockets

 *

 **/

HostsTool::~HostsTool()

{

  if (send_sock_ != 0)

    close(send_sock_);



  if (recv_sock_ != 0)

    close(recv_sock_);

}





/**

 * HostsTool::compute()

 * initiates tests received in its message queue.

 *

 **/

void

HostsTool::compute()

{

  while (true)

    {

      auto_ptr<Test> test(get()); //don't bother keeping this Test object..

      cout << "HostsTool::compute(): received test event..." << endl;

      if (test.get() != NULL)

	{

	  GUARD(&mutex_); //protect against concurrent access to test_in_progress_ flag

	  /*

           Note that on heavily cycled tests some tests may be dropped. This can be fixed

           by monitoring input queue on completion of sending and processing all incoming

           messages into a set. This isn't expected to be a problem with the current impl.

	  */

	  if (test_in_progress_ == true)

            continue;

	  if (test->get_target() == 0)

	    {

	      send(bc_addr_, packet_id_++);

	    }

	  else

	    {

	      send(test->get_target(), packet_id_++);

	    }

	}

    }

}



/**

 * HostsTool::finish()

 * processes completed tests and pushes results to manager

 *

 **/

void

HostsTool::finish()

{

  while (true)

    {

      HostsResult *host_result = receive();

      //      cout << "HostsTool::finish(): received result: " << host_result << endl;

      if (host_result != NULL)

	{

	  GUARD(&mutex_);

	  test_in_progress_ = false;

	  if (host_result->empty() == false)

	    {

	      Test *test = new Test(kHosts);

	      test->set_result(host_result);

	      //            cout << "HostsTool::finish(), dispatching result" << endl;

	      results(test);

	    }

	}

    }

}



/**

 * HostsTool::send()

 * pushes the icmp packet out on the wire.

 *

 **/

void

HostsTool::send(unsigned long target_addr, unsigned short packet_id)

{

  int err;

  ->type) << endl;

}

}

      else

	{

	  cerr << "HostsTool::receive(): error from recvfrom" << endl;

	}

}

//then push result upwards

return host_result;

}



/**

 * HostsTool::in_checksum()

 * computes checksum that accompanies sending icmp packet

 *

 **/

unsigned short

HostsTool::in_checksum(const unsigned short *buffer, int length) const

{

  unsigned long sum;

  for (sum=0; length>0; length--)

    sum += *buffer++;

  sum = (sum >> 16) + (sum & 0xffff);

  sum += (sum >> 16);

  return ~sum;

}
