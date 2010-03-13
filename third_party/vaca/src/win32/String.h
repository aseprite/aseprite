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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>

std::string Vaca::to_utf8(const String& string)
{
  int required_size =
    WideCharToMultiByte(CP_UTF8, 0,
			string.c_str(), string.size(),
			NULL, 0, NULL, NULL);

  if (required_size == 0)
    return std::string();

  std::vector<char> buf(++required_size);

  WideCharToMultiByte(CP_UTF8, 0, 
		      string.c_str(), string.size(),
		      &buf[0], required_size,
		      NULL, NULL);

  return std::string(&buf[0]);
}

String Vaca::from_utf8(const std::string& string)
{
  int required_size =
    MultiByteToWideChar(CP_UTF8, 0,
			string.c_str(), string.size(),
			NULL, 0);

  if (required_size == 0)
    return String();

  std::vector<wchar_t> buf(++required_size);

  MultiByteToWideChar(CP_UTF8, 0,
		      string.c_str(), string.size(),
		      &buf[0], required_size);

  return String(&buf[0]);
}

template<> std::string Vaca::convert_to(const Char* const& from)
{
  int len = std::wcslen(from)+1;
  std::auto_ptr<char> ansiBuf(new char[len]);
  int ret = WideCharToMultiByte(CP_ACP, 0, from, len, ansiBuf.get(), len, NULL, NULL);
  if (ret == 0)
    return "";
  else
    return std::string(ansiBuf.get());
}

template<> std::string Vaca::convert_to(const String& from)
{
  int len = from.size()+1;
  std::auto_ptr<char> ansiBuf(new char[len]);
  int ret = WideCharToMultiByte(CP_ACP, 0, from.c_str(), len, ansiBuf.get(), len, NULL, NULL);
  if (ret == 0)
    return "";
  else
    return std::string(ansiBuf.get());
}

template<> String Vaca::convert_to(const char* const& from)
{
  int len = strlen(from)+1;
  std::auto_ptr<Char> wideBuf(new Char[len]);
  int ret = MultiByteToWideChar(CP_ACP, 0, from, len, wideBuf.get(), len);
  if (ret == 0)
    return L"";
  else
    return String(wideBuf.get());
}

template<> String Vaca::convert_to(const std::string& from)
{
  int len = from.size()+1;
  std::auto_ptr<Char> wideBuf(new Char[len]);
  int ret = MultiByteToWideChar(CP_ACP, 0, from.c_str(), len, wideBuf.get(), len);
  if (ret == 0)
    return L"";
  else
    return String(wideBuf.get());
}

String Vaca::encode_url(const String& url)
{
  std::auto_ptr<Char> buf;
  DWORD size = 1024;

  while (true) {
    buf = std::auto_ptr<Char>(new Char[size]);

    if (::InternetCanonicalizeUrlW(url.c_str(), buf.get(), &size, 0))
      break;

    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      throw Exception();	// TODO improve the exception type/information

    // else continue growing the buffer...
  }

  return String(buf.get());
}

String Vaca::decode_url(const String& url)
{
  std::auto_ptr<Char> buf;
  DWORD size = 1024;

  while (true) {
    buf = std::auto_ptr<Char>(new Char[size]);

    if (::InternetCanonicalizeUrlW(url.c_str(), buf.get(), &size,
				   ICU_DECODE | ICU_NO_ENCODE))
      break;

    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      throw Exception();	// TODO improve the exception type/information

    // else continue growing the buffer...
  }

  return String(buf.get());
}
