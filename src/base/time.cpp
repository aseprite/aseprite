// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/time.h"

#if _WIN32
  #include <windows.h>
#else
  #include <ctime>
#endif

namespace base {

Time current_time()
{
#if _WIN32

  SYSTEMTIME st;
  GetLocalTime(&st);
  return Time(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

#else

  std::time_t now = std::time(nullptr);
  std::tm* t = std::localtime(&now);
  return Time(
    t->tm_year+1900, t->tm_mon+1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);

#endif
}

} // namespace base
