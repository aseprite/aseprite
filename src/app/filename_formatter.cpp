// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/filename_formatter.h"

#include "app/file/file.h"
#include "app/file/split_filename.h"
#include "base/convert_to.h"
#include "base/fs.h"
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

static bool autodetect_frame_format(const std::string& filename,
                                    std::string& left,
                                    std::string& frameFormat,
                                    std::string& right,
                                    int& frameBase)
{
  int frameWidth = 0;
  frameBase = split_filename(filename, left, right, frameWidth);
  if (frameBase >= 0) {
    std::vector<char> buf(32);
    std::sprintf(&buf[0], "{frame%0*d}", frameWidth, frameBase);
    frameFormat = std::string(&buf[0]);
    return true;
  }
  else
    return false;
}

bool get_frame_info_from_filename_format(
  const std::string& format, int* frameBase, int* width)
{
  const char* frameKey = "{frame";
  size_t i = format.find(frameKey);
  if (i != std::string::npos) {
    int keyLen = std::strlen(frameKey);

    size_t j = format.find("}", i+keyLen);
    if (j != std::string::npos) {
      std::string frameStr = format.substr(i, j - i + 1);

      if (frameBase)
        *frameBase = std::strtol(frameStr.c_str()+keyLen, NULL, 10);

      if (width)
        *width = (int(j) - int(i+keyLen));
    }
    return true;
  }
  else
    return false;
}

bool is_template_in_filename(const std::string& format)
{
  std::vector<std::string> formats{
    "{fullname}",
    "{path}",
    "{name}",
    "{title}",
    "{extension}",
    "{layer}",
    "{slice}",
    "{tag}",
    "{innertag}",
    "{outertag}",
    "{frame}",
    "{tagframe}"
  };
  for (int i = 0; i < formats.size(); i++) {
    if (format.find(formats[i]) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool is_tag_in_filename_format(const std::string& format)
{
  return (format.find("{tag}") != std::string::npos);
}

bool is_layer_in_filename_format(const std::string& format)
{
  return (format.find("{layer}") != std::string::npos);
}

bool is_group_in_filename_format(const std::string& format)
{
  return (format.find("{group}") != std::string::npos);
}

bool is_slice_in_filename_format(const std::string& format)
{
  return (format.find("{slice}") != std::string::npos);
}

std::string filename_formatter(
  const std::string& format,
  FilenameInfo& info,
  const bool replaceFrame)
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
  base::replace_string(output, "{group}", info.groupName());
  base::replace_string(output, "{slice}", info.sliceName());

  if (replaceFrame) {
    base::replace_string(output, "{tag}", info.innerTagName());
    base::replace_string(output, "{innertag}", info.innerTagName());
    base::replace_string(output, "{outertag}", info.outerTagName());
    base::replace_string(output, "{duration}", std::to_string(info.duration()));
    replace_frame("{frame", info.frame(), output);
    replace_frame("{tagframe", info.tagFrame(), output);
  }

  return output;
}

std::string get_default_filename_format(
  std::string& filename,
  const bool withPath,
  const bool hasFrames,
  const bool hasLayer,
  const bool hasTag)
{
  std::string format;

  if (withPath)
    format += "{path}/";

  format += "{title}";

  if (hasLayer)
    format += " ({layer})";

  if (hasTag)
    format += " #{tag}";

  if (hasFrames && is_static_image_format(filename) &&
      filename.find("{frame") == std::string::npos &&
      filename.find("{tagframe") == std::string::npos) {
    const bool autoFrameFromLastDigit =
      (!hasLayer &&
       !hasTag);

    // Check if we already have a frame number at the end of the
    // filename (e.g. output01.png)
    int frameBase = -1, frameWidth = 0;
    std::string left, frameFormat, right;

    if (autoFrameFromLastDigit &&
        autodetect_frame_format(
          filename, left, frameFormat, right, frameBase)) {
      if (hasLayer || hasTag)
        format += " ";
      format += frameFormat;

      // Remove the frame number from the filename part.
      filename = left;
      filename += right;
    }
    // Check if there is already a {frame} tag in the filename
    else if (get_frame_info_from_filename_format(filename, &frameBase, &frameWidth)) {
      // Do nothing
    }
    else {
      if (hasLayer || hasTag)
        format += " {frame}";
      else
        format += "{frame1}";
    }
  }

  format += ".{extension}";
  return format;
}

std::string get_default_filename_format_for_sheet(
  const std::string& filename,
  const bool hasFrames,
  const bool hasLayer,
  const bool hasTag)
{
  std::string format = "{title}";

  if (hasLayer)
    format += " ({layer})";

  if (hasTag)
    format += " #{tag}";

  if (hasFrames) {
    int frameBase, frameWidth;

    // Check if there is already a {frame} tag in the filename
    if (get_frame_info_from_filename_format(filename, &frameBase, &frameWidth)) {
      // Do nothing
    }
    else {
      format += " {frame}";
    }
  }

  format += ".{extension}";
  return format;
}

std::string get_default_tagname_format_for_sheet()
{
  return "{tag}";
}

std::string replace_frame_number_with_frame_format(const std::string& filename)
{
  std::string result = filename;

  if (is_static_image_format(filename) &&
      filename.find("{frame") == std::string::npos &&
      filename.find("{tagframe") == std::string::npos) {
    std::string left, frameFormat, right;
    int frameBase = -1;

    if (!autodetect_frame_format(
          filename, left, frameFormat, right, frameBase)) {
      frameFormat = "{frame1}";
    }
    result = left;
    result += frameFormat;
    result += right;
  }

  return result;
}

} // namespace app
