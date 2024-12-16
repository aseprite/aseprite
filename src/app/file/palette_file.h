// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_PALETTE_FILE_H_INCLUDED
#define APP_FILE_PALETTE_FILE_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "gfx/color_space.h"

#include <memory>

namespace doc {
class Palette;
}

namespace app {
struct FileOpConfig;

base::paths get_readable_palette_extensions();
base::paths get_writable_palette_extensions();

std::unique_ptr<doc::Palette> load_palette(const char* filename,
                                           const FileOpConfig* config = nullptr);
bool save_palette(const char* filename,
                  const doc::Palette* pal,
                  int columns,
                  const gfx::ColorSpaceRef& colorSpace);

} // namespace app

#endif
