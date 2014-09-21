/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/split_filename.h"
#include "base/convert_to.h"
#include "base/path.h"
#include "base/string.h"

#include <cstring>

namespace app {

// Splits a file-name like "my_ani0000.pcx" to "my_ani" and ".pcx",
// returning the number of the center; returns "-1" if the function
// can't split anything
int split_filename(const char* filename, std::string& left, std::string& right, int& width)
{
  left = base::join_path(
    base::get_file_path(filename),
    base::get_file_title(filename));
  right = base::get_file_extension(filename);
  if (!right.empty())
    right.insert(right.begin(), '.');

  // Remove all trailing numbers in the "left" side, and pass they to "buf".
  std::string result_str;
  width = 0;
  for (;;) {
    // Get the last utf-8 character
    int chr = 0;
    base::utf8_const_iterator begin(left.begin()), end(left.end());
    base::utf8_const_iterator it(begin), prev(begin);
    for (; it != end; prev=it, ++it)
      chr = *it;

    if ((chr >= '0') && (chr <= '9')) {
      result_str.insert(result_str.begin(), chr);
      width++;

      left.erase(std::distance(begin, prev));
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
