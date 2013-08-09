// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/launcher.h"
#include "base/exception.h"

#ifdef WIN32
#include <windows.h>
#ifndef SEE_MASK_DEFAULT
#define SEE_MASK_DEFAULT 0x00000000
#endif

static int win32_shell_execute(const char* verb, const char* file, const char* params)
{
  SHELLEXECUTEINFO sh;
  ZeroMemory((LPVOID)&sh, sizeof(sh));
  sh.cbSize = sizeof(sh);
  sh.fMask = SEE_MASK_DEFAULT;
  sh.lpVerb = verb;
  sh.lpFile = file;
  sh.lpParameters = params;
  sh.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteEx(&sh)) {
    int ret = GetLastError();
#if 0
    if (ret != 0) {
      DWORD flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS;
      LPSTR msgbuf;

      if (FormatMessageA(flags, NULL, ret,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         reinterpret_cast<LPSTR>(&msgbuf),
                         0, NULL)) {
        ui::Alert::show("Problem<<Cannot open:<<%s<<%s||&Close", file, msgbuf);
        LocalFree(msgbuf);

        ret = 0;
      }
    }
#endif
    return ret;
  }
  else
    return 0;
}
#endif  // WIN32

namespace base {
namespace launcher {

bool open_url(const std::string& url)
{
  return open_file(url);
}

bool open_file(const std::string& file)
{
  int ret = -1;

#ifdef WIN32

  ret = win32_shell_execute("open", file.c_str(), NULL);

#elif __APPLE__

  ret = system(("open \"" + file + "\"").c_str());

#else

  ret = system(("xdg-open \"" + file + "\"").c_str());

#endif

  return (ret == 0);
}

bool open_folder(const std::string& file)
{
#ifdef WIN32
  int ret = win32_shell_execute(NULL, "explorer", ("/e,/select,\"" + file + "\"").c_str());
  return (ret == 0);
#else
  return false;
#endif
}

bool support_open_folder()
{
#ifdef WIN32
  return true;
#else
  return false;
#endif
}

} // namespace launcher
} // namespace base
