// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/path.h"

#include "base/string.h"

#include <algorithm>
#include <iterator>

namespace base {

#ifdef WIN32
  const std::string::value_type path_separator = '\\';
#else
  const std::string::value_type path_separator = '/';
#endif

bool is_path_separator(std::string::value_type chr)
{
  return (chr == '\\' || chr == '/');
}

std::string get_file_path(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string res;

  for (rit=filename.rbegin(); rit!=filename.rend(); ++rit)
    if (is_path_separator(*rit))
      break;

  if (rit != filename.rend()) {
    ++rit;
    std::copy(filename.begin(), std::string::const_iterator(rit.base()),
              std::back_inserter(res));
  }

  return res;
}

std::string get_file_name(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string result;

  for (rit=filename.rbegin(); rit!=filename.rend(); ++rit)
    if (is_path_separator(*rit))
      break;

  std::copy(std::string::const_iterator(rit.base()), filename.end(),
            std::back_inserter(result));

  return result;
}

std::string get_file_extension(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string result;

  // search for the first dot from the end of the string
  for (rit=filename.rbegin(); rit!=filename.rend(); ++rit) {
    if (is_path_separator(*rit))
      return result;
    else if (*rit == '.')
      break;
  }

  if (rit != filename.rend()) {
    std::copy(std::string::const_iterator(rit.base()), filename.end(),
              std::back_inserter(result));
  }

  return result;
}

std::string get_file_title(const std::string& filename)
{
  std::string::const_reverse_iterator rit;
  std::string::const_iterator last_dot = filename.end();
  std::string result;

  for (rit=filename.rbegin(); rit!=filename.rend(); ++rit) {
    if (is_path_separator(*rit))
      break;
    else if (*rit == '.' && last_dot == filename.end())
      last_dot = rit.base()-1;
  }

  for (std::string::const_iterator it(rit.base()); it!=filename.end(); ++it) {
    if (it == last_dot)
      break;
    else
      result.push_back(*it);
  }

  return result;
}

std::string join_path(const std::string& path, const std::string& file)
{
  std::string result(path);

  // Add a separator at the end if it is necessay
  if (!result.empty() && !is_path_separator(*(result.end()-1)))
    result.push_back(path_separator);

  // Add the file
  result += file;
  return result;
}

std::string remove_path_separator(const std::string& path)
{
  std::string result(path);

  // Erase all trailing separators
  while (!result.empty() && is_path_separator(*(result.end()-1)))
    result.erase(result.end()-1);

  return result;
}

std::string fix_path_separators(const std::string& filename)
{
  std::string result(filename);

  // Replace any separator with the system path separator.
  std::replace_if(result.begin(), result.end(),
                  is_path_separator, path_separator);

  return result;
}

bool has_file_extension(const std::string& filename, const std::string& csv_extensions)
{
  if (!filename.empty()) {
    std::string ext = base::string_to_lower(get_file_extension(filename));

    int extsz = ext.size();
    std::string::const_iterator p =
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

} // namespace base
