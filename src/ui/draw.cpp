// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <vector>

#include "ui/draw.h"
#include "ui/font.h"
#include "ui/intern.h"
#include "ui/rect.h"
#include "ui/region.h"
#include "ui/system.h"
#include "ui/widget.h"

using namespace gfx;

/* TODO optional anti-aliased textout */
#define SETUP_ANTIALISING(f, bg, fill_bg)                               \
  ji_font_set_aa_mode(f, fill_bg ||                                     \
                         bitmap_color_depth(ji_screen) == 8 ? bg: -1)

namespace ui {

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

  textout_ex(bmp, font, tmp, x, y, fg_color, (fill_bg ? bg_color: -1));

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

void ji_move_region(JRegion region, int dx, int dy)
{
  int c, nrects = JI_REGION_NUM_RECTS(region);
  JRect rc;

  // Blit directly screen to screen.
  if (is_linear_bitmap(ji_screen) && nrects == 1) {
    rc = JI_REGION_RECTS(region);
    blit(ji_screen, ji_screen,
         rc->x1, rc->y1,
         rc->x1+dx, rc->y1+dy, jrect_w(rc), jrect_h(rc));
  }
  // Blit saving areas and copy them.
  else if (nrects > 1) {
    std::vector<BITMAP*> images(nrects);
    BITMAP* bmp;

    for (c=0, rc=JI_REGION_RECTS(region); c<nrects; ++c, ++rc) {
      bmp = create_bitmap(jrect_w(rc), jrect_h(rc));
      blit(ji_screen, bmp,
           rc->x1, rc->y1, 0, 0, bmp->w, bmp->h);
      images[c] = bmp;
    }

    for (c=0, rc=JI_REGION_RECTS(region); c<nrects; ++c, ++rc) {
      bmp = images[c];
      blit(bmp, ji_screen, 0, 0, rc->x1+dx, rc->y1+dy, bmp->w, bmp->h);
      destroy_bitmap(bmp);
    }
  }
}

} // namespace ui
