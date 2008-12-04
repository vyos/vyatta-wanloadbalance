/*
 * Module: main.cc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <stdio.h>
#include "stdlib.h"
#include <iostream>
#include "loadbalance.hh"

LoadBalance *g_lb = NULL;
pid_t pid_output (const char *path);


static void usage()
{
  cout << "lb -ftidh" << endl;
  cout << "-f [file] configuration file" << endl;
  cout << "-t load configuration file only and exit" << endl;
  cout << "-v print debug output" << endl;
  cout << "-i specify location of pid directory" << endl;
  cout << "-o specify location of output directory" << endl;
  cout << "-d run as daemon" << endl;
  cout << "-h help" << endl;

}

static void sig_end(int signo)
{
  if (g_lb)
    delete g_lb;
  cerr << "End signal: " << signo << endl;
  syslog(LOG_ERR, "wan_lb, exit signal caught, exiting..");
  exit(0);
}

static void sig_user1(int signo)
{
  if (g_lb)
    delete g_lb;
  cerr << "User signal: " << signo << endl;
  syslog(LOG_ERR, "wan_lb, user exit signal caught, exiting..");
  exit(0);
}

static void sig_user2(int signo)
{
  //used to wake up the process
  cerr << "User signal: " << signo << endl;
  syslog(LOG_ERR, "wan_lb, user exit signal caught, exiting..");
}


int main(int argc, char* argv[])
{
  int ch;
  bool debug = false;
  bool config_debug_mode = false, daemon = false;
  string pid_path = "/var/run";
  string output_path = "/var/load-balance";
  string c_file;

  //grab inputs
  while ((ch = getopt(argc, argv, "f:hti:o:dv")) != -1) {
    switch (ch) {
    case 'f':
      c_file = optarg;
      break;
    case 'h':
      usage();
      exit(0);
    case 't':
      config_debug_mode = true;
      break;
    case 'i':
      pid_path = optarg;
      break;
    case 'o':
      output_path = optarg;
      break;
    case 'd':
      daemon = true;
      break;
    case 'v':
      debug = true;
      break;
    default:
      usage();
      exit(0);
    }
  }


  //parse conf file
  if (c_file.empty()) {
    cout << "Configuration file is empty" << endl;
    exit(0);
  }
 
  if (daemon) {
    if (fork() != 0) {
      //      int s;
      //      wait(&s);
      exit(0);
    }
  }

  if (pid_path.empty() == false) {
    pid_output(pid_path.c_str());
  }

  g_lb = new LoadBalance(debug, output_path);
  
  bool success = g_lb->set_conf(c_file);
  if (success == false) {
    syslog(LOG_ERR, "wan_lb: error loading configuration file: %s", c_file.c_str());
    exit(0);
  }

  if (config_debug_mode) {
    exit(0);
  }

  if (debug) {
    cout << "STARTING CYCLE" << endl;
  }

  g_lb->init();

  
  //signal handler here
  //  sighup...
  signal(SIGINT, sig_end);
  signal(SIGTERM, sig_end);
  signal(SIGUSR1, sig_user1);
  signal(SIGUSR2, sig_user2);

    //drop into event loop
  do {
    if (debug) {
      cout << "main.cc: starting new cycle" << endl;
    }

    //health test
    g_lb->health_test();

    //apply rules
    g_lb->apply_rules();

    //update show output
    g_lb->output();

    g_lb->sleep();
  } while (g_lb->start_cycle());

  if (g_lb) {
    delete g_lb;
  }

  exit(0);
}

/**
 *
 *below borrowed from quagga library.
 **/
#define PIDFILE_MASK 0644
pid_t
pid_output (const char *path)
{
  FILE *fp;
  pid_t pid;
  mode_t oldumask;

  pid = getpid();

  oldumask = umask(0777 & ~PIDFILE_MASK);
  fp = fopen (path, "w");
  if (fp != NULL) 
    {
      fprintf (fp, "%d\n", (int) pid);
      fclose (fp);
      umask(oldumask);
      return pid;
    }
  /* XXX Why do we continue instead of exiting?  This seems incompatible
     with the behavior of the fcntl version below. */
  syslog(LOG_ERR,"Can't fopen pid lock file %s, continuing",
            path);
  umask(oldumask);
  return -1;
}
