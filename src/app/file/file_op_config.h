// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FILE_OP_CONFIG_H_INCLUDED
#define APP_FILE_FILE_OP_CONFIG_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/pref/preferences.h"
#include "gfx/color_space.h"

namespace app {

  // Options that came from Preferences but can be used in the non-UI thread.
  struct FileOpConfig {
    // True if we have to save/embed the color space of the document
    // in the file.
    bool preserveColorProfile = true;

    // Configuration of what to do when we load a file with a color
    // profile or without a color profile.
    app::gen::ColorProfileBehavior filesWithProfile = app::gen::ColorProfileBehavior::EMBEDDED;
    app::gen::ColorProfileBehavior missingProfile = app::gen::ColorProfileBehavior::ASSIGN;
    gfx::ColorSpacePtr workingCS = gfx::ColorSpace::MakeSRGB();

    // True if we should render each frame to save it with the new
    // blend mode.h
    bool newBlend = true;

    app::Color defaultSliceColor = app::Color::fromRgb(0, 0, 255);

    void fillFromPreferences();
  };

} // namespace app

#endif
