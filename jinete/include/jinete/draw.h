/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_DRAW_H
#define JINETE_DRAW_H

#include "jinete/base.h"

JI_BEGIN_DECLS

#define JI_COLOR_SHADE(color, r, g, b)		\
  makecol(MID (0, getr(color)+(r), 255),	\
	  MID (0, getg(color)+(g), 255),	\
	  MID (0, getb(color)+(b), 255))

#define JI_COLOR_INTERP(c1, c2, step, max)		\
  makecol(getr(c1)+(getr(c2)-getr(c1)) * step / max,	\
	  getg(c1)+(getg(c2)-getg(c1)) * step / max,	\
	  getb(c1)+(getb(c2)-getb(c1)) * step / max)

struct FONT;

void jdraw_rect(const JRect rect, int color);
void jdraw_rectfill(const JRect rect, int color);
void jdraw_rectedge(const JRect rect, int c1, int c2);
void jdraw_rectshade(const JRect rect, int c1, int c2, int align);
void jdraw_rectexclude(const JRect rc, const JRect exclude, int color);

void jdraw_char(struct FONT *f, int chr, int x, int y, int fg, int bg, bool fill_bg);
void jdraw_text(struct FONT *f, const char *text, int x, int y, int fg, int bg, bool fill_bg);
void jdraw_widget_text(JWidget widget, int fg, int bg, bool fill_bg);

void ji_blit_region(JRegion region, int dx, int dy);

JI_END_DECLS

#endif /* JINETE_DRAW_H */
