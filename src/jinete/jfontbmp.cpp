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

/* Original code from:
   allegro/tools/datedit.c
   allegro/tools/plugins/datfont.c
*/

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

/* state information for the bitmap font importer */
static BITMAP *import_bmp = NULL;

static int import_x = 0;
static int import_y = 0;

/* splits bitmaps into sub-sprites, using regions bounded by col #255 */
static void datedit_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h)
{
  int c1;
  int c2;

  if (bitmap_color_depth(bmp) == 8) {
    c1 = 255;
    c2 = 255;
  }
  else {
    c1 = makecol_depth(bitmap_color_depth(bmp), 255, 255, 0);
    c2 = makecol_depth(bitmap_color_depth(bmp), 0, 255, 255);
  }

  /* look for top left corner of character */
  while ((getpixel(bmp, *x, *y) != c1) || 
	 (getpixel(bmp, *x+1, *y) != c2) ||
	 (getpixel(bmp, *x, *y+1) != c2) ||
	 (getpixel(bmp, *x+1, *y+1) == c1) ||
	 (getpixel(bmp, *x+1, *y+1) == c2)) {
    (*x)++;
    if (*x >= bmp->w) {
      *x = 0;
      (*y)++;
      if (*y >= bmp->h) {
	*w = 0;
	*h = 0;
	return;
      }
    }
  }

  /* look for right edge of character */
  *w = 0;
  while ((getpixel(bmp, *x+*w+1, *y) == c2) &&
	 (getpixel(bmp, *x+*w+1, *y+1) != c2) &&
	 (*x+*w+1 <= bmp->w))
    (*w)++;

  /* look for bottom edge of character */
  *h = 0;
  while ((getpixel(bmp, *x, *y+*h+1) == c2) &&
	 (getpixel(bmp, *x+1, *y+*h+1) != c2) &&
	 (*y+*h+1 <= bmp->h))
    (*h)++;
}

/* import_bitmap_font_mono:
 *  Helper for import_bitmap_font, below.
 */
static int import_bitmap_font_mono(FONT_GLYPH** gl, int num)
{
  int w = 1, h = 1, i;

  for(i = 0; i < num; i++) {
    if(w > 0 && h > 0) datedit_find_character(import_bmp, &import_x, &import_y, &w, &h);
    if(w <= 0 || h <= 0) {
      int j;

      gl[i] = (FONT_GLYPH*)_al_malloc(sizeof(FONT_GLYPH) + 8);
      gl[i]->w = 8;
      gl[i]->h = 8;

      for(j = 0; j < 8; j++) gl[i]->dat[j] = 0;
    } else {
      int sx = ((w + 7) / 8), j, k;

      gl[i] = (FONT_GLYPH*)_al_malloc(sizeof(FONT_GLYPH) + sx * h);
      gl[i]->w = w;
      gl[i]->h = h;

      for(j = 0; j < sx * h; j++) gl[i]->dat[j] = 0;
      for(j = 0; j < h; j++) {
	for(k = 0; k < w; k++) {
	  if(getpixel(import_bmp, import_x + k + 1, import_y + j + 1))
	    gl[i]->dat[(j * sx) + (k / 8)] |= 0x80 >> (k & 7);
	}
      }

      import_x += w;
    }
  }

  return 0;
}

/* import_bitmap_font_color:
 *  Helper for import_bitmap_font, below.
 */
static int import_bitmap_font_color(BITMAP** bits, int num)
{
  int w = 1, h = 1, i;

  for(i = 0; i < num; i++) {
    if(w > 0 && h > 0) datedit_find_character(import_bmp, &import_x, &import_y, &w, &h);
    if(w <= 0 || h <= 0) {
      bits[i] = create_bitmap_ex(8, 8, 8);
      if(!bits[i]) return -1;
      clear_to_color(bits[i], 255);
    } else {
      bits[i] = create_bitmap_ex(8, w, h);
      if(!bits[i]) return -1;
      blit(import_bmp, bits[i], import_x + 1, import_y + 1, 0, 0, w, h);
      import_x += w;
    }
  }

  return 0;
}

/* bitmap_font_ismono:
 *  Helper for import_bitmap_font, below.
 */
static int bitmap_font_ismono(BITMAP *bmp)
{
  int x, y, col = -1, pixel;

  for(y = 0; y < bmp->h; y++) {
    for(x = 0; x < bmp->w; x++) {
      pixel = getpixel(bmp, x, y);
      if(pixel == 0 || pixel == 255) continue;
      if(col > 0 && pixel != col) return 0;
      col = pixel;
    }
  }

  return 1;
}

/* bitmap_font_count:
 *  Helper for `import_bitmap_font', below.
 */
static int bitmap_font_count(BITMAP* bmp)
{
  int x = 0, y = 0, w = 0, h = 0;
  int num = 0;

  while(1) {
    datedit_find_character(bmp, &x, &y, &w, &h);
    if (w <= 0 || h <= 0) break;
    num++;
    x += w;
  }

  return num;
}

FONT *_ji_bitmap2font(BITMAP *bmp)
{
  FONT *f;
  int begin = ' ';
  int end = -1;

  import_bmp = bmp;
  import_x = 0;
  import_y = 0;

  if(bitmap_color_depth(import_bmp) != 8) {
    import_bmp = NULL;
    return 0;
  }

  f = (FONT*)_al_malloc(sizeof(FONT));
  if(end == -1) end = bitmap_font_count(import_bmp) + begin;

  if (bitmap_font_ismono(import_bmp)) {

    FONT_MONO_DATA* mf = (FONT_MONO_DATA*)_al_malloc(sizeof(FONT_MONO_DATA));

    mf->glyphs = (FONT_GLYPH**)_al_malloc(sizeof(FONT_GLYPH*) * (end - begin));

    if( import_bitmap_font_mono(mf->glyphs, end - begin) ) {

      free(mf->glyphs);
      free(mf);
      free(f);
      f = 0;

    } else {

      f->data = mf;
      f->vtable = font_vtable_mono;
      f->height = mf->glyphs[0]->h;

      mf->begin = begin;
      mf->end = end;
      mf->next = 0;
    }

  } else {

    FONT_COLOR_DATA* cf = (FONT_COLOR_DATA*)_al_malloc(sizeof(FONT_COLOR_DATA));
    cf->bitmaps = (BITMAP**)_al_malloc(sizeof(BITMAP*) * (end - begin));

    if( import_bitmap_font_color(cf->bitmaps, end - begin) ) {

      free(cf->bitmaps);
      free(cf);
      free(f);
      f = 0;

    } else {

      f->data = cf;
      f->vtable = font_vtable_color;
      f->height = cf->bitmaps[0]->h;

      cf->begin = begin;
      cf->end = end;
      cf->next = 0;

    }

  }

  import_bmp = NULL;

  return f;
}
