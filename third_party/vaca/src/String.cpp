// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Vaca/String.h"
#include "Vaca/Exception.h"
#include <cassert>
#include <cstdio>
#include <cwchar>
#include <cstdarg>
#include <cstdlib>
#include <cctype>
#include <vector>

#include <algorithm>
#include <memory>

using namespace Vaca;

std::string Vaca::format_string(const char* fmt, ...)
{
  std::auto_ptr<char> buf;
  int size = 512;

  while (true) {
    buf = std::auto_ptr<char>(new char[size <<= 1]);

    va_list ap;
    va_start(ap, fmt);
    #if defined(VACA_ON_WINDOWS)
      int written = _vsnprintf(buf.get(), size, fmt, ap);
    #elif defined(VACA_ON_UNIXLIKE)
      int written = vsnprintf(buf.get(), size, fmt, ap);
    #else
      #error Implement this in your platform
    #endif
    va_end(ap);

    if (written == size) {
      if (buf.get()[size] == 0)
	break;
    }
    else if (written >= 0 && written < size) {
      assert(buf.get()[written] == 0);
      break;
    }
    // else continue growing the buffer...
  }

  return std::string(buf.get());
}

std::wstring Vaca::format_string(const wchar_t* fmt, ...)
{
  std::auto_ptr<wchar_t> buf;
  int size = 512;

  while (true) {
    buf = std::auto_ptr<wchar_t>(new wchar_t[size <<= 1]);

    va_list ap;
    va_start(ap, fmt);
    #if defined(VACA_ON_WINDOWS)
      int written = _vsnwprintf(buf.get(), size, fmt, ap);
    #elif defined(VACA_ON_UNIXLIKE)
      int written = vswprintf(buf.get(), size, fmt, ap);
    #else
      #error Implement this in your platform
    #endif
    va_end(ap);

    if (written == size) {
      if (buf.get()[size] == 0)
	break;
    }
    else if (written >= 0 && written < size) {
      assert(buf.get()[written] == 0);
      break;
    }
    // else continue growing the buffer...
  }

  return std::wstring(buf.get());
}

String Vaca::trim_string(const String& str)
{
  return trim_string(str.c_str());
}

String Vaca::trim_string(const Char* str)
{
  assert(str != NULL);

  String res(str);
  while (!res.empty() && std::isspace(res.at(0)))
    res.erase(res.begin());
  while (!res.empty() && std::isspace(res.at(res.size()-1)))
    res.erase(res.end()-1);

  return res;
}

template<> int Vaca::convert_to(const String& from)
{
  return (int)std::wcstol(from.c_str(), NULL, 10);
}

template<> long Vaca::convert_to(const String& from)
{
  return (long)std::wcstol(from.c_str(), NULL, 10);
}

template<> unsigned int Vaca::convert_to(const String& from)
{
  return (unsigned int)std::wcstoul(from.c_str(), NULL, 10);
}

template<> unsigned long Vaca::convert_to(const String& from)
{
  return (unsigned long)std::wcstoul(from.c_str(), NULL, 10);
}

template<> float Vaca::convert_to(const String& from)
{
  return static_cast<float>(std::wcstod(from.c_str(), NULL));
}

template<> double Vaca::convert_to(const String& from)
{
  return std::wcstod(from.c_str(), NULL);
}

template<> std::string Vaca::convert_to(const int& from)
{
  return format_string("%d", from);
}

template<> std::wstring Vaca::convert_to(const int& from)
{
  return format_string(L"%d", from);
}

template<> String Vaca::convert_to(const long& from)
{
  return format_string(L"%ld", from);
}

template<> String Vaca::convert_to(const unsigned int& from)
{
  return format_string(L"%u", from);
}

template<> String Vaca::convert_to(const unsigned long& from)
{
  return format_string(L"%lu", from);
}

template<> String Vaca::convert_to(const float& from)
{
  return format_string(L"%.16g", from);
}

template<> String Vaca::convert_to(const double& from)
{
  return format_string(L"%.16g", from);
}

/**
   Commondly used to give strings to Win32 API or from Win32 API (in
   structures and messages).
*/
void Vaca::copy_string_to(const String& src, Char* dest, int size)
{
  std::wcsncpy(dest, src.c_str(), size);
  dest[size-1] = L'\0';
}

/**
   Concatenates two path components.

   It is useful if the string represent a path and we have to add a
   file name. For example:
   @code
   String path(L"C:\\myproject\\src");
   assert(path / L"main.h" == L"C:\\myproject\\src\\main.h");
   @endcode

   @param component
     The string to be added at the end of the string
     (separated by a slash).
*/
String Vaca::operator/(const String& path, const String& comp)
{
  String res(path);

  if (!res.empty() && *(res.end()-1) != L'/' && *(res.end()-1) != L'\\')
    res.push_back(L'\\');

  res += comp;
  return res;
}

/**
   Adds a path component at the end of the path.

   It is useful if the string represent a path and we have to add a
   file name. For example:
   @code
   String path(L"C:\\myproject\\src");
   path /= L"main.h";
   assert(path == L"C:\\myproject\\src\\main.h");
   @endcode

   @param component
     The string to be added at the end of the string
     (separated by a slash).
*/
String& Vaca::operator/=(String& path, const String& comp)
{
  if (!path.empty() && *(path.end()-1) != L'/' && *(path.end()-1) != L'\\')
    path.push_back(L'\\');

  path += comp;
  return path;
}

/**
   Returns the file path (the path of "C:\foo\main.cpp" is "C:\foo"
   without the file name).

   @see file_name
*/
String Vaca::file_path(const String& fullpath)
{
  String::const_reverse_iterator rit;
  String res;

  for (rit=fullpath.rbegin(); rit!=fullpath.rend(); ++rit)
    if (*rit == L'\\' || *rit == L'/')
      break;

  if (rit != fullpath.rend()) {
    ++rit;
    std::copy(fullpath.begin(), String::const_iterator(rit.base()),
	      std::back_inserter(res));
  }

  return res;
}

/**
   Returns the file name (the file name of "C:\foo\main.cpp" is
   "main.cpp", without the path).

   @see file_path, file_title
*/
String Vaca::file_name(const String& fullpath)
{
  String::const_reverse_iterator rit;
  String res;

  for (rit=fullpath.rbegin(); rit!=fullpath.rend(); ++rit)
    if (*rit == L'\\' || *rit == L'/')
      break;

  std::copy(String::const_iterator(rit.base()), fullpath.end(),
	    std::back_inserter(res));

  return res;
}

/**
   Returns the file extension (the extension of "C:\foo\main.cpp" is
   "cpp", without the path and its title).

   @warning
     For a file name like "pack.tar.gz" the extension is "gz".

   @see file_path, file_title
*/
String Vaca::file_extension(const String& fullpath)
{
  String::const_reverse_iterator rit;
  String res;

  // search for the first dot from the end of the string
  for (rit=fullpath.rbegin(); rit!=fullpath.rend(); ++rit) {
    if (*rit == L'\\' || *rit == L'/')
      return res;
    else if (*rit == L'.')
      break;
  }

  if (rit != fullpath.rend()) {
    std::copy(String::const_iterator(rit.base()), fullpath.end(),
	      std::back_inserter(res));
  }

  return res;
}

/**
   Returns the file title (the title of "C:\foo\main.cpp" is "main",
   without the path and without the extension).

   @warning
     For a file name like "pack.tar.gz" the title is "pack.tar".

   @see file_path, file_extension
*/
String Vaca::file_title(const String& fullpath)
{
  String::const_reverse_iterator rit;
  String::const_iterator last_dot = fullpath.end();
  String res;

  for (rit=fullpath.rbegin(); rit!=fullpath.rend(); ++rit) {
    if (*rit == L'\\' || *rit == L'/')
      break;
    else if (*rit == L'.' && last_dot == fullpath.end())
      last_dot = rit.base()-1;
  }

  for (String::const_iterator it(rit.base()); it!=fullpath.end(); ++it) {
    if (it == last_dot)
      break;
    else
      res.push_back(*it);
  }

  return res;
}

String Vaca::url_host(const String& url)
{
  String host;
  int len = url.size();
  for (int c=0; c<len; ++c) {
    if (url[c] == L':' && url[c+1] == L'/' && url[c+2] == L'/') {
      for (c+=3; c<len && url[c] != L'/'; ++c)
	host.push_back(url[c]);
    }
  }
  return host;
}

String Vaca::url_object(const String& url)
{
  String object;
  int len = url.size();
  for (int c=0; c<len; ++c) {
    if (url[c] == ':' && url[c+1] == '/' && url[c+2] == '/') {
      for (c+=3; c<len && url[c] != '/'; ++c)
	;
      for (; c<len; ++c)
	object.push_back(url[c]);
    }
  }
  return object;
}

#if defined(VACA_ON_WINDOWS)
  #include "win32/String.h"
#elif defined(VACA_ALLEGRO)
  #include "allegro/String.h"
#endif
