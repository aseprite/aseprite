/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MODULES_GFX_H_INCLUDED
#define MODULES_GFX_H_INCLUDED

#include "app/color.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/color.h"

struct FONT;
struct BITMAP;

typedef struct RectTracker RectTracker;

void dotted_mode(int offset);

RectTracker* rect_tracker_new(BITMAP* bmp, int x1, int y1, int x2, int y2);
void rect_tracker_free(RectTracker* tracker);

void bevel_box(BITMAP* bmp, int x1, int y1, int x2, int y2, ui::Color c1, ui::Color c2, int bevel);
void rectgrid(BITMAP* bmp, int x1, int y1, int x2, int y2, int w, int h);

void draw_emptyset_symbol(BITMAP* bmp, const gfx::Rect& rc, ui::Color color);
void draw_color(BITMAP* bmp, const gfx::Rect& rc, PixelFormat pixelFormat, const app::Color& color);
void draw_color_button(BITMAP* bmp,
                       const gfx::Rect& rc,
                       bool outer_nw, bool outer_n, bool outer_ne, bool outer_e,
                       bool outer_se, bool outer_s, bool outer_sw, bool outer_w,
                       PixelFormat pixelFormat, const app::Color& color,
                       bool hot, bool drag);
void draw_progress_bar(BITMAP* bmp,
                       int x1, int y1, int x2, int y2,
                       float progress);

int character_length(FONT* font, int chr);
void render_character(BITMAP* bmp, FONT* font, int chr, int x, int y, int fg, int bg);

#endif
