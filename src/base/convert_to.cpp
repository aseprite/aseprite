// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <cstring>

#include "base/convert_to.h"

template<> int base::convert_to(const base::string& from)
{
  return std::strtol(from.c_str(), NULL, 10);
}

template<> base::string base::convert_to(const int& from)
{
  char buf[32];
  std::sprintf(buf, "%d", from);
  return buf;
}
