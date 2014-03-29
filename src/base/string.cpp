// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/string.h"
#include <cassert>
#include <cctype>
#include <vector>

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

#else

// Based on Allegro Unicode code (allegro/src/unicode.c)
static size_t insert_utf8_char(string* result, wchar_t chr)
{
  int size, bits, b, i;

  if (chr < 128) {
    if (result)
      result->push_back(chr);
    return 1;
  }

  bits = 7;
  while (chr >= (1<<bits))
    bits++;

  size = 2;
  b = 11;

  while (b < bits) {
    size++;
    b += 5;
  }

  if (result) {
    b -= (7-size);
    int firstbyte = chr>>b;
    for (i=0; i<size; i++)
      firstbyte |= (0x80>>i);

    result->push_back(firstbyte);

    for (i=1; i<size; i++) {
      b -= 6;
      result->push_back(0x80 | ((chr>>b)&0x3F));
    }
  }

  return size;
}

string to_utf8(const std::wstring& src)
{
  std::wstring::const_iterator it, begin = src.begin();
  std::wstring::const_iterator end = src.end();

  // Get required size to reserve a string so string::push_back()
  // doesn't need to reallocate its data.
  size_t required_size = 0;
  for (it = begin; it != end; ++it)
    required_size += insert_utf8_char(NULL, *it);
  if (!required_size)
    return "";

  string result;
  result.reserve(++required_size);
  for (it = begin; it != end; ++it)
    insert_utf8_char(&result, *it);
  return result;
}

std::wstring from_utf8(const string& src)
{
  int required_size = utf8_length(src);
  std::vector<wchar_t> buf(++required_size);
  std::vector<wchar_t>::iterator buf_it = buf.begin();
  std::vector<wchar_t>::iterator buf_end = buf.end();
  utf8_const_iterator it(src.begin());
  utf8_const_iterator end(src.end());

  while (it != end) {
    assert(buf_it != buf_end);
    *buf_it = *it;
    ++buf_it;
    ++it;
  }

  return std::wstring(&buf[0]);
}

#endif

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
