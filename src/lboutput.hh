/*
 * Module: lboutput.hh
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */
#ifndef __LBOUTPUT_HH__
#define __LBOUTPUT_HH__

#include "lbdata.hh"

using namespace std;

class LBOutput
{
public:
  LBOutput(bool debug, string &output_path) : _debug(debug), _output_path(output_path) {}
  ~LBOutput() {}

  void
  shutdown();

  void
  write(const LBData &lbdata);

private:
  bool _debug;
  string _output_path;
};
#endif //__LBOUTPUT_HH__
