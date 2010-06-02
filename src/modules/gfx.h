/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "core/color.h"
#include "jinete/jbase.h"

namespace Vaca { class Rect; }
using Vaca::Rect;

struct FONT;
struct BITMAP;

#define COLOR_SHADE(color, r, g, b)		\
  makecol(MID(getr(color)+r, 0, 255),		\
	  MID(getg(color)+g, 0, 255),		\
	  MID(getb(color)+b, 0, 255))

#define COLOR_INTERP(color1, color2, step, max)			\
  makecol(getr(color1)+(getr(color2)-getr(color1))*step/max,	\
	  getg(color1)+(getg(color2)-getg(color1))*step/max,	\
	  getb(color1)+(getb(color2)-getb(color1))*step/max)

/* graphics ids */
enum {
  GFX_LINKFRAME,

  GFX_ANI_FIRST,
  GFX_ANI_PREV,
  GFX_ANI_PLAY,
  GFX_ANI_NEXT,
  GFX_ANI_LAST,

  GFX_TOOL_MARKER,
  GFX_TOOL_PENCIL,
  GFX_TOOL_BRUSH,
  GFX_TOOL_ERASER,
  GFX_TOOL_FLOODFILL,
  GFX_TOOL_SPRAY,
  GFX_TOOL_LINE,
  GFX_TOOL_CURVE,
  GFX_TOOL_RECTANGLE,
  GFX_TOOL_ELLIPSE,
  GFX_TOOL_BLUR,
  GFX_TOOL_JUMBLE,
  GFX_TOOL_CONFIGURATION,

  GFX_TARGET_ONE,
  GFX_TARGET_FRAMES,
  GFX_TARGET_LAYERS,
  GFX_TARGET_FRAMES_LAYERS,

  GFX_FILE_HOME,
  GFX_FILE_FONTS,
  GFX_FILE_PALETTES,
  GFX_FILE_MKDIR,

  GFX_BRUSH_CIRCLE,
  GFX_BRUSH_SQUARE,
  GFX_BRUSH_LINE,

  GFX_SCALE_1,
  GFX_SCALE_2,
  GFX_SCALE_3,

  GFX_ROTATE_1,
  GFX_ROTATE_2,
  GFX_ROTATE_3,

  GFX_ARROW_LEFT,
  GFX_ARROW_RIGHT,
  GFX_ARROW_UP,
  GFX_ARROW_DOWN,

  GFX_BOX_SHOW,
  GFX_BOX_HIDE,
  GFX_BOX_LOCK,
  GFX_BOX_UNLOCK,

  GFX_BITMAP_COUNT,
};

typedef struct RectTracker RectTracker;

int init_module_graphics();
void exit_module_graphics();

BITMAP* get_gfx(int id);

void dotted_mode(int offset);
void simple_dotted_mode(BITMAP* bmp, int fg, int bg);

RectTracker *rect_tracker_new(BITMAP* bmp, int x1, int y1, int x2, int y2);
void rect_tracker_free(RectTracker *tracker);

void bevel_box(BITMAP* bmp, int x1, int y1, int x2, int y2, int c1, int c2, int bevel);
void rectdotted(BITMAP* bmp, int x1, int y1, int x2, int y2, int fg, int bg);
void rectgrid(BITMAP* bmp, int x1, int y1, int x2, int y2, int w, int h);

void draw_emptyset_symbol(BITMAP* bmp, const Rect& rc, int color);
void draw_color(BITMAP* bmp, const Rect& rc, int imgtype, color_t color);
void draw_color_button(BITMAP* bmp,
		       const Rect& rc,
		       bool outer_nw, bool outer_n, bool outer_ne, bool outer_e,
		       bool outer_se, bool outer_s, bool outer_sw, bool outer_w,
		       int imgtype, color_t color,
		       bool hot, bool drag);
void draw_progress_bar(BITMAP* bmp,
		       int x1, int y1, int x2, int y2,
		       float progress);

int character_length(FONT* font, int chr);
void render_character(BITMAP* bmp, FONT* font, int chr, int x, int y, int fg, int bg);

#endif
