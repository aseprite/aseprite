// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/string.h"
#include <cctype>

namespace base {

string string_to_lower(const string& original)
{
  string result(original);

  for (string::iterator it=result.begin(); it!=result.end(); ++it)
    *it = std::tolower(*it);

  return result;
}

string string_to_upper(const string& original)
{
  string result(original);

  for (string::iterator it=result.begin(); it!=result.end(); ++it)
    *it = std::toupper(*it);

  return result;
}

} // namespace base
