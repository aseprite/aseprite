// Aseprite
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

} // namespace app

#endif
