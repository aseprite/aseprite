/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JFONT_H_INCLUDED
#define JINETE_JFONT_H_INCLUDED

#include "jinete/jbase.h"

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
