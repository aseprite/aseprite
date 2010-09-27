// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JFONT_H_INCLUDED
#define GUI_JFONT_H_INCLUDED

#include "gui/jbase.h"

struct BITMAP;
struct FONT;

struct FONT *ji_font_load(const char *filepathname);
struct FONT *ji_font_load_bmp(const char *filepathname);
struct FONT *ji_font_load_ttf(const char *filepathname);

int ji_font_get_size(struct FONT *f);
int ji_font_set_size(struct FONT *f, int height);

int ji_font_get_aa_mode(struct FONT *f);
int ji_font_set_aa_mode(struct FONT *f, int mode);

bool ji_font_is_fixed(struct FONT *f);
bool ji_font_is_scalable(struct FONT *f);

const int *ji_font_get_available_fixed_sizes(struct FONT *f, int *n);

int ji_font_get_char_extra_spacing(struct FONT *f);
void ji_font_set_char_extra_spacing(struct FONT *f, int spacing);

int ji_font_char_len(struct FONT *f, int chr);
int ji_font_text_len(struct FONT *f, const char *text);

#endif
