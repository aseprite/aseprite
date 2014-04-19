// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <vector>

#include "ui/draw.h"
#include "ui/font.h"
#include "ui/intern.h"
#include "ui/system.h"
#include "ui/widget.h"

/* TODO optional anti-aliased textout */
#define SETUP_ANTIALISING(f, bg, fill_bg)                               \
  ji_font_set_aa_mode(f, fill_bg ||                                     \
                         bitmap_color_depth(ji_screen) == 8 ? to_system(bg): -1)

namespace ui {

using namespace gfx;

void _draw_text(BITMAP* bmp, FONT* font, const char *s, int x, int y,
  ui::Color fg_color, ui::Color bg_color, bool fill_bg, int underline_height)
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

  textout_ex(bmp, font, tmp, x, y, to_system(fg_color), (fill_bg ? to_system(bg_color): -1));

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
             y+text_height(font)+underline_height-1, to_system(fg_color));
  }
}

void _move_region(const Region& region, int dx, int dy)
{
  size_t nrects = region.size();

  // Blit directly screen to screen.
  if (is_linear_bitmap(ji_screen) && nrects == 1) {
    Rect rc = region[0];
    blit(ji_screen, ji_screen, rc.x, rc.y, rc.x+dx, rc.y+dy, rc.w, rc.h);
  }
  // Blit saving areas and copy them.
  else if (nrects > 1) {
    std::vector<BITMAP*> images(nrects);
    Region::const_iterator it, begin=region.begin(), end=region.end();
    BITMAP* bmp;
    int c;

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      bmp = create_bitmap(rc.w, rc.h);
      blit(ji_screen, bmp, rc.x, rc.y, 0, 0, bmp->w, bmp->h);
      images[c] = bmp;
    }

    for (c=0, it=begin; it != end; ++it, ++c) {
      const Rect& rc = *it;
      bmp = images[c];
      blit(bmp, ji_screen, 0, 0, rc.x+dx, rc.y+dy, bmp->w, bmp->h);
      destroy_bitmap(bmp);
    }
  }
}

} // namespace ui
