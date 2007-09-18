/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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
#include <string.h>

#include "jinete/list.h"

#include "modules/palette.h"
#include "raster/blend.h"
#include "raster/sprite.h"
#include "util/col_file.h"

#endif

PALETTE current_palette;	/* current original palette (you can
				   use _current_palette from Allegro
				   to refer to the current system
				   "fake-palette") */
RGB_MAP *orig_rgb_map = NULL;	/* color map for the original palette
				   (not for the system palette) */
COLOR_MAP *orig_trans_map = NULL;

/* current rgb map */
static RGB_MAP *my_rgb_map = NULL;

/* hook's information (for the `ase_set_palette' routine) */
static JList hooks;

/* system color (in order of importance) */
static RGB system_color[] =
{
  { 63, 63, 63, 0 }, {  0,  0,  0, 0 }, { 32, 32, 32, 0 }, { 48, 48, 48, 0 },
  { 11, 19, 36, 0 }, { 56, 56, 56, 0 }, { 63, 63,  0, 0 }, {  0,  0, 63, 0 }
};

#define SYSTEM_COLORS   (sizeof(system_color) / sizeof(RGB))

int init_module_palette(void)
{
  orig_rgb_map = jnew (RGB_MAP, 1);
  orig_trans_map = jnew (COLOR_MAP, 1);
  my_rgb_map = jnew (RGB_MAP, 1);

  hooks = jlist_new();
  rgb_map = my_rgb_map;
  color_map = NULL;
  palette_copy(current_palette, black_palette);

  return 0;
}

void exit_module_palette(void)
{
  rgb_map = NULL;

  jlist_free(hooks);
  jfree(my_rgb_map);
  jfree(orig_trans_map);
  jfree(orig_rgb_map);
}

void set_default_palette(RGB *palette)
{
  palette_copy(default_palette, palette);
}

/* changes the current system palette, all hooks will be called, if
   "palette" is NULL will be used the default one (ase_palette) */
bool set_current_palette(RGB *_palette, int forced)
{
  PALETTE palette;
  bool ret = FALSE;

  /* if "palette" == NULL we must use "default_palette" */
  if (!_palette)
    palette_copy(palette, default_palette);
  else
    palette_copy(palette, _palette);

  /* have changes */
  if (forced || palette_diff(palette, current_palette, NULL, NULL)) {
    int i, j, k;

    /* copy current palette */
    palette_copy(current_palette, palette);

    /* create a RGB map for the original palette */
    create_rgb_table(orig_rgb_map, palette, NULL);

    /* create a transparency-map with the original palette */
    rgb_map = orig_rgb_map;
    create_trans_table(orig_trans_map, palette, 128, 128, 128, NULL);
    rgb_map = my_rgb_map;

    /* make the fake-palette and fixup the _index_cmap[]... */

    /* fill maps with default values */
    for (i=0; i<256; i++)
      _index_cmap[i] = i;

    /* fake palette system is only for 8 bpp screen */
    if (screen && bitmap_color_depth(screen) == 8) {
      char found[SYSTEM_COLORS];

      for (i=0; i<SYSTEM_COLORS; i++) {
	found[i] = FALSE;

	for (j=1; j<256; j++) {
	  /* color found */
	  if ((system_color[i].r == palette[j].r) &&
	      (system_color[i].g == palette[j].g) &&
	      (system_color[i].b == palette[j].b)) {
	    found[i] = TRUE;
	    break;
	  }
	}
      }

      for (i=0; i<SYSTEM_COLORS; i++) {
	if (found[i])
	  continue;

	for (j=255; j>1; j--) {
	  for (k=1; k<j; k++) {
	    if ((palette[j].r == palette[k].r) &&
		(palette[j].g == palette[k].g) &&
		(palette[j].b == palette[k].b)) {
	      _index_cmap[j] = k;
	      palette[j].r = system_color[i].r;
	      palette[j].g = system_color[i].g;
	      palette[j].b = system_color[i].b;
	      j = 0;
	      break;
	    }
	  }
	}
      }
    }

    /* create a map for the fake-palette */
    create_rgb_table(my_rgb_map, palette, NULL);

    /* change system color palette */
    set_palette(palette);

    /* call hooks */
    call_palette_hooks();

    ret = TRUE;
  }

  return ret;
}

/* changes a color of the current system palette */
void set_current_color (int index, int r, int g, int b)
{
  if (index >= 0 && index <= 255 &&
      (current_palette[index].r != r ||
       current_palette[index].g != g ||
       current_palette[index].b != b)) {
    RGB rgb = { r, g, b, 0 };

    current_palette[index] = rgb;

    if ((screen) && (bitmap_color_depth (screen) == 8) &&
	(_index_cmap[index] != index))
      _index_cmap[index] = index;

    set_color (index, &rgb);
  }
}

/* inserts a new palette-hook in the end of the list */
void hook_palette_changes (void (*proc)(void))
{
  jlist_append(hooks, proc);
}

/* removes a palette-hook */
void unhook_palette_changes (void (*proc)(void))
{
  jlist_remove(hooks, proc);
}

/* calls all hooks */
void call_palette_hooks (void)
{
  JLink link;

  /* call the hooks */
  JI_LIST_FOR_EACH(hooks, link)
    (*((void (*)())link->data))();
}

/**********************************************************************/

static int regen_rgb_map = FALSE;

void use_current_sprite_rgb_map (void)
{
  rgb_map = orig_rgb_map;
}

void use_sprite_rgb_map(Sprite *sprite)
{
  rgb_map = my_rgb_map;

  regen_rgb_map = TRUE;
  create_rgb_table(my_rgb_map,
		   sprite_get_palette(sprite, sprite->frpos), NULL);
}

void restore_rgb_map (void)
{
  rgb_map = my_rgb_map;

  if (regen_rgb_map) {
    regen_rgb_map = FALSE;
    create_rgb_table (my_rgb_map, _current_palette, NULL);
  }
}

/**********************************************************************/

/* creates a linear ramp in the palette */
void make_palette_ramp (RGB *p, int from, int to)
{
  float rv, gv, bv;
  float rd, gd, bd;
  int i;

  if (from > to) {
    i    = from;
    from = to;
    to   = i;
  }

  if ((to - from) < 2)
    return;

  /* deltas */
  rd = (float)(p[to].r - p[from].r) / (float)(to - from);
  gd = (float)(p[to].g - p[from].g) / (float)(to - from);
  bd = (float)(p[to].b - p[from].b) / (float)(to - from);

  /* values */
  rv = p[from].r;
  gv = p[from].g;
  bv = p[from].b;

  for (i=from+1; i<to; i++) {
    rv += rd;
    gv += gd;
    bv += bd;

    p[i].r = rv;
    p[i].g = gv;
    p[i].b = bv;
  }
}

/* creates a rectangular ramp in the palette */
void make_palette_rect_ramp (RGB *p, int from, int to, int cols)
{
  float rv1, gv1, bv1, rv2, gv2, bv2;
  float rd1, gd1, bd1, rd2, gd2, bd2;
  int imin = MIN (from, to);
  int imax = MAX (from, to);
  int xmin = imin % cols;
  int ymin = imin / cols;
  int xmax = imax % cols;
  int ymax = imax / cols;
  int x1 = MIN (xmin, xmax);
  int y1 = MIN (ymin, ymax);
  int x2 = MAX (xmin, xmax);
  int y2 = MAX (ymin, ymax);
  int i, rows;

  rows = y2 - y1;

  /* left side */
  from = y1*cols+x1;
  to   = y2*cols+x1;

  /* deltas */
  rd1 = (float)(p[to].r - p[from].r) / (float)rows;
  gd1 = (float)(p[to].g - p[from].g) / (float)rows;
  bd1 = (float)(p[to].b - p[from].b) / (float)rows;

  /* values */
  rv1 = p[from].r;
  gv1 = p[from].g;
  bv1 = p[from].b;

  /* right side */
  from = y1*cols+x2;
  to   = y2*cols+x2;

  /* deltas */
  rd2 = (float)(p[to].r - p[from].r) / (float)rows;
  gd2 = (float)(p[to].g - p[from].g) / (float)rows;
  bd2 = (float)(p[to].b - p[from].b) / (float)rows;

  /* values */
  rv2 = p[from].r;
  gv2 = p[from].g;
  bv2 = p[from].b;

  for (i=y1; i<=y2; i++) {
    /* left side */
    p[i*cols+x1].r = rv1;
    p[i*cols+x1].g = gv1;
    p[i*cols+x1].b = bv1;

    /* right side */
    p[i*cols+x2].r = rv2;
    p[i*cols+x2].g = gv2;
    p[i*cols+x2].b = bv2;

    /* ramp from left to right side */
    make_palette_ramp (p, i*cols+x1, i*cols+x2);

    rv1 += rd1;
    gv1 += gd1;
    bv1 += bd1;

    rv2 += rd2;
    gv2 += gd2;
    bv2 += bd2;
  }
}

/* counts differences between palettes */
int palette_diff(RGB *p1, RGB *p2, int *from, int *to)
{
  register int c, diff = 0;

  if (from) *from = -1;
  if (to) *to = -1;

  /* compare palettes */
  for (c=0; c<256; c++) {
    if ((p1[c].r != p2[c].r) ||
	(p1[c].g != p2[c].g) ||
	(p1[c].b != p2[c].b)) {
      if (from && *from < 0) *from = c;
      if (to) *to = c;
      diff++;
    }
  }

  return diff;
}

void palette_copy(RGB *dst, RGB *src)
{
  memcpy(dst, src, sizeof (PALETTE));
}

RGB *palette_load (const char *filename)
{
  RGB *palette = NULL;
  char ext[64];

  ustrcpy (ext, get_extension (filename));

  if ((ustricmp (ext, "pcx") == 0) ||
      (ustricmp (ext, "bmp") == 0) ||
      (ustricmp (ext, "tga") == 0) ||
      (ustricmp (ext, "lbm") == 0)) {
    BITMAP *bmp;

    palette = jmalloc (sizeof (PALETTE));
    bmp = load_bitmap (filename, palette);
    if (bmp)
      destroy_bitmap (bmp);
    else {
      jfree (palette);
      palette = NULL;
    }
  }
  else if (ustricmp (ext, "col") == 0) {
    palette = load_col_file (filename);
  }

  return palette;
}

int palette_save (RGB *palette, const char *filename)
{
  int ret = -1;
  char ext[64];

  ustrcpy (ext, get_extension (filename));

  if ((ustricmp (ext, "pcx") == 0) ||
      (ustricmp (ext, "bmp") == 0) ||
      (ustricmp (ext, "tga") == 0)) {
    int c, x, y;
    BITMAP *bmp;

    bmp = create_bitmap_ex (8, 16, 16);
    for (y=c=0; y<16; y++)
      for (x=0; x<16; x++)
	putpixel (bmp, x, y, c++);

    ret = save_bitmap (filename, bmp, palette);
    destroy_bitmap (bmp);
  }
  else if (ustricmp (ext, "col") == 0) {
    ret = save_col_file (palette, filename);
  }

  return ret;
}
