// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/win32_exception.h"

#include "base/string.h"

#include <windows.h>

namespace base {

Win32Exception::Win32Exception(const std::string& msg) throw()
  : Exception()
{
  DWORD errcode = GetLastError();
  LPVOID buf;

  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | // TODO Try to use a TLS buffer
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    errcode,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPWSTR)&buf,
    0, NULL);

  setMessage((msg + "\n" + to_utf8((LPWSTR)buf)).c_str());
  LocalFree(buf);
}

Win32Exception::~Win32Exception() throw()
{
}

} // namespace base
