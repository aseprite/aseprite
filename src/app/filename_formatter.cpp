// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/filename_formatter.h"

#include "base/convert_to.h"
#include "base/path.h"
#include "base/replace_string.h"

#include <cstdlib>
#include <cstring>
#include <vector>

namespace app {

static bool replace_frame(const char* frameKey, // E.g. = "{frame"
                          int frameBase,
                          std::string& str)
{
  size_t i = str.find(frameKey);
  if (i != std::string::npos) {
    int keyLen = std::strlen(frameKey);

    size_t j = str.find("}", i+keyLen);
    if (j != std::string::npos) {
      std::string from = str.substr(i, j - i + 1);
      if (frameBase >= 0) {
        std::vector<char> to(32);
        int offset = std::strtol(from.c_str()+keyLen, NULL, 10);

        std::sprintf(&to[0], "%0*d", (int(j)-int(i+keyLen)), frameBase + offset);
        base::replace_string(str, from, &to[0]);
      }
      else
        base::replace_string(str, from, "");
    }
    return true;
  }
  else
    return false;
}

std::string filename_formatter(
  const std::string& format,
  FilenameInfo& info,
  bool replaceFrame)
{
  const std::string& filename = info.filename();
  std::string path = base::get_file_path(filename);
  if (path.empty())
    path = ".";

  std::string output = format;
  base::replace_string(output, "{fullname}", filename);
  base::replace_string(output, "{path}", path);
  base::replace_string(output, "{name}", base::get_file_name(filename));
  base::replace_string(output, "{title}", base::get_file_title(filename));
  base::replace_string(output, "{extension}", base::get_file_extension(filename));
  base::replace_string(output, "{layer}", info.layerName());

  if (replaceFrame) {
    base::replace_string(output, "{tag}", info.innerTagName());
    base::replace_string(output, "{innertag}", info.innerTagName());
    base::replace_string(output, "{outertag}", info.outerTagName());
    replace_frame("{frame", info.frame(), output);
    replace_frame("{tagframe", info.tagFrame(), output);
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
  size_t j = output.find("{tagframe");
  if (i == std::string::npos &&
      j == std::string::npos) {
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
