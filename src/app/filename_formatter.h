// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILENAME_FORMATTER_H_INCLUDED
#define APP_FILENAME_FORMATTER_H_INCLUDED
#pragma once

#include <string>

namespace app {

  class FilenameInfo {
  public:
    FilenameInfo() : m_frame(-1), m_tagFrame(-1) { }

    const std::string& filename() const { return m_filename; }
    const std::string& layerName() const { return m_layerName; }
    const std::string& groupName() const { return m_groupName; }
    const std::string& innerTagName() const { return m_innerTagName; }
    const std::string& outerTagName() const { return m_outerTagName; }
    const std::string& sliceName() const { return m_sliceName; }
    int frame() const { return m_frame; }
    int tagFrame() const { return m_tagFrame; }

    FilenameInfo& filename(const std::string& value) {
      m_filename = value;
      return *this;
    }

    FilenameInfo& layerName(const std::string& value) {
      m_layerName = value;
      return *this;
    }

    FilenameInfo& groupName(const std::string& value) {
      m_groupName = value;
      return *this;
    }

    FilenameInfo& innerTagName(const std::string& value) {
      m_innerTagName = value;
      return *this;
    }

    FilenameInfo& outerTagName(const std::string& value) {
      m_outerTagName = value;
      return *this;
    }

    FilenameInfo& sliceName(const std::string& value) {
      m_sliceName = value;
      return *this;
    }

    FilenameInfo& frame(int value) {
      m_frame = value;
      return *this;
    }

    FilenameInfo& tagFrame(int value) {
      m_tagFrame = value;
      return *this;
    }

  private:
    std::string m_filename;
    std::string m_layerName;
    std::string m_groupName;
    std::string m_innerTagName;
    std::string m_outerTagName;
    std::string m_sliceName;
    int m_frame;
    int m_tagFrame;
  };

  // Returns the information inside {frame} tag.
  // E.g. For {frame001} returns width=3 and startFrom=1
  bool get_frame_info_from_filename_format(
    const std::string& format, int* startFrom, int* width);

  // Returns true if the given filename format contains {tag}, {layer} or {group}
  bool is_tag_in_filename_format(const std::string& format);
  bool is_layer_in_filename_format(const std::string& format);
  bool is_group_in_filename_format(const std::string& format);
  bool is_slice_in_filename_format(const std::string& format);

  // If "replaceFrame" is false, this function doesn't replace all the
  // information that depends on the current frame ({frame},
  // {tagframe}, {tag}, etc.)
  std::string filename_formatter(
    const std::string& format,
    FilenameInfo& info,
    const bool replaceFrame = true);

  std::string get_default_filename_format(
    std::string& filename,
    const bool withPath,
    const bool hasFrames,
    const bool hasLayer,
    const bool hasTag);

  std::string get_default_filename_format_for_sheet(
    const std::string& filename,
    const bool hasFrames,
    const bool hasLayer,
    const bool hasTag);

} // namespace app

#endif
