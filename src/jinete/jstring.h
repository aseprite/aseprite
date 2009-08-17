/* Jinete - a GUI library
 * Copyright (C) 2003-2009 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JSTRING_H_INCLUDED
#define JINETE_JSTRING_H_INCLUDED

#include <string>

/**
 * A string of characters.
 */
class jstring : public std::basic_string<char>
{
public:
  static const char separator;

  jstring() {
  }

  explicit jstring(int length)
    : std::string(length, static_cast<char>(0))
  {
  }

  jstring(const jstring& str)
    : std::string(str)
  {
  }

  jstring(const std::string& str)
    : std::string(str)
  {
  }

  jstring(const char* str)
    : std::string(str)
  {
  }

  jstring(const char* str, int length)
    : std::string(str, length)
  {
  }

  char front() const { return *begin(); }
  char back() const { return *(end()-1); }

  void tolower();
  void toupper();
  
  jstring filepath() const;
  jstring filename() const;
  jstring extension() const;
  jstring filetitle() const;

  jstring operator/(const jstring& component) const;
  jstring& operator/=(const jstring& component);
  void remove_separator();
  void fix_separators();
  bool has_extension(const jstring& csv_extensions) const;

  template<typename List>
  void split(value_type separator, List& result) const
  {
    jstring tok;

    tok.reserve(size());

    for (const_iterator it=begin(); it!=end(); ++it) {
      if (*it == separator) {
	result.push_back(tok);
	tok.clear();
      }
      else
	tok.push_back(*it);
    }
    if (!tok.empty())
	result.push_back(tok);
  }

  static bool is_separator(char c) {
    return (c == '\\' || c == '/');
  }
  
};

inline jstring operator+(const jstring& _s1, const char* _s2)
{
  jstring _res(_s1);
  _res.append(jstring(_s2));
  return _res;
}

inline jstring operator+(const char* _s1, const jstring& _s2)
{
  jstring _res(_s1);
  _res.append(_s2);
  return _res;
}

#endif // ASE_JINETE_STRING_H
