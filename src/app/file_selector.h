// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_SELECTOR_H_INCLUDED
#define APP_FILE_SELECTOR_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "doc/pixel_ratio.h"

#include <string>

namespace ui {
  class ComboBox;
}

namespace app {

  enum class FileSelectorType { Open, OpenMultiple, Save };

  bool show_file_selector(
    const std::string& title,
    const std::string& initialPath,
    const base::paths& extensions,
    FileSelectorType type,
    base::paths& output);

  std::string get_initial_path_to_select_filename(
    const std::string& initialFilename);

  // Get/set the directory used for the file selector (custom one, and
  // the X11 native one).
  std::string get_current_dir_for_file_selector();
  void set_current_dir_for_file_selector(const std::string& dirPath);

} // namespace app

#endif
