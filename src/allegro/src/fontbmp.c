/*	 ______   ___    ___ 
 *	/\  _  \ /\_ \  /\_ \ 
 *	\ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *	 \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *	  \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *	   \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *	    \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *					   /\____/
 *					   \_/__/
 *
 *      Read font from a bitmap.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

/* state information for the bitmap font importer */
static int import_x = 0;
static int import_y = 0;



/* splits bitmaps into sub-sprites, using regions bounded by col #255 */
static void font_find_character(BITMAP *bmp, int *x, int *y, int *w, int *h)
{
   int c;

   if (_bitmap_has_alpha(bmp)) {
      c = getpixel(bmp, 0, 0);
   }
   else if (bitmap_color_depth(bmp) == 8) {
      c = 255;
   }
   else {
      c = makecol_depth(bitmap_color_depth(bmp), 255, 255, 0);
   }

   /* look for top left corner of character */
   while ((getpixel(bmp, *x, *y) != c) || 
	  (getpixel(bmp, *x+1, *y) != c) ||
	  (getpixel(bmp, *x, *y+1) != c) ||
	  (getpixel(bmp, *x+1, *y+1) == c)) {
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
   while ((getpixel(bmp, *x+*w+1, *y) == c) &&
	  (getpixel(bmp, *x+*w+1, *y+1) != c) &&
	  (*x+*w+1 <= bmp->w))
      (*w)++;

   /* look for bottom edge of character */
   *h = 0;
   while ((getpixel(bmp, *x, *y+*h+1) == c) &&
	  (getpixel(bmp, *x+1, *y+*h+1) != c) &&
	  (*y+*h+1 <= bmp->h))
      (*h)++;
}



/* import_bitmap_font_mono:
 *  Helper for import_bitmap_font, below.
 */
static int import_bitmap_font_mono(BITMAP *import_bmp, FONT_GLYPH** gl, int num)
{
   int w = 1, h = 1, i;

   for(i = 0; i < num; i++) {
      if(w > 0 && h > 0) font_find_character(import_bmp, &import_x, &import_y, &w, &h);
      if(w <= 0 || h <= 0) {
	 int j;

	 gl[i] = _AL_MALLOC(sizeof(FONT_GLYPH) + 8);
	 gl[i]->w = 8;
	 gl[i]->h = 8;

	 for(j = 0; j < 8; j++) gl[i]->dat[j] = 0;
      }
      else {
	 int sx = ((w + 7) / 8), j, k;

	 gl[i] = _AL_MALLOC(sizeof(FONT_GLYPH) + sx * h);
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
static int import_bitmap_font_color(BITMAP *import_bmp, BITMAP** bits, int num)
{
   int w = 1, h = 1, i;

   for(i = 0; i < num; i++) {
      if(w > 0 && h > 0) font_find_character(import_bmp, &import_x, &import_y, &w, &h);
      if(w <= 0 || h <= 0) {
	 bits[i] = create_bitmap_ex(bitmap_color_depth(import_bmp), 8, 8);
	 if(!bits[i]) return -1;
	 clear_to_color(bits[i], 255);
      }
      else {
	 bits[i] = create_bitmap_ex(bitmap_color_depth(import_bmp), w, h);
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

   while (1) {
      font_find_character(bmp, &x, &y, &w, &h);
      if (w <= 0 || h <= 0)
         break;
      num++;
      x += w;
   }

   return num;
}



/* import routine for the Allegro .pcx font format */
FONT *load_bitmap_font(AL_CONST char *fname, RGB *pal, void *param)
{
   /* NB: `end' is -1 if we want every glyph */
   int color_conv_mode;
   BITMAP *import_bmp;
   FONT *f;
   ASSERT(fname);

   /* Don't change the colourdepth of the bitmap if it is 8 bit */
   color_conv_mode = get_color_conversion();
   set_color_conversion(COLORCONV_MOST | COLORCONV_KEEP_TRANS);
   import_bmp = load_bitmap(fname, pal);
   set_color_conversion(color_conv_mode);

   if(!import_bmp) 
     return NULL;

   f = grab_font_from_bitmap(import_bmp);

   destroy_bitmap(import_bmp);

   return f;
}



/* work horse for grabbing a font from an Allegro bitmap */
FONT *grab_font_from_bitmap(BITMAP *bmp)
{
   int begin = ' ';
   int end = -1;
   FONT *f;
   ASSERT(bmp)

   import_x = 0;
   import_y = 0;

   f = _AL_MALLOC(sizeof *f);
   if (end == -1) end = bitmap_font_count(bmp) + begin;

   if (bitmap_font_ismono(bmp)) {
      FONT_MONO_DATA* mf = _AL_MALLOC(sizeof(FONT_MONO_DATA));

      mf->glyphs = _AL_MALLOC(sizeof(FONT_GLYPH*) * (end - begin));

      if ( import_bitmap_font_mono(bmp, mf->glyphs, end - begin) ) {
	 _AL_FREE(mf->glyphs);
	 _AL_FREE(mf);
	 _AL_FREE(f);
	 f = NULL;
      }
      else {
	 f->data = mf;
	 f->vtable = font_vtable_mono;
	 f->height = mf->glyphs[0]->h;

	 mf->begin = begin;
	 mf->end = end;
	 mf->next = NULL;
      }
   }
   else {
      FONT_COLOR_DATA* cf = _AL_MALLOC(sizeof(FONT_COLOR_DATA));
      cf->bitmaps = _AL_MALLOC(sizeof(BITMAP*) * (end - begin));

      if( import_bitmap_font_color(bmp, cf->bitmaps, end - begin) ) {
	 _AL_FREE(cf->bitmaps);
	 _AL_FREE(cf);
	 _AL_FREE(f);
	 f = 0;
      }
      else {
	 f->data = cf;
	 f->vtable = font_vtable_color;
	 f->height = cf->bitmaps[0]->h;

	 cf->begin = begin;
	 cf->end = end;
	 cf->next = 0;
      }
   }
   
   return f;
}
