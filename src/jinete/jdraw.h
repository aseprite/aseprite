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

#ifndef JINETE_JDRAW_H_INCLUDED
#define JINETE_JDRAW_H_INCLUDED

#include "jinete/jbase.h"

namespace Vaca { class Rect; }
using Vaca::Rect;

#define JI_COLOR_SHADE(color, r, g, b)		\
  makecol(MID(0, getr(color)+(r), 255),		\
	  MID(0, getg(color)+(g), 255),		\
	  MID(0, getb(color)+(b), 255))

#define JI_COLOR_INTERP(c1, c2, step, max)		\
  makecol(getr(c1)+(getr(c2)-getr(c1)) * step / max,	\
	  getg(c1)+(getg(c2)-getg(c1)) * step / max,	\
	  getb(c1)+(getb(c2)-getb(c1)) * step / max)

struct FONT;
struct BITMAP;

void jrectedge(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
	       int c1, int c2);
void jrectexclude(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
		  int ex1, int ey1, int ex2, int ey2, int color);
void jrectshade(struct BITMAP *bmp, int x1, int y1, int x2, int y2,
		int c1, int c2, int align);

void jdraw_rect(const JRect rect, int color);
void jdraw_rectfill(const JRect rect, int color);
void jdraw_rectfill(const Rect& rect, int color);
void jdraw_rectedge(const JRect rect, int c1, int c2);
void jdraw_rectshade(const JRect rect, int c1, int c2, int align);
void jdraw_rectexclude(const JRect rc, const JRect exclude, int color);

void jdraw_char(FONT* f, int chr, int x, int y, int fg, int bg, bool fill_bg);
void jdraw_text(BITMAP* bmp, FONT* f, const char* text, int x, int y, int fg, int bg, bool fill_bg, int underline_height = 1);

void jdraw_inverted_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);

void ji_move_region(JRegion region, int dx, int dy);

#endif
