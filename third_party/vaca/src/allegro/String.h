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

#include <allegro.h>

// TODO
// std::string Vaca::to_utf8(const String& string);
// String Vaca::from_utf8(const std::string& string);

template<> std::string Vaca::convert_to(const Char* const& from)
{
  int len = std::wcslen(from)+1;
  std::auto_ptr<char> ansiBuf(new char[len]);
  if (uconvert((const char*)from, U_UNICODE, ansiBuf.get(), U_ASCII, len))
    return std::string(ansiBuf.get());
  else
    return "";
}

template<> std::string Vaca::convert_to(const String& from)
{
  int len = from.size()+1;
  std::auto_ptr<char> ansiBuf(new char[len]);
  if (uconvert((const char*)from.c_str(), U_UNICODE, ansiBuf.get(), U_ASCII, len))
    return std::string(ansiBuf.get());
  else
    return "";
}

template<> String Vaca::convert_to(const char* const& from)
{
  int len = strlen(from)+1;
  std::auto_ptr<Char> wideBuf(new Char[len]);
  if (uconvert(from, U_ASCII, (char*)wideBuf.get(), U_UNICODE, len))
    return String(wideBuf.get());
  else
    return L"";
}

template<> String Vaca::convert_to(const std::string& from)
{
  int len = from.size()+1;
  std::auto_ptr<Char> wideBuf(new Char[len]);
  if (uconvert(from.c_str(), U_ASCII, (char*)wideBuf.get(), U_UNICODE, len))
    return String(wideBuf.get());
  else
    return L"";
}

// TODO
// String Vaca::encode_url(const String& url);
// String Vaca::decode_url(const String& url);

