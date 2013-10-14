// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/string.h"
#include <vector>
#include <cctype>

#ifdef WIN32
#include <windows.h>
#endif

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

#ifdef WIN32

string to_utf8(const std::wstring& src)
{
  int required_size =
    ::WideCharToMultiByte(CP_UTF8, 0,
                          src.c_str(), src.size(),
                          NULL, 0, NULL, NULL);

  if (required_size == 0)
    return string();

  std::vector<char> buf(++required_size);

  ::WideCharToMultiByte(CP_UTF8, 0,
                        src.c_str(), src.size(),
                        &buf[0], required_size,
                        NULL, NULL);

  return base::string(&buf[0]);
}

std::wstring from_utf8(const string& src)
{
  int required_size =
    MultiByteToWideChar(CP_UTF8, 0,
                        src.c_str(), src.size(),
                        NULL, 0);

  if (required_size == 0)
    return std::wstring();

  std::vector<wchar_t> buf(++required_size);

  ::MultiByteToWideChar(CP_UTF8, 0,
                        src.c_str(), src.size(),
                        &buf[0], required_size);

  return std::wstring(&buf[0]);
}

#endif  // WIN32

int utf8_length(const string& utf8string)
{
  utf8_const_iterator it(utf8string.begin());
  utf8_const_iterator end(utf8string.end());
  int c = 0;

  while (it != end)
    ++it, ++c;

  return c;
}

} // namespace base
