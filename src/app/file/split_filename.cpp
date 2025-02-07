// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/file/split_filename.h"
#include "base/fs.h"

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

  // Remove all trailing numbers in the "left" side.
  std::string result_str;
  width = 0;
  int num = -1;
  int order = 1;

  auto it = left.rbegin();
  auto end = left.rend();

  while (it != end) {
    const int chr = *it;
    if (chr >= '0' && chr <= '9') {
      if (num < 0)
        num = 0;

      num += order * (chr - '0');
      order *= 10;
      ++width;
      ++it;
    }
    else
      break;
  }

  if (width > 0)
    left.erase(left.end() - width, left.end());

  return num;
}

} // namespace app
