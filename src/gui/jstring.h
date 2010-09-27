// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JSTRING_H_INCLUDED
#define GUI_JSTRING_H_INCLUDED

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
