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

#ifndef VACA_STRING_H
#define VACA_STRING_H

#include "Vaca/base.h"
#include <vector>

namespace Vaca {

/**
   @defgroup string_utils String Utilities
   @{
*/

  // ============================================================
  // UTILITARY
  // ============================================================

  VACA_DLL std::string format_string(const char* fmt, ...);
  VACA_DLL std::wstring format_string(const wchar_t* fmt, ...);

  VACA_DLL String trim_string(const String& str);
  VACA_DLL String trim_string(const Char* str);

  VACA_DLL std::string to_utf8(const String& string);
  VACA_DLL String from_utf8(const std::string& string);

  // Split a string in parts
  VACA_DLL void split_string(const String& string, std::vector<String>& parts, const String& separators);

  // ============================================================
  // CONVERSION
  // ============================================================

  // you have to use a specialization
  template<typename To, typename From>
  To convert_to(const From& from) {
    enum { not_supported = 1/(1 == 0) }; // static_assert(false)
  }

  // convert from String
  template<> VACA_DLL std::string   convert_to(const String& from);
  template<> VACA_DLL int           convert_to(const String& from);
  template<> VACA_DLL long          convert_to(const String& from);
  template<> VACA_DLL unsigned int  convert_to(const String& from);
  template<> VACA_DLL unsigned long convert_to(const String& from);
  template<> VACA_DLL float         convert_to(const String& from);
  template<> VACA_DLL double        convert_to(const String& from);
  template<> VACA_DLL std::string   convert_to(const Char* const& from);

  template<> inline std::string convert_to(Char* const& from) {
    return convert_to<std::string, const Char*>(from);
  }

  // Convert to String
  template<> VACA_DLL String convert_to(const std::string& from);
  template<> VACA_DLL String convert_to(const int& from);
  template<> VACA_DLL String convert_to(const long& from);
  template<> VACA_DLL String convert_to(const unsigned int& from);
  template<> VACA_DLL String convert_to(const unsigned long& from);
  template<> VACA_DLL String convert_to(const float& from);
  template<> VACA_DLL String convert_to(const double& from);
  template<> VACA_DLL String convert_to(const char* const& from);

  template<> inline String convert_to(char* const& from) {
    return convert_to<String, const char*>(from);
  }

  // Copy to a raw string
  VACA_DLL void copy_string_to(const String& src, Char* dest, int size);

  // ============================================================
  // FILE NAMES, PATHS AND URLS
  // ============================================================

  VACA_DLL String operator/(const String& path, const String& comp);
  VACA_DLL String& operator/=(String& path, const String& comp);

  VACA_DLL String file_path(const String& fullpath);
  VACA_DLL String file_name(const String& fullpath);
  VACA_DLL String file_extension(const String& fullpath);
  VACA_DLL String file_title(const String& fullpath);

  VACA_DLL String url_host(const String& url);
  VACA_DLL String url_object(const String& url);

  VACA_DLL String encode_url(const String& url);
  VACA_DLL String decode_url(const String& url);

/** @} */

} // namespace Vaca

#endif // VACA_STRING_H
