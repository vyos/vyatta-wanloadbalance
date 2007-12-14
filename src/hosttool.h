/**
 * Module: HostsTool
 * Description: Collects lists of hosts on the network responding to broadcast icmp.
 * This list of hosts are then dispatched for processing of results
 *
 * Author: Michael Larson
 * email: mike(at)lrlart.com
 * Date: August 2004
 **/
#ifndef HOSTSTOOL_HPP_
#define HOSTSTOOL_HPP_

//forward decls
class HostsResult;

//header includes
#include <vector>
#include "Task.hpp"
#include "Test.hpp"
#include "ToolBase.hpp"

/*********************************************
 **           HostsToolKonstants            **
 *********************************************/
class HostsToolKonstants
{
 public:
  static const int packet_data_len_;
  static const int recv_timeout_;
  static const int ip_offset_;
};

/*********************************************
 **                 HostsTool               **
 *********************************************/
class HostsTool : public ToolBase
{
 public:
  /*
   * Constructor and Destructor
   */
  HostsTool(Task<Test> *pCompleteTask, unsigned long local_ip, unsigned long bc_addr_);
  ~HostsTool();

  /*
   * compute test result
   */
   void
     compute();

   /*
    * finished tests
    */
   void
     finish();

   /*
    * send a new test
    */
   void
     send(unsigned long target_addr, unsigned short packet_id);

   /*
    * receive results
    */
   HostsResult*
     receive();
   
 private:
   /*
    * checksum for icmp packets
    */
   unsigned short
     in_checksum(const unsigned short *addr, int len) const;

 private:
   unsigned long local_ip_;
   int send_sock_;
   int recv_sock_;
   unsigned long bc_addr_;
   int packet_id_;
   bool test_in_progress_;
   PERF_MUTEX mutex_;
};

#endif //HOSTSTOOL_HPP_
