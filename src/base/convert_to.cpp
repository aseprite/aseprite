// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/convert_to.h"
#include "base/version.h"
#include <cstdio>
#include <cstdlib>

namespace base {

template<> int convert_to(const string& from)
{
  return std::strtol(from.c_str(), NULL, 10);
}

template<> string convert_to(const int& from)
{
  char buf[32];
  std::sprintf(buf, "%d", from);
  return buf;
}

template<> Version convert_to(const string& from)
{
  Version result;
  int i = 0;
  int j = 0;
  while (j != string::npos) {
    j = from.find('.', i);
    string digitString = from.substr(i, j - i);
    int digit = convert_to<int>(digitString);
    result.addDigit(digit);
    i = j+1;
  }
  return result;
}

template<> string convert_to(const Version& from)
{
  string result;
  result.reserve(3*from.size());

  for (size_t i=0; i<from.size(); ++i) {
    result += convert_to<string>(from[i]);
    if (i < from.size()-1)
      result += ".";
  }

  return result;
}

} // namespace base
