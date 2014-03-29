// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_FONT_H_INCLUDED
#define UI_FONT_H_INCLUDED
#pragma once

#include "ui/base.h"

struct FONT;

namespace ui {

  FONT* ji_font_load(const char* filepathname);
  FONT* ji_font_load_bmp(const char* filepathname);
  FONT* ji_font_load_ttf(const char* filepathname);

  int ji_font_get_size(FONT* f);
  int ji_font_set_size(FONT* f, int height);

  int ji_font_get_aa_mode(FONT* f);
  int ji_font_set_aa_mode(FONT* f, int mode);

  bool ji_font_is_fixed(FONT* f);
  bool ji_font_is_scalable(FONT* f);

  const int *ji_font_get_available_fixed_sizes(FONT* f, int* n);

  int ji_font_get_char_extra_spacing(FONT* f);
  void ji_font_set_char_extra_spacing(FONT* f, int spacing);

  int ji_font_char_len(FONT* f, int chr);
  int ji_font_text_len(FONT* f, const char* text);

} // namespace ui

#endif
