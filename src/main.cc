/*
 * Module: main.cc
 *
 * **** License ****
 * Version: VPL 1.0
 *
 * The contents of this file are subject to the Vyatta Public License
 * Version 1.0 ("License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.vyatta.com/vpl
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * This code was originally developed by Vyatta, Inc.
 * Portions created by Vyatta are Copyright (C) 2007 Vyatta, Inc.
 * All Rights Reserved.
 *
 * Author: Michael Larson
 * Date: 2007
 * Description:
 *
 * **** End License ****
 *
 */
#include <signal.h>
#include <syslog.h>
#include <stdio.h>
#include <iostream>
#include "loadbalance.hh"

LoadBalance *g_lb = NULL;


static void usage()
{
  cout << "lb -fh" << endl;
  cout << "-f [file] configuration file" << endl;
  cout << "-t load configuration file only and exit" << endl;
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

static void sig_user(int signo)
{
  if (g_lb)
    delete g_lb;
  cerr << "User signal: " << signo << endl;
  syslog(LOG_ERR, "wan_lb, user exit signal caught, exiting..");
  exit(0);
}


int main(int argc, char* argv[])
{
  int ch;
  bool config_debug_mode = false;
  string c_file;

  //grab inputs
  while ((ch = getopt(argc, argv, "f:ht")) != -1) {
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
 
  g_lb = new LoadBalance();
  
  bool success = g_lb->set_conf(c_file);
  if (success == false) {
    syslog(LOG_ERR, "wan_lb: error loading configuration file: %s", c_file.c_str());
    exit(0);
  }

  if (config_debug_mode) {
    exit(0);
  }

#ifdef DEBUG
  cout << "STARTING CYCLE" << endl;
#endif

  g_lb->init();

  
  //signal handler here
  //  sighup...
  signal(SIGINT, sig_end);
  signal(SIGTERM, sig_end);
  signal(SIGUSR1, sig_user);

    //drop into event loop
  do {
#ifdef DEBUG
    cout << "main.cc: starting new cycle" << endl;
#endif

    //health test
    g_lb->health_test();

    //apply rules
    g_lb->apply_rules();

    //update show output
    g_lb->output();

    g_lb->sleep();
  } while (g_lb->start_cycle());
  exit(0);
}
