// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_PIC_FILE_H_INCLUDED
#define APP_UTIL_PIC_FILE_H_INCLUDED
#pragma once

namespace doc {
class Image;
class Palette;
} // namespace doc

namespace app {

doc::Image* load_pic_file(const char* filename, int* x, int* y, doc::Palette** palette);
int save_pic_file(const char* filename,
                  int x,
                  int y,
                  const doc::Palette* palette,
                  const doc::Image* image);

} // namespace app

#endif
