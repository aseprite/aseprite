/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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
 *   * Neither the name of the Jinete nor the names of its contributors may
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

#include <algorithm>
#include <iterator>
#include <cctype>

#include "allegro/base.h"

#include "jinete/jstring.h"

#if defined ALLEGRO_WINDOWS
  const char jstring::separator = '\\';
#else
  const char jstring::separator = '/';
#endif

void jstring::tolower()
{
  // std::transform(begin(), end(), std::back_inserter(res), std::tolower);
  for (iterator it=begin(); it!=end(); ++it)
    *it = std::tolower(*it);
}

void jstring::toupper()
{
  for (iterator it=begin(); it!=end(); ++it)
    *it = std::toupper(*it);
}

jstring jstring::filepath() const
{
  const_reverse_iterator rit;
  jstring res;

  for (rit=rbegin(); rit!=rend(); ++rit)
    if (is_separator(*rit))
      break;

  if (rit != rend()) {
    ++rit;
    std::copy(begin(), const_iterator(rit.base()),
	      std::back_inserter(res));
  }

  return res;
}

jstring jstring::filename() const
{
  const_reverse_iterator rit;
  jstring res;

  for (rit=rbegin(); rit!=rend(); ++rit)
    if (is_separator(*rit))
      break;

  std::copy(const_iterator(rit.base()), end(),
	    std::back_inserter(res));

  return res;
}

jstring jstring::extension() const
{
  const_reverse_iterator rit;
  jstring res;

  // search for the first dot from the end of the string
  for (rit=rbegin(); rit!=rend(); ++rit) {
    if (is_separator(*rit))
      return res;
    else if (*rit == '.')
      break;
  }

  if (rit != rend()) {
    std::copy(const_iterator(rit.base()), end(),
	      std::back_inserter(res));
  }

  return res;
}

jstring jstring::filetitle() const
{
  const_reverse_iterator rit;
  const_iterator last_dot = end();
  jstring res;

  for (rit=rbegin(); rit!=rend(); ++rit) {
    if (is_separator(*rit))
      break;
    else if (*rit == '.' && last_dot == end())
      last_dot = rit.base()-1;
  }

  for (const_iterator it(rit.base()); it!=end(); ++it) {
    if (it == last_dot)
      break;
    else
      res.push_back(*it);
  }

  return res;
}

jstring jstring::operator/(const jstring& component) const
{
  jstring res(*this);
  res /= component;
  return res;
}

/**
 * Adds a the file name @a component in the path (this string)
 * separated with a slash.
 */
jstring& jstring::operator/=(const jstring& component)
{
  if (!empty() && !is_separator(back()))
    push_back(jstring::separator);

  operator+=(component);
  return *this;
}

void jstring::remove_separator()
{
  while (!empty() && is_separator(back()))
    erase(end()-1);
}

void jstring::fix_separators()
{
  std::replace_if(begin(), end(), jstring::is_separator, jstring::separator);
}

bool jstring::has_extension(const jstring& csv_extensions) const
{
  if (!empty()) {
    jstring ext = extension();
    ext.tolower();

    int extsz = ext.size();
    jstring::const_iterator p =
      std::search(csv_extensions.begin(),
		  csv_extensions.end(),
		  ext.begin(), ext.end());

    if ((p != csv_extensions.end()) &&
	((p+extsz) == csv_extensions.end() || *(p+extsz) == ',') &&
	(p == csv_extensions.begin() || *(p-1) == ','))
      return true;
  }
  return false;
}
