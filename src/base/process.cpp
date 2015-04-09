// Aseprite Base Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/process.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace base {

#ifdef _WIN32

pid get_current_process_id()
{
  return (pid)GetCurrentProcessId();
}

bool is_process_running(pid pid)
{
  bool running = false;

  HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
  if (handle) {
    DWORD exitCode = 0;
    if (GetExitCodeProcess(handle, &exitCode)) {
      running = (exitCode == STILL_ACTIVE);
    }
    CloseHandle(handle);
  }

  return running;
}

#else

pid get_current_process_id()
{
  return (pid)getpid();
}

bool is_process_running(pid pid)
{
  return (kill(pid, 0) == 0);
}

#endif

} // namespace base
