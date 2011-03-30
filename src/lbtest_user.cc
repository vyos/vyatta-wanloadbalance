/*
 * Module: lbtest_user.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "lbdata.hh"
#include "lbtest_user.hh"

using namespace std;

/**
 *
 *
 **/
void
LBTestUser::send(LBHealth &health)
{
  if (_debug) {
    cout << "LBTestUser::process()" << endl;
  }

  _status_line = name() + "\t" + string("Script: ") + _script;
  

  //need to set environment variable for interface

  //execute script
  string environment = "export WLB_SCRIPT_IFACE=" + health._interface;
  string cmd = "/bin/bash -p -c '" + environment + ";umask 000; " + _script + ";'";
  string out;
  if (system_out(cmd,out) == 0) {
    _state = LBTest::K_SUCCESS;
  }
  else {
    _state = LBTest::K_FAILURE;
  }
  return;
}

int
LBTestUser::system_out(const string &cmd, string &out) 
{
  //  fprintf(out_stream,"system out\n");
  if (cmd.empty()) {
    return -1;               
  }

  int cp[2]; // Child to parent pipe
  if( pipe(cp) < 0) {                                                                                                       
    return -1;
  }

  pid_t pid = fork();                                                                                 
  if (pid == 0) {                                                                                          
    //child
    close(cp[0]);
    close(0); // Close current stdin.
    dup2(cp[1],STDOUT_FILENO); // Make stdout go to write end of pipe.
    dup2(cp[1],STDERR_FILENO); // Make stderr go to write end of pipe.
    //    fcntl(cp[1],F_SETFD,fcntl(cp[1],F_GETFD) & (~FD_CLOEXEC));
    close(cp[1]);
    int ret = 0;  


    //set user and group here
    setregid(getegid(),getegid());
    setreuid(geteuid(),geteuid());

    //    fprintf(out_stream,"executing: %s\n",cmd);
    if (execl("/bin/sh","sh","-c",cmd.c_str(),NULL) == -1) {                                   
      ret = errno;
    }
    close(cp[1]);
    exit(ret);
  }
  else {
    //parent         
    char buf[8192];
    memset(buf,'\0',8192);
    close(cp[1]);
    fd_set rfds;
    struct timeval tv;

    int flags = fcntl(cp[0], F_GETFL, 0);
    fcntl(cp[0], F_SETFL, flags | O_NONBLOCK);

    FD_ZERO(&rfds);
    FD_SET(cp[0], &rfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    while (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) != -1) {
      int err = 0;
      if ((err = read(cp[0], &buf, 8191)) > 0) {
	out += string(buf);
      }
      else if (err == -1 && errno == EAGAIN) {
	//try again
      }
      else {
	break;
      }
      FD_ZERO(&rfds);
      FD_SET(cp[0], &rfds);
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      fflush(NULL);
    }

    //now wait on child to kick the bucket                  
    int status;
    wait(&status);
    close(cp[0]);
    return WEXITSTATUS(status);
  }
}


/**
 *
 *
 **/
string
LBTestUser::dump()
{
  char buf[20];
  sprintf(buf,"%u",_resp_time);
  string foo = string("script: ") + _script + ", resp_time: " + buf;
  return foo;
}
