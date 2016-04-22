// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _DEBUG

#include "base/debug.h"

#include "base/convert_to.h"
#include "base/string.h"

#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
#endif

int base_assert(const char* condition, const char* file, int lineNum)
{
#ifdef _WIN32

  std::vector<wchar_t> buf(MAX_PATH);
  GetModuleFileNameW(NULL, &buf[0], MAX_PATH);

  int ret = _CrtDbgReportW(_CRT_ASSERT,
    base::from_utf8(file).c_str(),
    lineNum,
    &buf[0],
    base::from_utf8(condition).c_str());

  return (ret == 1 ? 1: 0);

#else

  std::string text = "Assertion failed: ";
  text += condition;
  text += ", file ";
  text += file;
  text += ", line ";
  text += base::convert_to<std::string>(lineNum);
  std::cerr << text << std::endl;
  return 1;

#endif
}

void base_trace(const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  char buf[4096];
  vsprintf(buf, msg, ap);
  va_end(ap);

#ifdef _WIN32
  _CrtDbgReport(_CRT_WARN, NULL, 0, NULL, buf);
#endif

  std::cerr << buf << std::flush;
}

#endif
