// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
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

#ifdef _WIN32
  #include <windows.h>
#endif

namespace base {

std::string string_to_lower(const std::string& original)
{
  std::wstring result(from_utf8(original));
  auto it(result.begin());
  auto end(result.end());
  while (it != end) {
    *it = std::tolower(*it);
    ++it;
  }
  return to_utf8(result);
}

std::string string_to_upper(const std::string& original)
{
  std::wstring result(from_utf8(original));
  auto it(result.begin());
  auto end(result.end());
  while (it != end) {
    *it = std::toupper(*it);
    ++it;
  }
  return to_utf8(result);
}

#ifdef _WIN32

std::string to_utf8(const std::wstring& src)
{
  int required_size =
    ::WideCharToMultiByte(CP_UTF8, 0,
      src.c_str(), (int)src.size(),
      NULL, 0, NULL, NULL);

  if (required_size == 0)
    return std::string();

  std::vector<char> buf(++required_size);

  ::WideCharToMultiByte(CP_UTF8, 0,
    src.c_str(), (int)src.size(),
    &buf[0], required_size,
    NULL, NULL);

  return std::string(&buf[0]);
}

std::wstring from_utf8(const std::string& src)
{
  int required_size =
    MultiByteToWideChar(CP_UTF8, 0,
      src.c_str(), (int)src.size(),
      NULL, 0);

  if (required_size == 0)
    return std::wstring();

  std::vector<wchar_t> buf(++required_size);

  ::MultiByteToWideChar(CP_UTF8, 0,
    src.c_str(), (int)src.size(),
    &buf[0], required_size);

  return std::wstring(&buf[0]);
}

#else

// Based on Allegro Unicode code (allegro/src/unicode.c)
static std::size_t insert_utf8_char(std::string* result, wchar_t chr)
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

std::string to_utf8(const std::wstring& src)
{
  std::wstring::const_iterator it, begin = src.begin();
  std::wstring::const_iterator end = src.end();

  // Get required size to reserve a string so string::push_back()
  // doesn't need to reallocate its data.
  std::size_t required_size = 0;
  for (it = begin; it != end; ++it)
    required_size += insert_utf8_char(NULL, *it);
  if (!required_size)
    return "";

  std::string result;
  result.reserve(++required_size);
  for (it = begin; it != end; ++it)
    insert_utf8_char(&result, *it);
  return result;
}

std::wstring from_utf8(const std::string& src)
{
  int required_size = utf8_length(src);
  std::vector<wchar_t> buf(++required_size);
  std::vector<wchar_t>::iterator buf_it = buf.begin();
#ifdef _DEBUG
  std::vector<wchar_t>::iterator buf_end = buf.end();
#endif
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

int utf8_length(const std::string& utf8string)
{
  utf8_const_iterator it(utf8string.begin());
  utf8_const_iterator end(utf8string.end());
  int c = 0;

  while (it != end)
    ++it, ++c;

  return c;
}

int utf8_icmp(const std::string& a, const std::string& b, int n)
{
  utf8_const_iterator a_it(a.begin());
  utf8_const_iterator a_end(a.end());
  utf8_const_iterator b_it(b.begin());
  utf8_const_iterator b_end(b.end());
  int i = 0;

  for (; (n == 0 || i < n) && a_it != a_end && b_it != b_end; ++a_it, ++b_it, ++i) {
    int a_chr = std::tolower(*a_it);
    int b_chr = std::tolower(*b_it);

    if (a_chr < b_chr)
      return -1;
    else if (a_chr > b_chr)
      return 1;
  }

  if (n > 0 && i == n)
    return 0;
  else if (a_it == a_end && b_it == b_end)
    return 0;
  else if (a_it == a_end)
    return -1;
  else
    return 1;
}

} // namespace base
