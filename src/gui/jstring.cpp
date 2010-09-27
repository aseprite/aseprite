// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <algorithm>
#include <iterator>
#include <cctype>

#include "allegro/base.h"

#include "gui/jstring.h"

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
