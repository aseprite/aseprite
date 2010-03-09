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

/* Based on code of AllegroFont, Copyright (c) 2001, 2002 Javier Gonzalez */

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <stdlib.h>
#include <string.h>

#include "jinete/jfont.h"
#include "jinete/jintern.h"
#include "jinete/jtheme.h"

FONT* ji_font_load(const char* filepathname)
{
  FONT* f = ji_font_load_bmp(filepathname);
  if (!f)
    f = ji_font_load_ttf(filepathname);
  return f;
}

FONT* ji_font_load_bmp(const char* filepathname)
{
  int old_color_conv = _color_conv;
  FONT* f = NULL;
  PALETTE junk;
  BITMAP *bmp;

  set_color_conversion(COLORCONV_NONE);
  bmp = load_bitmap(filepathname, junk);
  set_color_conversion(old_color_conv);

  if (bmp) {
    bmp = ji_apply_guiscale(bmp);
    f = _ji_bitmap2font(bmp);
    destroy_bitmap(bmp);
  }

  return f;
}

/**********************************************************************/
#if 1 /* with FreeType */
/**********************************************************************/

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

typedef struct CACHED_GLYPH {
  bool is_cached : 1;
  bool mono_available : 1;
  bool aa_available : 1;
  int width, height, aawidth, aaheight;
  int left, top, aaleft, aatop;
  int advancex, advancey;
  unsigned char* bmp;
  unsigned char* aabmp;
} CACHED_GLYPH;

typedef struct FONT_AA_DATA {
  int aa_mode;
  FT_Face face;           /* face */
  int face_h;             /* face height */
  int real_face_h;        /* real face height */
  int face_ascender;      /* face ascender */
  char* data;             /* if loaded from memory, the data chunk */
  int data_size;          /* and its size */
  int ch_spacing;         /* extra spacing */
  int num_fixed_sizes;    /* -1 if scalable, >=0 if fixed */
  CACHED_GLYPH* cached_glyphs; /* array to know which glyphs have been cached */
  int* fixed_sizes;       /* array with the fixed sizes */
} FONT_AA_DATA;

static FT_Library ft_library;
static bool ji_font_inited = false;

extern FONT_VTABLE _ji_font_vtable_aa;
extern FONT_VTABLE* ji_font_vtable_aa;

/* helpers */

static void _font_reget_fixed_sizes(FONT* f)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  if (af->num_fixed_sizes < 0) {
    /* scalable font */
    af->fixed_sizes[0] = -1;
  }
  else {
    /* fixed */
    int i;

    for (i=0; i<af->num_fixed_sizes; i++)
      af->fixed_sizes[i] = af->face->available_sizes[i].height;

    /* set last one to -1 */
    af->fixed_sizes[af->num_fixed_sizes] = -1;
  }
}

static void _font_uncache_glyphs(FONT* f)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  if (af->cached_glyphs) {
    int i;

    for (i=0; i<af->face->num_glyphs; i++) {
      if (af->cached_glyphs[i].is_cached) {
        af->cached_glyphs[i].is_cached = false;
        if (af->cached_glyphs[i].bmp) {
          jfree(af->cached_glyphs[i].bmp);
          af->cached_glyphs[i].bmp = NULL;
        }
        if (af->cached_glyphs[i].aabmp) {
          jfree(af->cached_glyphs[i].aabmp);
          af->cached_glyphs[i].aabmp = NULL;
        }
      }
    }
  }
}

static void _font_delete_glyphs(FONT* f)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  _font_uncache_glyphs(f);

  if (af->cached_glyphs) {
    jfree(af->cached_glyphs);
    af->cached_glyphs = NULL;
  }
}

static void _font_cache_glyph(AL_CONST FONT* f, int glyph_number)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  /* if glyph not cached yet */
  if (!af->cached_glyphs[glyph_number].is_cached) {
    FT_Glyph new_glyph;
    /* load the font glyph */
    FT_Load_Glyph(af->face, glyph_number, FT_LOAD_DEFAULT);
    FT_Get_Glyph(af->face->glyph, &new_glyph);

    /* ok, this glyph is now cached */
    af->cached_glyphs[glyph_number].is_cached = true;
    af->cached_glyphs[glyph_number].mono_available = false;
    af->cached_glyphs[glyph_number].aa_available = false;

    /* render the mono bmp */
    {
      FT_Bitmap *ft_bmp;
      FT_Glyph glyph;
      FT_BitmapGlyph bmp_glyph;

      FT_Glyph_Copy(new_glyph, &glyph);

      /* only render glyph if it is not already a bitmap */
      if (glyph->format != ft_glyph_format_bitmap)
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_mono, NULL, 1);

      /* the FT rendered bitmap */
      bmp_glyph = (FT_BitmapGlyph)glyph;
      ft_bmp = &bmp_glyph->bitmap;

      /* save only if the bitmap is really 1 bit */
      if (ft_bmp->pixel_mode == ft_pixel_mode_mono) {

        af->cached_glyphs[glyph_number].mono_available = true;

        /* set width, height, left, top */
        af->cached_glyphs[glyph_number].width = ft_bmp->width;
        af->cached_glyphs[glyph_number].height = ft_bmp->rows;
        af->cached_glyphs[glyph_number].left = bmp_glyph->left;
        af->cached_glyphs[glyph_number].top = bmp_glyph->top;

        /* allocate bitmap */
        af->cached_glyphs[glyph_number].bmp =
	  jnew(unsigned char, ft_bmp->width * ft_bmp->rows);

        /* monochrome drawing */
        {
          unsigned char *outbmp_p = af->cached_glyphs[glyph_number].bmp;
          unsigned char *bmp_p;
          int bmp_x, bmp_y, bit;

          /* copy the FT character bitmap to ours */
          bmp_p = ft_bmp->buffer;
          for (bmp_y = 0; bmp_y < ft_bmp->rows; bmp_y++) {
            unsigned char *next_bmp_p;
            next_bmp_p = bmp_p + ft_bmp->pitch;
            bit = 7;
            for (bmp_x = 0; bmp_x < ft_bmp->width; bmp_x++) {
              *outbmp_p = *bmp_p & (1 << bit);
              outbmp_p++;

              if (bit == 0) {
                bit = 7;
                bmp_p++;
              }
              else
                bit--;
            }
            bmp_p = next_bmp_p;
          }
        }
      }

      FT_Done_Glyph(glyph);
    }


    /* render the aa bmp */
    {
      FT_Bitmap *ft_bmp;
      FT_Glyph glyph;
      FT_BitmapGlyph bmp_glyph;
    
      FT_Glyph_Copy(new_glyph, &glyph);

      /* only render glyph if it is not already a bitmap */
      if (glyph->format != ft_glyph_format_bitmap)
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, NULL, 1);

      /* the FT rendered bitmap */
      bmp_glyph = (FT_BitmapGlyph)glyph;
      ft_bmp = &bmp_glyph->bitmap;

      /* save only if the bitmap is really 8 bit */
      if (ft_bmp->pixel_mode == ft_pixel_mode_grays) {

        af->cached_glyphs[glyph_number].aa_available = true;

        /* set width, height, left, top */
        af->cached_glyphs[glyph_number].aawidth = ft_bmp->width;
        af->cached_glyphs[glyph_number].aaheight = ft_bmp->rows;
        af->cached_glyphs[glyph_number].aaleft = bmp_glyph->left;
        af->cached_glyphs[glyph_number].aatop = bmp_glyph->top;

        /* allocate bitmap */
        af->cached_glyphs[glyph_number].aabmp =
	  jnew(unsigned char, ft_bmp->width * ft_bmp->rows);

        /* aa drawing */
        {
          unsigned char *outbmp_p = af->cached_glyphs[glyph_number].aabmp;
          unsigned char *bmp_p;
          int bmp_y;
          unsigned char mul = 256 / ft_bmp->num_grays;
          /* we set it to 0 because it is faster to test for false */
          if (mul == 1)
            mul = 0;

          /* copy the FT character bitmap to ours */
          bmp_p = ft_bmp->buffer;
          for (bmp_y = 0; bmp_y < ft_bmp->rows; bmp_y++) {
            unsigned char *next_bmp_p;
            next_bmp_p = bmp_p + ft_bmp->pitch;
            memcpy(outbmp_p, bmp_p, ft_bmp->width * sizeof(unsigned char));

            /* we have to change our pixels if the numgrays is not 256 */
            if (mul) {
              unsigned char *p = outbmp_p;
              unsigned char *p_end = p + ft_bmp->width;
              for (; p < p_end; p++)
                *p *= mul;
            }

            outbmp_p += ft_bmp->width;
            bmp_p = next_bmp_p;
          }
        }
      }

      FT_Done_Glyph(glyph);
    }

    af->cached_glyphs[glyph_number].advancex = af->face->glyph->advance.x >> 6;
    af->cached_glyphs[glyph_number].advancey = af->face->glyph->advance.y >> 6;

    /* delete the glyph */
    FT_Done_Glyph(new_glyph);
  }
}

static void _font_new_cache_glyph(FONT* f)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
  int i;

  if (!af->cached_glyphs)
    af->cached_glyphs = jnew(struct CACHED_GLYPH, af->face->num_glyphs);

  for (i=0; i<af->face->num_glyphs; i++) {
    af->cached_glyphs[i].is_cached = false;
    af->cached_glyphs[i].bmp = NULL;
    af->cached_glyphs[i].aabmp = 0;
  }
}

/* API */

int _ji_font_init()
{
  if (!ji_font_inited) {
    if (FT_Init_FreeType (&ft_library))
      return -1;

    ji_font_inited = true;
  }
  return 0;
}

void _ji_font_exit()
{
  if (ji_font_inited) {
    FT_Done_FreeType(ft_library);

    ji_font_inited = false;
  }
}

FONT* ji_font_load_ttf(const char *filepathname)
{
  FONT_AA_DATA* af;
  int error;
  FONT* f;

  /* try to allocate the memory */
  f = (FONT*)jmalloc(sizeof(FONT));
  if (!f)
    return NULL;

  f->data = af = (FONT_AA_DATA*)jmalloc0(sizeof(FONT_AA_DATA));
  if (!f->data) {
    jfree(f);
    return NULL;
  }

  f->height = 0;
  f->vtable = ji_font_vtable_aa;

  /* clear the struct */
  af->cached_glyphs = NULL;

  /* we are loading from file, no mem buffer needed */
  af->data = NULL;
  af->data_size = 0; 

  /* load the font */
  error = FT_New_Face(ft_library, filepathname, 0, &af->face);
  if (error) {
    jfree(af);
    jfree(f);
    return NULL;
  }

  /* get if the font contains only fixed sizes */
  if (!(af->face->face_flags & FT_FACE_FLAG_SCALABLE))
    af->num_fixed_sizes = af->face->num_fixed_sizes;
  else
    af->num_fixed_sizes = -1;

  _font_new_cache_glyph(f);

  if (af->num_fixed_sizes < 0) {
    af->fixed_sizes = (int*)jmalloc(sizeof(int));
    _font_reget_fixed_sizes(f);

    ji_font_set_size(f, 8);
  }
  else {
    af->fixed_sizes = jnew(int, af->num_fixed_sizes + 1);
    _font_reget_fixed_sizes(f);

    /* set as current size the first found fixed size */
    ji_font_set_size(f, af->fixed_sizes[0]);
  }

  ji_font_set_char_extra_spacing(f, 0);
  return f;
}

int ji_font_get_size(FONT* f)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
    return af->face_h;
  }
  else
    return text_height(f);
}

int ji_font_set_size(FONT* f, int h)
{
  int error, test_h, direction;
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  if (f->vtable != ji_font_vtable_aa)
    return -1;

  /* check the font doesn't already use that w and h */
  if (h == af->face_h)
    return 0;
  else if (h <= 0)
    return -1;

  /* keep changing the size until the real size is not the one */
  /* we want */
  test_h = h;
  direction = 0;
  while (1) {
    int real_height;
    error = FT_Set_Pixel_Sizes(af->face, 0, test_h);
    if (error)
      break;

    /* compare real height with asked height */
    real_height = abs(af->face->size->metrics.ascender >> 6)
                + abs(af->face->size->metrics.descender >> 6);
    if (real_height == h) {
      /* we found the wanted height */
      break;
    }

    /* check the direction */
    if (direction == 0) {
      /* direction still not set */
      if (real_height > h)
        direction = -1;
      else
        direction = 1;
    }
    
    /* check we didn't overpass it */
    else if ((direction > 0) && (real_height > h)) {
      /* decrease one and found */
      test_h--;
      FT_Set_Pixel_Sizes(af->face, 0, test_h);
      break;
    }

    /* check we didn't surpass it */
    else if ((direction < 0) && (real_height < h)) {
      break;
    }

    test_h += direction;

    /* check we arent at 0 */
    if (test_h <= 0) {
      error = true;
      break;
    }

  }

  if (!error) {
    _font_uncache_glyphs(f);
    af->face_h = h;
    af->real_face_h = test_h;
    af->face_ascender = af->face->size->metrics.ascender >> 6;
    return 0;
  }
  else {
    FT_Set_Pixel_Sizes(af->face, 0, af->real_face_h);
    return -1;
  }
}

int ji_font_get_aa_mode(struct FONT* f)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
    return af->aa_mode;
  }
  else
    return -1;
}

int ji_font_set_aa_mode(struct FONT* f, int mode)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
    int old_mode = af->aa_mode;
    af->aa_mode = mode;
    return old_mode;
  }
  else
    return -1;
}

bool ji_font_is_fixed(FONT* f)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

    if (af->num_fixed_sizes < 0)
      return false;
    else
      return true;
  }
  else
    return true;
}

bool ji_font_is_scalable(FONT* f)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

    if (af->num_fixed_sizes < 0)
      return true;
    else
      return false;
  }
  else
    return false;
}

const int *ji_font_get_available_fixed_sizes(FONT* f, int *n)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

    /* we reget them just to make sure the contents where not changed */
    _font_reget_fixed_sizes(f);

    if (n)
      *n = af->num_fixed_sizes;

    return af->fixed_sizes;
  }
  else
    return NULL;		/* TODO return text_height(...) size */
}

int ji_font_get_char_extra_spacing(FONT* f)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

    return af->ch_spacing;
  }
  else
    return 0;
}

void ji_font_set_char_extra_spacing(FONT* f, int spacing)
{
  if (f->vtable == ji_font_vtable_aa) {
    FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

    af->ch_spacing = MID(0, spacing, 4096);
  }
}

int ji_font_char_len(FONT* f, int chr)
{
  return f->vtable->char_length(f, chr);
}

// see jdraw_text
int ji_font_text_len(struct FONT* f, const char *s)
{
  int in_pos = 0;
  int pix_len = 0;
  int c;

  while ((c = ugetc(s+in_pos)) != 0) {
    if (c == '&') {
      in_pos += uwidth(s+in_pos);
      c = ugetc(s+in_pos);
      if (c == '&') {
	pix_len += f->vtable->char_length(f, '&');
	in_pos += uwidth(s+in_pos);
      }
    }
    else {
      pix_len += f->vtable->char_length(f, c);
      in_pos += uwidth(s+in_pos);
    }
  }

  return pix_len;
}

/* _aa_find_glyph:
 *  Helper for aa vtable entries, below.
 */
static CACHED_GLYPH *_aa_find_glyph(AL_CONST FONT* f, int ch)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
  int glyph_index;

  /* get the character out of the font */
  if (af->face->charmap)
    glyph_index = FT_Get_Char_Index(af->face, ch);
  else
    glyph_index = ch;
    
  /* apply kerning */
  /*if (last_glyph_index) {
    FT_Vector v;
    FT_Get_Kerning(af->face, last_glyph_index, glyph_index, ft_kerning_default, &v);
    total_length += v.x >> 6;
    }*/

  /* cache */
  _font_cache_glyph(f, glyph_index);

  return af->cached_glyphs + glyph_index;
}

/* font_height:
 *  Returns the height, in pixels of the font.
 */
static int aa_font_height(AL_CONST FONT* f)
{
  /* TODO */
/*   return f->height; */
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  return af->face_h;
}

/* aa_char_length:
 *  Returns the length of a character, in pixels, as it would be rendered
 *  in this font.
 */
static int aa_char_length(AL_CONST FONT* f, int ch)
{
  CACHED_GLYPH *g = _aa_find_glyph(f, ch);

/*   return g->advancex ? g->advancex + af->ch_spacing: 0; */
  return g->advancex;
}

/* aa_length:
 *  Returns the length, in pixels, of a string as rendered in a font.
 */
static int aa_length(AL_CONST FONT* f, AL_CONST char *text)
{
  AL_CONST FONT_AA_DATA* af = reinterpret_cast<AL_CONST FONT_AA_DATA*>(f->data);
  AL_CONST char *p = text;
  int ch = 0, w = 0;

  while ((ch = ugetxc(&p))) {
    w += f->vtable->char_length(f, ch)
      + af->ch_spacing;
  }

  return w;
}

static void _aa_prepare_alpha_table(AL_CONST FONT* f, int color,
				    int *alpha_table, int depth)
{
  AL_CONST FONT_AA_DATA* af = reinterpret_cast<AL_CONST FONT_AA_DATA*>(f->data);
  int i, r, g, b, br, bg, bb, ir, ig, ib;
  int blendr, blendg, blendb;

  /* if we are gonna use transparent mode, set blender */
  if (af->aa_mode < 0)
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
  /* else if we are doing opaque mode, draw a rect and init our table*/
  else {
    /* get the color rgb */
    r = getr_depth(depth, color);
    g = getg_depth(depth, color);
    b = getb_depth(depth, color);

    /* get the background rgb */   
    br = getr_depth(depth, af->aa_mode);
    bg = getg_depth(depth, af->aa_mode);
    bb = getb_depth(depth, af->aa_mode);

    /* get increments */
    if (r == br)
      ir = 0;
    else
      ir = (r + 1) - br;

    if (g == bg)
      ig = 0;
    else
      ig = (g + 1) - bg;

    if (b == bb)
      ib = 0;
    else
      ib = (b + 1) - bb;

    blendr = br << 8;
    blendg = bg << 8;
    blendb = bb << 8;
    
    /* blend both values and make our alpha table */
    for (i=0; i<256; i++) {
      if (i == 0)
	alpha_table[i] = -1;
      else
	alpha_table[i] = makecol_depth(depth,
				       blendr >> 8, blendg >> 8, blendb >> 8);

      blendr += ir;
      blendg += ig;
      blendb += ib;
    }
  }
}

static void _aa_render_glyph(BITMAP *bmp, FONT_AA_DATA* af, CACHED_GLYPH *g,
			     int u, int v, int color, int *alpha_table)
{
  int max_bmp_x, max_bmp_y;
  unsigned char *bmp_p;
  int bmp_x, bmp_y;
  int c;

  /* anti-aliased drawing */
  if (g->aa_available) {
    bmp_p = g->aabmp;

    u = (u + g->aaleft);
    v = (v - g->aatop) + af->face_ascender;

    max_bmp_x = g->aawidth + u;
    max_bmp_y = g->aaheight + v;

    /* if in opaque mode */
    if (af->aa_mode >= 0) {
      /* copy the character bitmap to the dest one */
      for (bmp_y=v; bmp_y<max_bmp_y; bmp_y++) {
	for (bmp_x=u; bmp_x<max_bmp_x; bmp_x++) {
	  c = alpha_table[*bmp_p];
	  if (c >= 0)
	    putpixel(bmp, bmp_x, bmp_y, c);
	  bmp_p++;
	}
      }
    }
    /* if in transparent mode */
    else {
      /* copy the FT character bitmap to our dest one */
      for (bmp_y=v; bmp_y<max_bmp_y; bmp_y++) {
	for (bmp_x=u; bmp_x<max_bmp_x; bmp_x++) {
	  set_trans_blender(0, 0, 0, *bmp_p);
	  putpixel(bmp, bmp_x, bmp_y, color);
	  bmp_p++;
	}
      }
    }
  }
  /* monochrome drawing */
  else if (g->mono_available) {
    bmp_p = g->bmp;

    u = (u + g->left);
    v = (v - g->top) + af->face_ascender;

    max_bmp_x = g->width + u;
    max_bmp_y = g->height + v;

    /* copy the character bitmap to our allegro one */
    for (bmp_y=u; bmp_y<max_bmp_y; bmp_y++) {
      for (bmp_x=v; bmp_x<max_bmp_x; bmp_x++) {
	if (*bmp_p)
	  putpixel(bmp, bmp_x, bmp_y, color);
	/*             else if (af->aa_mode >= 0) */
	/*               putpixel(bmp, bmp_x, bmp_y, af->aa_mode); */
	bmp_p++;
      }
    }
  }
}

/* aa_render_char:
 *  Renders a color character onto a bitmap, at the specified location, using
 *  the specified colors. If fg == -1, render as color, else render as
 *  mono; if bg == -1, render as transparent, else render as opaque.
 *  Returns the character width, in pixels.
 */
static int aa_render_char(AL_CONST FONT* f, int ch, int fg, int bg, BITMAP *bmp, int x, int y)
{
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
  CACHED_GLYPH *g = NULL;
  int w = 0;

  acquire_bitmap(bmp);

  if (bg >= 0) {
    rectfill(bmp, x, y,
	     x + f->vtable->char_length(f, ch) - 1,
	     y + f->vtable->font_height(f) - 1, bg);
  }

  g = _aa_find_glyph(f, ch);
  if (g) {
    int alpha_table[256];

    _aa_prepare_alpha_table(f, fg, alpha_table, bitmap_color_depth(bmp));
    _aa_render_glyph(bmp, af, g, x, y, fg, alpha_table);

    w += g->advancex;

    /* reset blender */
    if (af->aa_mode < 0)
      solid_mode();
  }

  release_bitmap(bmp);

  return w;
}

/* aa_render:
 *  Renders a color font onto a bitmap, at the specified location, using
 *  the specified colors. If fg == -1, render as color, else render as
 *  mono; if bg == -1, render as transparent, else render as opaque.
 */
static void aa_render(AL_CONST FONT* f, AL_CONST char *text, int fg, int bg, BITMAP *bmp, int x, int y)
{
  AL_CONST char *p = text;
  FONT_AA_DATA* af = reinterpret_cast<FONT_AA_DATA*>(f->data);
  CACHED_GLYPH *g;
  int alpha_table[256];
  int ch = 0;

  acquire_bitmap(bmp);

  if (bg >= 0) {
    rectfill(bmp, x, y,
	     x + text_length(f, text) - 1,
	     y + text_height(f) - 1, bg);
    bg = -1; /* to avoid filling rectangles for each character */
  }

  _aa_prepare_alpha_table(f, fg, alpha_table, bitmap_color_depth(bmp));

  while ((ch = ugetxc (&p))) {
    g = _aa_find_glyph(f, ch);
    if (g) {
      _aa_render_glyph(bmp, af, g, x, y, fg, alpha_table);

      if (g->advancex) x += g->advancex + af->ch_spacing;
      if (g->advancey) y += g->advancey + af->ch_spacing;
    }
  }

  /* reset blender */
  if (af->aa_mode < 0)
    solid_mode();

  release_bitmap(bmp);
}

static void aa_destroy(FONT* f)
{
  FONT_AA_DATA* af;

  if (!f) return;
  af = reinterpret_cast<FONT_AA_DATA*>(f->data);

  /* delete old glyphs */
  _font_delete_glyphs(f);

  /* delete the face */
  FT_Done_Face(af->face);

  if (af->fixed_sizes)
    jfree(af->fixed_sizes);

  /* deallocate the data */
  if (af->data)
    jfree(af->data);

  jfree(af);
  jfree(f);
}

/********
 * vtable declarations
 ********/

FONT_VTABLE _ji_font_vtable_aa =
{
  aa_font_height,
  aa_char_length,
  aa_length,
  aa_render_char,
  aa_render,
  aa_destroy
};

FONT_VTABLE *ji_font_vtable_aa = &_ji_font_vtable_aa;

/**********************************************************************/
#else /* without FreeType */
/**********************************************************************/

int _ji_font_init()
{
  return 0;
}

void _ji_font_exit()
{
}

FONT* ji_font_load_ttf(const char* filepathname)
{
  return NULL;
}

int ji_font_get_size(FONT* f)
{
  return text_height(f);
}

int ji_font_set_size(FONT* f, int h)
{
  /* do nothing */
  return -1;
}

int ji_font_get_aa_mode(struct FONT* f)
{
  return -1;
}

int ji_font_set_aa_mode(struct FONT* f, int mode)
{
  return -1;
}

bool ji_font_is_fixed(FONT* f)
{
  return true;
}

bool ji_font_is_scalable(FONT* f)
{
  return false;
}

const int *ji_font_get_available_fixed_sizes(FONT* f, int* n)
{
  return NULL; /* TODO return text_height(...) size */
}

int ji_font_get_char_extra_spacing(FONT* f)
{
  return 0;
}

void ji_font_set_char_extra_spacing(FONT* f, int spacing)
{
  /* do nothing */
}

int ji_font_char_len(FONT* f, int chr)
{
  return f->vtable->char_length(f, chr);
}

// see jdraw_text
int ji_font_text_len(struct FONT* f, const char* s)
{
  int in_pos = 0;
  int pix_len = 0;
  int c;

  while ((c = ugetc(s+in_pos)) != 0) {
    if (c == '&') {
      in_pos += uwidth(s+in_pos);
      c = ugetc(s+in_pos);
      if (c == '&') {
	pix_len += f->vtable->char_length(f, '&');
	in_pos += uwidth(s+in_pos);
      }
    }
    else {
      pix_len += f->vtable->char_length(f, c);
      in_pos += uwidth(s+in_pos);
    }
  }

  return pix_len;
}

#endif
