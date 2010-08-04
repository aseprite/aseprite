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

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/jdraw.h"
#include "jinete/jfont.h"
#include "jinete/jintern.h"
#include "jinete/jlist.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jwidget.h"

/* TODO optional anti-aliased textout */
#define SETUP_ANTIALISING(f, bg, fill_bg)				\
  ji_font_set_aa_mode(f, fill_bg ||					\
			 bitmap_color_depth(ji_screen) == 8 ? bg: -1)

void jrectedge(BITMAP *bmp, int x1, int y1, int x2, int y2,
	       int c1, int c2)
{
  hline(bmp, x1,   y1,   x2-1, c1);
  hline(bmp, x1+1, y2,   x2,   c2);
  vline(bmp, x1,   y1+1, y2,   c1);
  vline(bmp, x2,   y1,   y2-1, c2);
}

void jrectexclude(BITMAP *bmp, int x1, int y1, int x2, int y2,
		  int ex1, int ey1, int ex2, int ey2, int color)
{
  if ((ex1 > x2) || (ex2 < x1) ||
      (ey1 > y2) || (ey2 < y1))
    rectfill(bmp, x1, y1, x2, y2, color);
  else {
    int my1, my2;

    my1 = MAX(y1, ey1);
    my2 = MIN(y2, ey2);

    // top
    if (y1 < ey1)
      rectfill(bmp, x1, y1, x2, ey1-1, color);

    // left
    if (x1 < ex1)
      rectfill(bmp, x1, my1, ex1-1, my2, color);

    // right
    if (x2 > ex2)
      rectfill(bmp, ex2+1, my1, x2, my2, color);

    // bottom
    if (y2 > ey2)
      rectfill(bmp, x1, ey2+1, x2, y2, color);
  }
}

void jrectshade(BITMAP *bmp, int x1, int y1, int x2, int y2,
		int c1, int c2, int align)
{
  register int c;
  int r[2], g[2], b[2];

  r[0] = getr(c1);
  g[0] = getg(c1);
  b[0] = getb(c1);

  r[1] = getr(c2);
  g[1] = getg(c2);
  b[1] = getb(c2);

  if (align & JI_VERTICAL) {
    if (y1 == y2)
      hline(bmp, x1, y1, x2, c1);
    else
      for (c=y1; c<=y2; c++)
	hline(bmp,
	      x1, c, x2,
	      makecol((r[0] + (r[1] - r[0]) * (c - y1) / (y2 - y1)),
		      (g[0] + (g[1] - g[0]) * (c - y1) / (y2 - y1)),
		      (b[0] + (b[1] - b[0]) * (c - y1) / (y2 - y1))));
  }
  else if (align & JI_HORIZONTAL) {
    if (x1 == x2)
      vline(ji_screen, x1, y1, y2, c1);
    else
      for (c=x1; c<=x2; c++)
	vline(ji_screen,
	      c, y1, y2,
	      makecol((r[0] + (r[1] - r[0]) * (c - x1) / (x2 - x1)),
		      (g[0] + (g[1] - g[0]) * (c - x1) / (x2 - x1)),
		      (b[0] + (b[1] - b[0]) * (c - x1) / (x2 - x1))));
  }
}

void jdraw_rect(const JRect r, int color)
{
  rect(ji_screen, r->x1, r->y1, r->x2-1, r->y2-1, color);
}

void jdraw_rectfill(const JRect r, int color)
{
  rectfill(ji_screen, r->x1, r->y1, r->x2-1, r->y2-1, color);
}

void jdraw_rectfill(const Rect& r, int color)
{
  rectfill(ji_screen, r.x, r.y, r.x+r.w-1, r.y+r.h-1, color);
}

void jdraw_rectedge(const JRect r, int c1, int c2)
{
  jrectedge(ji_screen, r->x1, r->y1, r->x2-1, r->y2-1, c1, c2);
}

void jdraw_rectshade(const JRect rect, int c1, int c2, int align)
{
  jrectshade(ji_screen,
	     rect->x1, rect->y1,
	     rect->x2-1, rect->y2-1, c1, c2, align);
}

void jdraw_rectexclude(const JRect rc, const JRect exclude, int color)
{
  jrectexclude(ji_screen,
	       rc->x1, rc->y1,
	       rc->x2-1, rc->y2-1,
	       exclude->x1, exclude->y1,
	       exclude->x2-1, exclude->y2-1, color);
}

void jdraw_char(FONT* f, int chr, int x, int y, int fg, int bg, bool fill_bg)
{
  SETUP_ANTIALISING(f, bg, fill_bg);

  f->vtable->render_char(f, chr, fg, fill_bg ? bg: -1, ji_screen, x, y);
}

void jdraw_text(BITMAP* bmp, FONT* font, const char *s, int x, int y,
		int fg_color, int bg_color, bool fill_bg, int underline_height)
{
  // original code from allegro/src/guiproc.c
  char tmp[1024];
  int hline_pos = -1;
  int len = 0;
  int in_pos = 0;
  int out_pos = 0;
  int c;

  while (((c = ugetc(s+in_pos)) != 0) && (out_pos<(int)(sizeof(tmp)-ucwidth(0)))) {
    if (c == '&') {
      in_pos += uwidth(s+in_pos);
      c = ugetc(s+in_pos);
      if (c == '&') {
	out_pos += usetc(tmp+out_pos, '&');
	in_pos += uwidth(s+in_pos);
	len++;
      }
      else
	hline_pos = len;
    }
    else {
      out_pos += usetc(tmp+out_pos, c);
      in_pos += uwidth(s+in_pos);
      len++;
    }
  }

  usetc(tmp+out_pos, 0);

  SETUP_ANTIALISING(font, bg_color, fill_bg);

  text_mode(fill_bg ? bg_color: -1);
  textout(bmp, font, tmp, x, y, fg_color);

  if (hline_pos >= 0) {
    c = ugetat(tmp, hline_pos);
    usetat(tmp, hline_pos, 0);
    hline_pos = text_length(font, tmp);
    c = usetc(tmp, c);
    usetc(tmp+c, 0);
    c = text_length(font, tmp);

    rectfill(bmp, x+hline_pos,
	     y+text_height(font),
	     x+hline_pos+c-1,
	     y+text_height(font)+underline_height-1, fg_color);
  }
}

void jdraw_inverted_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
  if (bitmap_color_depth(bmp) == 8) {
    draw_character_ex(bmp, sprite, x, y, makecol(255, 255, 255), -1);
  }
  else {
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    set_invert_blender(0, 0, 0, 255);
    draw_lit_sprite(bmp, sprite, x, y, 255);
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
  }
}

void ji_move_region(JRegion region, int dx, int dy)
{
  int c, nrects = JI_REGION_NUM_RECTS(region);
  JRect rc;

  if (ji_dirty_region) {
    jregion_translate(region, dx, dy);
    ji_add_dirty_region(region);
    jregion_translate(region, -dx, -dy);
  }

  /* blit directly screen to screen *************************************/
  if (is_linear_bitmap(ji_screen) && nrects == 1) {
    rc = JI_REGION_RECTS(region);
    blit(ji_screen, ji_screen,
	 rc->x1, rc->y1,
	 rc->x1+dx, rc->y1+dy, jrect_w(rc), jrect_h(rc));
  }
  /* blit saving areas and copy them ************************************/
  else if (nrects > 1) {
    /* TODO optimize this routine, it's really slow */
    JList images = jlist_new();
    BITMAP *bmp;
    JLink link;

    for (c=0, rc=JI_REGION_RECTS(region);
	 c<nrects;
	 c++, rc++) {
      bmp = create_bitmap(jrect_w(rc), jrect_h(rc));
      blit(ji_screen, bmp,
	   rc->x1, rc->y1, 0, 0, bmp->w, bmp->h);
      jlist_append(images, bmp);
    }

    for (c=0, rc=JI_REGION_RECTS(region), link=jlist_first(images);
	 c<nrects;
	 c++, rc++, link=link->next) {
      bmp = reinterpret_cast<BITMAP*>(link->data);
      blit(bmp, ji_screen, 0, 0, rc->x1+dx, rc->y1+dy, bmp->w, bmp->h);
      destroy_bitmap(bmp);
    }

    jlist_free(images);
  }
}

#if 0
typedef struct XPM_COLOR {
  char *chars;
  int color;
} XPM_COLOR;

BITMAP *ji_xpm_to_bitmap(const char *xpm_image, int depth)
{
  int w, h, colors, charsbycolor;
  int *colormap;
  const char *p;
  BITMAP *bmp;

  /* read header */
  p = xpm_image[0];
  w = ustrtol(p,(char *)&p, 10);
  h = ustrtol(p,(char *)&p, 10);
  colors = ustrtol(p,(char *)&p, 10);
  charsbycolor = ustrtol(p,(char *)&p, 10);

  /* error? */
  if (w < 1 || h < 1 || colors < 1 || charsbycolor < 1)
    return NULL;

  /* read color map */
  colormap = jmalloc(sizeof(XPM_COLOR) * colors);
  for (c=0; c<colors; c++) {
    colormap[c].chars = jmalloc(charsbycolor);
    colormap[c].color = makecol
  }

  /* read image */
  for (c=0; c<colors; c++) {
  }

  return bmp;
}
#endif
