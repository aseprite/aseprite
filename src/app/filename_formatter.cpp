/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#include "app/filename_formatter.h"

#include "base/convert_to.h"
#include "base/path.h"
#include "base/replace_string.h"

#include <vector>

namespace app {

std::string filename_formatter(
  const std::string& format,
  const std::string& filename,
  const std::string& layerName,
  int frame, bool replaceFrame)
{
  std::string path = base::get_file_path(filename);
  if (path.empty())
    path = ".";

  std::string output = format;
  base::replace_string(output, "{fullname}", filename);
  base::replace_string(output, "{path}", path);
  base::replace_string(output, "{name}", base::get_file_name(filename));
  base::replace_string(output, "{title}", base::get_file_title(filename));
  base::replace_string(output, "{extension}", base::get_file_extension(filename));
  base::replace_string(output, "{layer}", layerName);

  if (replaceFrame) {
    size_t i = output.find("{frame");
    if (i != std::string::npos) {
      size_t j = output.find("}", i+6);
      if (j != std::string::npos) {
        std::string from = output.substr(i, j - i + 1);
        if (frame >= 0) {
          std::vector<char> to(32);
          int offset = std::strtol(from.c_str()+6, NULL, 10);

          std::sprintf(&to[0], "%0*d", (int(j)-int(i+6)), frame + offset);
          base::replace_string(output, from, &to[0]);
        }
        else
          base::replace_string(output, from, "");
      }
    }
  }

  return output;
}

std::string set_frame_format(
  const std::string& format,
  const std::string& newFrameFormat)
{
  std::string output = format;

  size_t i = output.find("{frame");
  if (i != std::string::npos) {
    size_t j = output.find("}", i+6);
    if (j != std::string::npos) {
      output.replace(i, j - i + 1, newFrameFormat);
    }
  }

  return output;
}

std::string add_frame_format(
  const std::string& format,
  const std::string& newFrameFormat)
{
  std::string output = format;

  size_t i = output.find("{frame");
  if (i == std::string::npos) {
    output =
      base::join_path(
        base::get_file_path(format),
        base::get_file_title(format))
      + newFrameFormat + "." + 
      base::get_file_extension(format);
  }

  return output;
}

} // namespace app
