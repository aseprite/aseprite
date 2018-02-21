// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/font_path.h"

namespace app {

void get_font_dirs(base::paths& fontDirs)
{
  // TODO use a Cocoa API to get the list of paths
  fontDirs.push_back("~/Library/Fonts");
  fontDirs.push_back("/Library/Fonts");
  fontDirs.push_back("/System/Library/Fonts/");
}

} // namespace app
