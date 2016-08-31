// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CLIPBOARD_NATIVE_H_INCLUDED
#define APP_UTIL_CLIPBOARD_NATIVE_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace doc {
  class Image;
  class Mask;
  class Palette;
}

namespace app {
namespace clipboard {

void register_native_clipboard_formats();
bool has_native_clipboard_bitmap();
bool set_native_clipboard_bitmap(const doc::Image* image,
                                 const doc::Mask* mask,
                                 const doc::Palette* palette);
bool get_native_clipboard_bitmap(doc::Image** image,
                                 doc::Mask** mask,
                                 doc::Palette** palette);
bool get_native_clipboard_bitmap_size(gfx::Size* size);

} // namespace clipboard
} // namespace app

#endif
