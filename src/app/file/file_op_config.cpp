// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file_op_config.h"

#include "app/color_spaces.h"

namespace app {

void FileOpConfig::fillFromPreferences()
{
  preserveColorProfile = Preferences::instance().color.manage();
  filesWithProfile = Preferences::instance().color.filesWithProfile();
  missingProfile = Preferences::instance().color.missingProfile();
  newBlend = Preferences::instance().experimental.newBlend();
  defaultSliceColor = Preferences::instance().slices.defaultColor();
  workingCS = get_working_rgb_space_from_preferences();
}

} // namespace app
