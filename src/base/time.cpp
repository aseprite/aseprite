// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
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
  #include <sys/time.h>
#endif

#if __APPLE__
  #include <mach/mach_time.h>
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

tick_t current_tick()
{
#if _WIN32
  // TODO use GetTickCount64 (available from Vista)
  return GetTickCount();
#elif defined(__APPLE__)
  static mach_timebase_info_data_t timebase = { 0, 0 };
  if (timebase.denom == 0)
    (void)mach_timebase_info(&timebase);
  return tick_t(double(mach_absolute_time()) * double(timebase.numer) / double(timebase.denom) / 1.0e6);
#else
  // TODO use clock_gettime(CLOCK_MONOTONIC, &now); if it's possible
  struct timeval now;
  gettimeofday(&now, nullptr);
  return now.tv_sec*1000 + now.tv_usec/1000;
#endif
}

} // namespace base
