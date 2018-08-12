// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/split_filename.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/string.h"

#include <cstring>

namespace app {

// Splits a file-name like "my_ani0000.pcx" to "my_ani" and ".pcx",
// returning the number of the center; returns "-1" if the function
// can't split anything
int split_filename(const std::string& filename, std::string& left, std::string& right, int& width)
{
  left = base::get_file_title_with_path(filename);
  right = base::get_file_extension(filename);
  if (!right.empty())
    right.insert(right.begin(), '.');

  // Remove all trailing numbers in the "left" side, and pass they to "result_str".
  std::string result_str;
  width = 0;
  for (;;) {
    // Get the last UTF-8 character (as we don't have a
    // reverse_iterator, we iterate from the beginning)
    int chr = 0;
    base::utf8_const_iterator begin(left.begin()), end(left.end());
    base::utf8_const_iterator it(begin), prev(begin);
    for (; it != end; prev=it, ++it)
      chr = *it;

    if ((chr >= '0') && (chr <= '9')) {
      result_str.insert(result_str.begin(), chr);
      width++;

      left.erase(prev - begin);
    }
    else
      break;
  }

  // Convert the "buf" to integer and return it.
  if (!result_str.empty()) {
    return base::convert_to<int>(result_str);
  }
  else
    return -1;
}

} // namespace app
