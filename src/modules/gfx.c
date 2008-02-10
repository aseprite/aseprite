/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/jintern.h"
#include "jinete/jsystem.h"

#include "console/console.h"
#include "core/cfg.h"
#include "core/dirs.h"
#include "modules/gfx.h"
#include "modules/palette.h"
#include "modules/tools.h"
#include "widgets/editor.h"

#endif

/* static BITMAP *icons_pcx; */
static BITMAP *gfx_bmps[GFX_BITMAP_COUNT];

#include "modules/gfxdata.c"

static void convert_data_to_bitmap(DATA *data, BITMAP **bmp)
{
  const char *p;
  int x, y;
  int black = makecol(0, 0, 0);
  int gray = makecol(128, 128, 128);
  int white = makecol(255, 255, 255);
  int mask;

  *bmp = create_bitmap(data->w, data->h);
  mask = bitmap_mask_color(*bmp);

  p = data->line;
  for (y=0; y<data->h; y++)
    for (x=0; x<data->w; x++) {
      putpixel(*bmp, x, y,
	       (*p == '#') ? black:
	       (*p == '%') ? gray:
	       (*p == '.') ? white: mask);
      p++;
    }
}

static void gen_gfx(void)
{
  int c;

  for(c=0; c<GFX_BITMAP_COUNT; c++) {
    if(gfx_bmps[c])
      destroy_bitmap(gfx_bmps[c]);

    gfx_bmps[c] = NULL;
  }
}

int init_module_graphics(void)
{
/*   DIRS *dirs, *dir; */
/*   PALETTE pal; */
  int c;

/*   dirs = filename_in_datadir("icons.pcx"); */
/*   for (dir=dirs; dir; dir=dir->next) { */
/*     set_color_conversion(COLORCONV_NONE); */
/*     icons_pcx = load_bitmap(dir->path, pal); */
/*     set_color_conversion(COLORCONV_TOTAL); */

/*     if (icons_pcx) */
/*       break; */
/*   } */
/*   dirs_free(dirs); */

/*   if (!icons_pcx) { */
/*     user_printf("Error loading icons.pcx"); */
/*     return -1; */
/*   } */
  
  for(c=0; c<GFX_BITMAP_COUNT; c++)
    gfx_bmps[c] = NULL;

  hook_palette_changes(gen_gfx);
  return 0;
}

void exit_module_graphics(void)
{
  int c;

  unhook_palette_changes(gen_gfx);

  for(c=0; c<GFX_BITMAP_COUNT; c++)
    if(gfx_bmps[c]) {
      destroy_bitmap(gfx_bmps[c]);
      gfx_bmps[c] = NULL;
    }

/*   if (icons_pcx) */
/*     destroy_bitmap(icons_pcx); */
}

BITMAP *get_gfx(int id)
{
  if (!gfx_bmps[id]) {
/*     if (id == GFX_TOOL_MARKER) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*0, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_DOTS) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*1, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_PENCIL) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*2, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_BRUSH) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*3, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_FLOODFILL) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*4, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_SPRAY) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*5, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_LINE) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*6, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_RECTANGLE) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*7, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_ELLIPSE) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*8, 0, 0, 9, 9); */
/*     } */
/*     else if (id == GFX_TOOL_CONFIGURATION) { */
/*       gfx_bmps[id] = create_bitmap(9, 9); */
/*       blit(icons_pcx, gfx_bmps[id], 9*1, 9*9, 0, 0, 9, 9); */
/*     } */
/*     else { */
      convert_data_to_bitmap(&gfx_data[id], &gfx_bmps[id]);
/*     } */
  }

  return gfx_bmps[id];
}

void dotted_mode(int offset)
{
  /* these pattern were taken from The GIMP:
     gimp-1.3.11/app/display/gimpdisplayshell-marching-ants.h */
  static int pattern_data[8][8] = {
    {
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
    },
    {
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
    },
    {
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
    },
    {
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
    },
    {
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
    },
    {
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
    },
    {
      0x3C,    /*  --####--  */
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
    },
    {
      0x78,    /*  -####---  */
      0xF0,    /*  ####----  */
      0xE1,    /*  ###----#  */
      0xC3,    /*  ##----##  */
      0x87,    /*  #----###  */
      0x0F,    /*  ----####  */
      0x1E,    /*  ---####-  */
      0x3C,    /*  --####--  */
    },
  };

  static BITMAP *pattern = NULL;
  int x, y, fg, bg;

  if (offset < 0) {
    if (pattern) {
      destroy_bitmap(pattern);
      pattern = NULL;
    }
    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
    return;
  }

  if (!pattern)
    pattern = create_bitmap(8, 8);

  offset = 7 - (offset & 7);

  bg = makecol(0, 0, 0);
  fg = makecol(255, 255, 255);

  clear_bitmap(pattern);

  for (y=0; y<8; y++)
    for (x=0; x<8; x++)
      putpixel(pattern, x, y,
	       (pattern_data[offset][y] & (1<<(7-x)))? fg: bg);

  drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
}

void simple_dotted_mode(BITMAP *bmp, int fg, int bg)
{
  static BITMAP *pattern = NULL;

  if (pattern && bitmap_color_depth(pattern) != bitmap_color_depth(bmp))
    destroy_bitmap(pattern);

  pattern = create_bitmap_ex(bitmap_color_depth(bmp), 2, 2);
  clear_bitmap(pattern);

  putpixel(pattern, 0, 0, fg);
  putpixel(pattern, 0, 1, bg);
  putpixel(pattern, 1, 0, bg);
  putpixel(pattern, 1, 1, fg);

  drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
}


/**********************************************************************/
/* Set/Restore sub-clip regions */

typedef struct CLIP_DATA
{
  BITMAP *bmp;
  int cl, ct, cr, cb;
} CLIP_DATA;

void *subclip(BITMAP *bmp, int x1, int y1, int x2, int y2)
{
  int cl, ct, cr, cb;
  CLIP_DATA *data;

  cl = bmp->cl;
  ct = bmp->ct;
  cr = bmp->cr;
  cb = bmp->cb;

  if ((x2 < cl) || (x1 >= cr) || (y2 < ct) || (y1 >= cb))
    return NULL;

  x1 = MID(cl, x1, cr-1);
  y1 = MID(ct, y1, cb-1);
  x2 = MID(x1, x2, cr-1);
  y2 = MID(y1, y2, cb-1);

  set_clip(bmp, x1, y1, x2, y2);

  data = jnew(CLIP_DATA, 1);
  data->bmp = bmp;
  data->cl = cl;
  data->ct = ct;
  data->cr = cr;
  data->cb = cb;

  return data;
}

void backclip(void *_data)
{
  CLIP_DATA *data = _data;
  set_clip(data->bmp, data->cl, data->ct, data->cr, data->cb);
  jfree(data);
}


/**********************************************************************/
/* Save/Restore rectangles */

typedef struct RECT_DATA
{
  BITMAP *bmp;
  int x1, y1, x2, y2;
  int npixel;
  int *pixel;
} RECT_DATA;

static void do_rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int c,
		    void (*proc)(BITMAP *bmp, int x, int y, int c))
{
  int x, y, u1, u2, v1, v2;

  if ((x2 < bmp->cl) || (x1 >= bmp->cr) ||
      (y2 < bmp->ct) || (y1 >= bmp->cb))
    return;

  u1 = MID(bmp->cl, x1, bmp->cr-1);
  u2 = MID(x1, x2, bmp->cr-1);

  if ((y1 >= bmp->ct) && (y1 < bmp->cb)) {
    for (x=u1; x<=u2; x++)
      (*proc) (bmp, x, y1, c);
  }

  if ((y2 >= bmp->ct) && (y2 < bmp->cb)) {
    for (x=u1; x<=u2; x++)
      (*proc) (bmp, x, y2, c);
  }

  v1 = MID(bmp->ct, y1+1, bmp->cb-1);
  v2 = MID(v1, y2-1, bmp->cb-1);

  if ((x1 >= bmp->cl) && (x1 < bmp->cr)) {
    for (y=v1; y<=v2; y++)
      (*proc)(bmp, x1, y, c);
  }

  if ((x2 >= bmp->cl) && (x2 < bmp->cr)) {
    for (y=v1; y<=v2; y++)
      (*proc)(bmp, x2, y, c);
  }
}

static void count_rect(BITMAP *bmp, int x, int y, int c)
{
  RECT_DATA *data = (RECT_DATA *)c;
  data->npixel++;
}

static void save_rect(BITMAP *bmp, int x, int y, int c)
{
  RECT_DATA *data = (RECT_DATA *)c;
  data->pixel[data->npixel++] = getpixel(bmp, x, y);
}

static void restore_rect(BITMAP *bmp, int x, int y, int c)
{
  RECT_DATA *data = (RECT_DATA *)c;
  putpixel(bmp, x, y, data->pixel[data->npixel++]);
}

void *rectsave(BITMAP *bmp, int x1, int y1, int x2, int y2)
{
  RECT_DATA *data;
  int x, y;

  if (x1 > x2) { x = x1; x1 = x2; x2 = x; }
  if (y1 > y2) { y = y1; y1 = y2; y2 = y; }

  data = jnew(RECT_DATA, 1);

  data->bmp = bmp;
  data->x1 = x1;
  data->y1 = y1;
  data->x2 = x2;
  data->y2 = y2;

  data->npixel = 0;
  do_rect(bmp, x1, y1, x2, y2, (int)data, count_rect);

  data->pixel = jmalloc(sizeof(int) * data->npixel);

  data->npixel = 0;
  do_rect(bmp, x1, y1, x2, y2, (int)data, save_rect);

  return data;
}

void rectrestore(void *_data)
{
  RECT_DATA *data = _data;

  data->npixel = 0;
  do_rect(data->bmp, data->x1, data->y1, data->x2, data->y2,
	  (int)data, restore_rect);
}

void rectdiscard(void *_data)
{
  RECT_DATA *data = _data;

  if (data->pixel)
    jfree(data->pixel);
  jfree(data);
}



/**********************************************************************/
/* Rectangles */

void bevel_box(BITMAP *bmp, int x1, int y1, int x2, int y2, int c1, int c2, int bevel)
{
  hline(bmp, x1+bevel, y1, x2-bevel, c1); /* top */
  hline(bmp, x1+bevel, y2, x2-bevel, c2); /* bottom */

  vline(bmp, x1, y1+bevel, y2-bevel, c1); /* left */
  vline(bmp, x2, y1+bevel, y2-bevel, c2); /* right */

  line(bmp, x1, y1+bevel, x1+bevel, y1, c1); /* top-left */
  line(bmp, x1, y2-bevel, x1+bevel, y2, c2); /* bottom-left */

  line(bmp, x2-bevel, y1, x2, y1+bevel, c2); /* top-right */
  line(bmp, x2-bevel, y2, x2, y2-bevel, c2); /* bottom-right */
}

void rectdotted(BITMAP *bmp, int x1, int y1, int x2, int y2, int fg, int bg)
{
  simple_dotted_mode(bmp, fg, bg);
  rect(bmp, x1, y1, x2, y2, 0);
  solid_mode();
}

void rectgrid(BITMAP *bmp, int x1, int y1, int x2, int y2, int w, int h)
{
  int x, y, u, v, c1, c2;

  c1 = makecol_depth(bitmap_color_depth(bmp), 128, 128, 128);
  c2 = makecol_depth(bitmap_color_depth(bmp), 192, 192, 192);

  u = 0;
  v = 0;
  for (y=y1; y<=y2-h; y+=h) {
    for (x=x1; x<=x2-w; x+=w)
      rectfill(bmp, x, y, x+w-1, y+h-1, ((u++)&1)? c1: c2);

    if (x <= x2)
      rectfill(bmp, x, y, x2, y+h-1, ((u++)&1)? c1: c2);

    u = (++v);
  }

  if (y <= y2) {
    for (x=x1; x<=x2-w; x+=w)
      rectfill(bmp, x, y, x+w-1, y2, ((u++)&1)? c1: c2);

    if (x <= x2)
      rectfill(bmp, x, y, x2, y2, ((u++)&1)? c1: c2);
  }
}

void rectfill_exclude(BITMAP *bmp, int x1, int y1, int x2, int y2, int ex1, int ey1, int ex2, int ey2, int color)
{
  _ji_theme_rectfill_exclude(bmp, x1, y1, x2, y2,
			     ex1, ey1, ex2, ey2, color);
}

void rectshade(BITMAP *bmp, int x1, int y1, int x2, int y2, int top, int bottom)
{
  int x, y, r[3], g[3], b[3];

  r[0] = getr(top);
  g[0] = getg(top);
  b[0] = getb(top);

  r[2] = getr(bottom);
  g[2] = getg(bottom);
  b[2] = getb(bottom);

  if ((x2-x1+1) > (y2-y1+1)/2) {
    for (y=y1; y<=y2; y++) {
      r[1] = r[0] + (r[2] - r[0]) * (y - y1) / (y2 - y1);
      g[1] = g[0] + (g[2] - g[0]) * (y - y1) / (y2 - y1);
      b[1] = b[0] + (b[2] - b[0]) * (y - y1) / (y2 - y1);

      hline(bmp, x1, y, x2, makecol(r[1], g[1], b[1]));
    }
  }
  else {
    for (x=x1; x<=x2; x++) {
      r[1] = r[0] + (r[2] - r[0]) * (x - x1) / (x2 - x1);
      g[1] = g[0] + (g[2] - g[0]) * (x - x1) / (x2 - x1);
      b[1] = b[0] + (b[2] - b[0]) * (x - x1) / (x2 - x1);

      vline(bmp, x, y1, y2, makecol(r[1], g[1], b[1]));
    }
  }
}



/************************************************************************/
/* Font related */

int character_length(FONT *font, int chr)
{
  return font->vtable->char_length(font, chr);
}

/* renders a character of the font centered in x and y */
void render_character(BITMAP *bmp, FONT *font, int chr, int x, int y, int fg, int bg)
{
  font->vtable->render_char(font, chr, fg, bg, bmp, x, y);
}
