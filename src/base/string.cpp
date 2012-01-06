// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/string.h"
#include <cctype>

using namespace base;

string base::string_to_lower(const string& original)
{
  string result(original);

  for (string::iterator it=result.begin(); it!=result.end(); ++it)
    *it = std::tolower(*it);

  return result;
}

string base::string_to_upper(const string& original)
{
  string result(original);

  for (string::iterator it=result.begin(); it!=result.end(); ++it)
    *it = std::toupper(*it);

  return result;
}
