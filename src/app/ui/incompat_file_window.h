// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_INCOMPAT_FILE_WINDOW_H_INCLUDED
#define APP_UI_INCOMPAT_FILE_WINDOW_H_INCLUDED
#pragma once

#include "gfx/size.h"

#include "incompat_file.xml.h"

#include <string>

namespace app {

  // Shows the window to offer a solution for forward compatibility
  // (don't save/overwrite files that were saved with future Aseprite
  // versions/unknown data in the original .aseprite file).
  class IncompatFileWindow : public app::gen::IncompatFile {
  public:
    void show(std::string incompatibilities = std::string());
  };

}

#endif
