/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Datafile reading routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Elias Pschernig made load_datafile_object() handle properties.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



static void unload_midi(MIDI *m);
static void initialise_datafile(DATAFILE *data);



/* hack to let the grabber prevent compiled sprites from being compiled */
int _compile_sprites = TRUE;

/* flag to detect whether compiled datafiles are constructed:
 *  A runtime flag is required because the shared version of the library may
 *  have to deal both with executables that use constructors and with
 *  executables that don't.
 */
static int constructed_datafiles = FALSE;

static void (*datafile_callback)(DATAFILE *) = NULL;



/* load_st_data:
 *  I'm not using this format any more, but files created with the old
 *  version of Allegro might have some bitmaps stored like this. It is 
 *  the 4bpp planar system used by the Atari ST low resolution mode.
 */
static void load_st_data(unsigned char *pos, long size, PACKFILE *f)
{
   int c;
   unsigned int d1, d2, d3, d4;

   size /= 8;           /* number of 4 word planes to read */

   while (size) {
      d1 = pack_mgetw(f);
      d2 = pack_mgetw(f);
      d3 = pack_mgetw(f);
      d4 = pack_mgetw(f);
      for (c=0; c<16; c++) {
	 *(pos++) = ((d1 & 0x8000) >> 15) + ((d2 & 0x8000) >> 14) +
		    ((d3 & 0x8000) >> 13) + ((d4 & 0x8000) >> 12);
	 d1 <<= 1;
	 d2 <<= 1;
	 d3 <<= 1;
	 d4 <<= 1; 
      }
      size--;
   }
}



/* read_block:
 *  Reads a block of size bytes from a file, allocating memory to store it.
 */
static void *read_block(PACKFILE *f, int size, int alloc_size)
{
   void *p;

   p = _AL_MALLOC_ATOMIC(MAX(size, alloc_size));
   if (!p) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   pack_fread(p, size, f);

   if (pack_ferror(f)) {
      _AL_FREE(p);
      return NULL;
   }

   return p;
}



/* read_bitmap:
 *  Reads a bitmap from a file, allocating memory to store it.
 */
static BITMAP *read_bitmap(PACKFILE *f, int bits, int allowconv)
{
   int x, y, w, h, c, r, g, b, a;
   int destbits, rgba;
   unsigned short *p16;
   uint32_t *p32;
   BITMAP *bmp;

   if (bits < 0) {
      bits = -bits;
      rgba = TRUE;
   }
   else
      rgba = FALSE;

   if (allowconv)
      destbits = _color_load_depth(bits, rgba);
   else
      destbits = 8;

   w = pack_mgetw(f);
   h = pack_mgetw(f);

   bmp = create_bitmap_ex(MAX(bits, 8), w, h);
   if (!bmp) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   switch (bits) {

      case 4:
	 /* old format ST data */
	 load_st_data(bmp->dat, w*h/2, f);
	 break;

      case 8:
	 /* 256 color bitmap */
	 pack_fread(bmp->dat, w*h, f);

	 break;

      case 15:
	 /* 15bit hicolor */
	 for (y=0; y<h; y++) {
	    p16 = (uint16_t *)bmp->line[y];

	    for (x=0; x<w; x++) {
	       c = pack_igetw(f);
	       r = _rgb_scale_5[(c >> 11) & 0x1F]; /* stored as 16 bit */
	       g = _rgb_scale_6[(c >> 5) & 0x3F];
	       b = _rgb_scale_5[c & 0x1F];
	       p16[x] = makecol15(r, g, b);
	    }
	 }
	 break;

      case 16:
	 /* 16bit hicolor */
	 for (y=0; y<h; y++) {
	    p16 = (uint16_t *)bmp->line[y];

	    for (x=0; x<w; x++) {
	       c = pack_igetw(f);
	       r = _rgb_scale_5[(c >> 11) & 0x1F];
	       g = _rgb_scale_6[(c >> 5) & 0x3F];
	       b = _rgb_scale_5[c & 0x1F];
	       p16[x] = makecol16(r, g, b);
	    }
	 }
	 break;

      case 24:
	 /* 24bit truecolor */
	 for (y=0; y<h; y++) {
	    for (x=0; x<w; x++) {
	       r = pack_getc(f);
	       g = pack_getc(f);
	       b = pack_getc(f);

	       if (rgba)
		  pack_getc(f);

	       c = makecol24(r, g, b);
	       WRITE3BYTES(bmp->line[y] + (x * 3), c);
	    }
	 }
	 break;

      case 32:
	 /* 32bit rgba */
	 for (y=0; y<h; y++) {
	    p32 = (uint32_t *)bmp->line[y];

	    for (x=0; x<w; x++) {
	       r = pack_getc(f);
	       g = pack_getc(f);
	       b = pack_getc(f);

	       if (rgba)
		  a = pack_getc(f);
	       else
		  a = 0;

	       p32[x] = makeacol32(r, g, b, a);
	    }
	 }
	 break;

   }

   if (bits != destbits) {
      BITMAP *tmp = bmp;
      bmp = create_bitmap_ex(destbits, w, h);
      if (!bmp) {
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      blit(tmp, bmp, 0, 0, 0, 0, bmp->w, bmp->h);
      destroy_bitmap(tmp);
   }

   return bmp;
}



/* read_font_fixed:
 *  Reads a fixed pitch font from a file. This format is no longer used.
 */
static FONT *read_font_fixed(PACKFILE *pack, int height, int maxchars)
{
   FONT *f = NULL;
   FONT_MONO_DATA *mf = NULL;
   FONT_GLYPH **gl = NULL;
   
   int i = 0;
   
   f = _AL_MALLOC(sizeof(FONT));
   mf = _AL_MALLOC(sizeof(FONT_MONO_DATA));
   gl = _AL_MALLOC(sizeof(FONT_GLYPH *) * maxchars);
   
   if (!f || !mf || !gl) {
      _AL_FREE(f);
      _AL_FREE(mf);
      _AL_FREE(gl);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   f->data = mf;
   f->height = height;
   f->vtable = font_vtable_mono;
   
   mf->begin = ' ';
   mf->end = ' ' + maxchars;
   mf->next = NULL;
   mf->glyphs = gl;
   
   memset(gl, 0, sizeof(FONT_GLYPH *) * maxchars);
   
   for (i = 0; i < maxchars; i++) {
      FONT_GLYPH *g = _AL_MALLOC(sizeof(FONT_GLYPH) + height);
      
      if (!g) {
	 destroy_font(f);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      
      gl[i] = g;
      g->w = 8;
      g->h = height;
      
      pack_fread(g->dat, height, pack);
   }
   
   return f;
}



/* read_font_prop:
 *  Reads a proportional font from a file. This format is no longer used.
 */
static FONT *read_font_prop(PACKFILE *pack, int maxchars)
{
   FONT *f = NULL;
   FONT_COLOR_DATA *cf = NULL;
   BITMAP **bits = NULL;
   int i = 0, h = 0;
   
   f = _AL_MALLOC(sizeof(FONT));
   cf = _AL_MALLOC(sizeof(FONT_COLOR_DATA));
   bits = _AL_MALLOC(sizeof(BITMAP *) * maxchars);
   
   if (!f || !cf || !bits) {
      _AL_FREE(f);
      _AL_FREE(cf);
      _AL_FREE(bits);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   f->data = cf;
   f->vtable = font_vtable_color;
   
   cf->begin = ' ';
   cf->end = ' ' + maxchars;
   cf->next = NULL;
   cf->bitmaps = bits;
   
   memset(bits, 0, sizeof(BITMAP *) * maxchars);
   
   for (i = 0; i < maxchars; i++) {
      if (pack_feof(pack)) break;
      
      bits[i] = read_bitmap(pack, 8, FALSE);
      if (!bits[i]) {
	 destroy_font(f);
	 return NULL;
      }
      
      if (bits[i]->h > h) h = bits[i]->h;
   }
   
   while (i < maxchars) {
      bits[i] = create_bitmap_ex(8, 8, h);
      if (!bits[i]) {
	 destroy_font(f);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      
      clear_bitmap(bits[i]);
      i++;
   }
   
   f->height = h;
   
   return f;
}



/* read_font_mono:
 *  Helper for read_font, below
 */
static FONT_MONO_DATA *read_font_mono(PACKFILE *f, int *hmax)
{
   FONT_MONO_DATA *mf = NULL;
   int max = 0, i = 0;
   FONT_GLYPH **gl = NULL;
   
   mf = _AL_MALLOC(sizeof(FONT_MONO_DATA));
   if (!mf) {
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   mf->begin = pack_mgetl(f);
   mf->end = pack_mgetl(f) + 1;
   mf->next = NULL;
   max = mf->end - mf->begin;
   
   mf->glyphs = gl = _AL_MALLOC(sizeof(FONT_GLYPH *) * max);
   if (!gl) {
      _AL_FREE(mf);
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   for (i = 0; i < max; i++) {
      int w, h, sz;
      
      w = pack_mgetw(f);
      h = pack_mgetw(f);
      sz = ((w + 7) / 8) * h;
      
      if (h > *hmax) *hmax = h;
      
      gl[i] = _AL_MALLOC(sizeof(FONT_GLYPH) + sz);
      if (!gl[i]) {
	 while (i) {
	    i--;
	    _AL_FREE(mf->glyphs[i]);
	 }
	 _AL_FREE(mf);
	 _AL_FREE(mf->glyphs);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }
      
      gl[i]->w = w;
      gl[i]->h = h;
      pack_fread(gl[i]->dat, sz, f);
   }
   
   return mf;
}



/* read_font_color:
 *  Helper for read_font, below.
 */
static FONT_COLOR_DATA *read_font_color(PACKFILE *pack, int *hmax, int depth)
{
   FONT_COLOR_DATA *cf = NULL;
   int max = 0, i = 0;
   BITMAP **bits = NULL;
   
   cf = _AL_MALLOC(sizeof(FONT_COLOR_DATA));
   if (!cf) {
      *allegro_errno = ENOMEM;
      return NULL;
   }
   
   cf->begin = pack_mgetl(pack);
   cf->end = pack_mgetl(pack) + 1;
   cf->next = NULL;
   max = cf->end - cf->begin;

   cf->bitmaps = bits = _AL_MALLOC(sizeof(BITMAP *) * max);
   if (!bits) {
      _AL_FREE(cf);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (i = 0; i < max; i++) {
      /* Do not allow colour conversions for 8 bit bitmaps, but require it for
       * all other colour depths.
       */
      bits[i] = read_bitmap(pack, depth, depth!=8);
      if (!bits[i]) {
	 while (i) {
	    i--;
	    destroy_bitmap(bits[i]);
	 }
	 _AL_FREE(bits);
	 _AL_FREE(cf);
	 *allegro_errno = ENOMEM;
	 return 0;
      }
      
      if (bits[i]->h > *hmax) 
	 *hmax = bits[i]->h;
   }

   return cf;
}



/* read_font:
 *  Reads a new style, Unicode format font from a file.
 */
static FONT *read_font(PACKFILE *pack)
{
   FONT *f = NULL;
   int num_ranges = 0;
   int height = 0;
   int depth;

   f = _AL_MALLOC(sizeof(FONT));
   if (!f) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   f->data = NULL;

   num_ranges = pack_mgetw(pack);
   while (num_ranges--) {
      depth = pack_getc(pack);
      if (depth == 1 || depth == 255) {
	 FONT_MONO_DATA *mf = 0, *iter = (FONT_MONO_DATA *)f->data;
	 
	 f->vtable = font_vtable_mono;

	 mf = read_font_mono(pack, &height);
	 if (!mf) {
	    destroy_font(f);
	    return NULL;
	 }

	 if (!iter)
	    f->data = mf;
	 else {
	    while (iter->next) iter = iter->next;
	    iter->next = mf;
	 }
      } 
      else {
	 FONT_COLOR_DATA *cf = NULL, *iter = (FONT_COLOR_DATA *)f->data;
         
         /* Older versions of Allegro use `0' to indicate a colour font */
         if (depth == 0)
            depth = 8;

	 f->vtable = font_vtable_color;

	 cf = read_font_color(pack, &height, depth);
	 if (!cf) {
	    destroy_font(f);
	    return NULL;
	 }

	 if (!iter) 
	    f->data = cf;
	 else {
	    while (iter->next) iter = iter->next;
	    iter->next = cf;
	 }
      }
   }

   f->height = height;
   return f;
}



/* read_palette:
 *  Reads a palette from a file.
 */
static RGB *read_palette(PACKFILE *f, int size)
{
   RGB *p;
   int c, x;

   p = _AL_MALLOC_ATOMIC(sizeof(PALETTE));
   if (!p) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<size; c++) {
      p[c].r = pack_getc(f) >> 2;
      p[c].g = pack_getc(f) >> 2;
      p[c].b = pack_getc(f) >> 2;
   }

   x = 0;
   while (c < PAL_SIZE) {
      p[c] = p[x];
      c++;
      x++;
      if (x >= size)
	 x = 0;
   }

   return p;
}



/* read_sample:
 *  Reads a sample from a file.
 */
static SAMPLE *read_sample(PACKFILE *f)
{
   signed short bits;
   SAMPLE *s;

   s = _AL_MALLOC(sizeof(SAMPLE));
   if (!s) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   bits = pack_mgetw(f);

   if (bits < 0) {
      s->bits = -bits;
      s->stereo = TRUE;
   }
   else {
      s->bits = bits;
      s->stereo = FALSE;
   }

   s->freq = pack_mgetw(f);
   s->len = pack_mgetl(f);
   s->priority = 128;
   s->loop_start = 0;
   s->loop_end = s->len;
   s->param = 0;

   if (s->bits == 8) {
      s->data = read_block(f, s->len * ((s->stereo) ? 2 : 1), 0);
   }
   else {
      s->data = _AL_MALLOC_ATOMIC(s->len * sizeof(short) * ((s->stereo) ? 2 : 1));
      if (s->data) {
	 int i;

	 for (i=0; i < (int)s->len * ((s->stereo) ? 2 : 1); i++) {
	    ((unsigned short *)s->data)[i] = pack_igetw(f);
	 }

	 if (pack_ferror(f)) {
	    _AL_FREE(s->data);
	    s->data = NULL;
	 }
      }
   }
   if (!s->data) {
      _AL_FREE(s);
      return NULL;
   }

   LOCK_DATA(s, sizeof(SAMPLE));
   LOCK_DATA(s->data, s->len * ((s->bits==8) ? 1 : sizeof(short)) * ((s->stereo) ? 2 : 1));

   return s;
}



/* read_midi:
 *  Reads MIDI data from a datafile (this is not the same thing as the 
 *  standard midi file format).
 */
static MIDI *read_midi(PACKFILE *f)
{
   MIDI *m;
   int c;

   m = _AL_MALLOC(sizeof(MIDI));
   if (!m) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<MIDI_TRACKS; c++) {
      m->track[c].len = 0;
      m->track[c].data = NULL;
   }

   m->divisions = pack_mgetw(f);

   for (c=0; c<MIDI_TRACKS; c++) {
      m->track[c].len = pack_mgetl(f);
      if (m->track[c].len > 0) {
	 m->track[c].data = read_block(f, m->track[c].len, 0);
	 if (!m->track[c].data) {
	    unload_midi(m);
	    return NULL;
	 }
      }
   }

   LOCK_DATA(m, sizeof(MIDI));

   for (c=0; c<MIDI_TRACKS; c++) {
      if (m->track[c].data) {
	 LOCK_DATA(m->track[c].data, m->track[c].len);
      }
   }

   return m;
}



/* read_rle_sprite:
 *  Reads an RLE compressed sprite from a file, allocating memory for it. 
 */
static RLE_SPRITE *read_rle_sprite(PACKFILE *f, int bits)
{
   int x, y, w, h, c, r, g, b, a;
   int destbits, size, rgba;
   RLE_SPRITE *s;
   BITMAP *b1, *b2;
   unsigned int eol_marker;
   signed short s16;
   signed short *p16;
   int32_t *p32;

   if (bits < 0) {
      bits = -bits;
      rgba = TRUE;
   }
   else
      rgba = FALSE;

   w = pack_mgetw(f);
   h = pack_mgetw(f);
   size = pack_mgetl(f);

   s = _AL_MALLOC(sizeof(RLE_SPRITE) + size);
   if (!s) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   s->w = w;
   s->h = h;
   s->color_depth = bits;
   s->size = size;

   switch (bits) {

      case 8:
	 /* easy! */
	 pack_fread(s->dat, size, f);
	 break;

      case 15:
      case 16:
	 /* read hicolor data */
	 p16 = (signed short *)s->dat;
	 eol_marker = (bits == 15) ? MASK_COLOR_15 : MASK_COLOR_16;

	 for (y=0; y<h; y++) {
	    s16 = pack_igetw(f);

	    while ((unsigned short)s16 != MASK_COLOR_16) {
	       if (s16 < 0) {
		  /* skip count */
		  *p16 = s16;
		  p16++;
	       }
	       else {
		  /* solid run */
		  x = s16;
		  *p16 = s16;
		  p16++;

		  while (x-- > 0) {
		     c = pack_igetw(f);
		     r = _rgb_scale_5[(c >> 11) & 0x1F];
		     g = _rgb_scale_6[(c >> 5) & 0x3F];
		     b = _rgb_scale_5[c & 0x1F];
		     *p16 = makecol_depth(bits, r, g, b);
		     p16++;
		  }
	       }

	       s16 = pack_igetw(f);
	    }

	    /* end of line */
	    *p16 = eol_marker;
	    p16++;
	 }
	 break;

      case 24:
      case 32:
	 /* read truecolor data */
	 p32 = (int32_t *)s->dat;
	 eol_marker = (bits == 24) ? MASK_COLOR_24 : MASK_COLOR_32;

	 for (y=0; y<h; y++) {
	    c = pack_igetl(f);

	    while ((uint32_t)c != MASK_COLOR_32) {
	       if (c < 0) {
		  /* skip count */
		  *p32 = c;
		  p32++;
	       }
	       else {
		  /* solid run */
		  x = c;
		  *p32 = c;
		  p32++;

		  while (x-- > 0) {
		     r = pack_getc(f);
		     g = pack_getc(f);
		     b = pack_getc(f);

		     if (rgba)
			a = pack_getc(f);
		     else
			a = 0;

		     *p32 = makeacol_depth(bits, r, g, b, a);

		     p32++;
		  }
	       }

	       c = pack_igetl(f);
	    }

	    /* end of line */
	    *p32 = eol_marker;
	    p32++;
	 }
	 break;
   }

   destbits = _color_load_depth(bits, rgba);

   if (destbits != bits) {
      b1 = create_bitmap_ex(bits, s->w, s->h);
      if (!b1) {
	 destroy_rle_sprite(s);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }

      clear_to_color(b1, bitmap_mask_color(b1));
      draw_rle_sprite(b1, s, 0, 0);

      b2 = create_bitmap_ex(destbits, s->w, s->h);
      if (!b2) {
	 destroy_rle_sprite(s);
	 destroy_bitmap(b1);
	 *allegro_errno = ENOMEM;
	 return NULL;
      }

      blit(b1, b2, 0, 0, 0, 0, s->w, s->h);

      destroy_rle_sprite(s);
      s = get_rle_sprite(b2);

      destroy_bitmap(b1);
      destroy_bitmap(b2);
   }

   return s;
}



/* read_compiled_sprite:
 *  Reads a compiled sprite from a file, allocating memory for it.
 */
static COMPILED_SPRITE *read_compiled_sprite(PACKFILE *f, int planar, int bits)
{
   BITMAP *b;
   COMPILED_SPRITE *s;

   b = read_bitmap(f, bits, TRUE);
   if (!b)
      return NULL;

   if (!_compile_sprites)
      return (COMPILED_SPRITE *)b;

   s = get_compiled_sprite(b, planar);
   if (!s)
      *allegro_errno = ENOMEM;

   destroy_bitmap(b);

   return s;
}



/* read_old_datafile:
 *  Helper for reading old-format datafiles (only needed for backward
 *  compatibility).
 */
static DATAFILE *read_old_datafile(PACKFILE *f, void (*callback)(DATAFILE *))
{
   DATAFILE *dat;
   int size;
   int type;
   int c;

   size = pack_mgetw(f);
   if (size == EOF) {
      pack_fclose(f);
      return NULL;
   }

   dat = _AL_MALLOC(sizeof(DATAFILE)*(size+1));
   if (!dat) {
      pack_fclose(f);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c<=size; c++) {
      dat[c].type = DAT_END;
      dat[c].dat = NULL;
      dat[c].size = 0;
      dat[c].prop = NULL;
   }

   *allegro_errno = 0;

   for (c=0; c<size; c++) {

      type = pack_mgetw(f);

      switch (type) {

	 case V1_DAT_FONT: 
	 case V1_DAT_FONT_8x8: 
	    dat[c].type = DAT_FONT;
	    dat[c].dat = read_font_fixed(f, 8, OLD_FONT_SIZE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_FONT_PROP:
	    dat[c].type = DAT_FONT;
	    dat[c].dat = read_font_prop(f, OLD_FONT_SIZE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_BITMAP:
	 case V1_DAT_BITMAP_256:
	    dat[c].type = DAT_BITMAP;
	    dat[c].dat = read_bitmap(f, 8, TRUE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_BITMAP_16:
	    dat[c].type = DAT_BITMAP;
	    dat[c].dat = read_bitmap(f, 4, FALSE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_SPRITE_256:
	    dat[c].type = DAT_BITMAP;
	    pack_mgetw(f);
	    dat[c].dat = read_bitmap(f, 8, TRUE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_SPRITE_16:
	    dat[c].type = DAT_BITMAP;
	    pack_mgetw(f);
	    dat[c].dat = read_bitmap(f, 4, FALSE);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_PALETTE:
	 case V1_DAT_PALETTE_256:
	    dat[c].type = DAT_PALETTE;
	    dat[c].dat = read_palette(f, PAL_SIZE);
	    dat[c].size = sizeof(PALETTE);
	    break;

	 case V1_DAT_PALETTE_16:
	    dat[c].type = DAT_PALETTE;
	    dat[c].dat = read_palette(f, 16);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_SAMPLE:
	    dat[c].type = DAT_SAMPLE;
	    dat[c].dat = read_sample(f);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_MIDI:
	    dat[c].type = DAT_MIDI;
	    dat[c].dat = read_midi(f);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_RLE_SPRITE:
	    dat[c].type = DAT_RLE_SPRITE;
	    dat[c].dat = read_rle_sprite(f, 8);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_FLI:
	    dat[c].type = DAT_FLI;
	    dat[c].size = pack_mgetl(f);
	    dat[c].dat = read_block(f, dat[c].size, 0);
	    break;

	 case V1_DAT_C_SPRITE:
	    dat[c].type = DAT_C_SPRITE;
	    dat[c].dat = read_compiled_sprite(f, FALSE, 8);
	    dat[c].size = 0;
	    break;

	 case V1_DAT_XC_SPRITE:
	    dat[c].type = DAT_XC_SPRITE;
	    dat[c].dat = read_compiled_sprite(f, TRUE, 8);
	    dat[c].size = 0;
	    break;

	 default:
	    dat[c].type = DAT_DATA;
	    dat[c].size = pack_mgetl(f);
	    dat[c].dat = read_block(f, dat[c].size, 0);
	    break;
      }

      if (*allegro_errno) {
	 if (!dat[c].dat)
	    dat[c].type = DAT_END;
	 unload_datafile(dat);
	 pack_fclose(f);
	 return NULL;
      }

      if (callback)
	 callback(dat+c);
   }

   return dat;
}



/* load_data_object:
 *  Loads a binary data object from a datafile.
 */
static void *load_data_object(PACKFILE *f, long size)
{
   return read_block(f, size, 0);
}



/* load_font_object:
 *  Loads a font object from a datafile.
 */
static void *load_font_object(PACKFILE *f, long size)
{
   short height = pack_mgetw(f);

   if (height > 0)
      return read_font_fixed(f, height, LESS_OLD_FONT_SIZE);
   else if (height < 0)
      return read_font_prop(f, LESS_OLD_FONT_SIZE);
   else
      return read_font(f);
}



/* load_sample_object:
 *  Loads a sample object from a datafile.
 */
static void *load_sample_object(PACKFILE *f, long size)
{
   return read_sample(f);
}



/* load_midi_object:
 *  Loads a midifile object from a datafile.
 */
static void *load_midi_object(PACKFILE *f, long size)
{
   return read_midi(f);
}



/* load_bitmap_object:
 *  Loads a bitmap object from a datafile.
 */
static void *load_bitmap_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_bitmap(f, bits, TRUE);
}



/* load_rle_sprite_object:
 *  Loads an RLE sprite object from a datafile.
 */
static void *load_rle_sprite_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_rle_sprite(f, bits);
}



/* load_compiled_sprite_object:
 *  Loads a compiled sprite object from a datafile.
 */
static void *load_compiled_sprite_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_compiled_sprite(f, FALSE, bits);
}



/* load_xcompiled_sprite_object:
 *  Loads a mode-X compiled object from a datafile.
 */
static void *load_xcompiled_sprite_object(PACKFILE *f, long size)
{
   short bits = pack_mgetw(f);

   return read_compiled_sprite(f, TRUE, bits);
}



/* unload_sample: 
 *  Destroys a sample object.
 */
static void unload_sample(SAMPLE *s)
{
   if (s) {
      if (s->data) {
	 UNLOCK_DATA(s->data, s->len * ((s->bits==8) ? 1 : sizeof(short)) * ((s->stereo) ? 2 : 1));
	 _AL_FREE(s->data);
      }

      UNLOCK_DATA(s, sizeof(SAMPLE));
      _AL_FREE(s);
   }
}



/* unload_midi: 
 *  Destroys a MIDI object.
 */
static void unload_midi(MIDI *m)
{
   int c;

   if (m) {
      for (c=0; c<MIDI_TRACKS; c++) {
	 if (m->track[c].data) {
	    UNLOCK_DATA(m->track[c].data, m->track[c].len);
	    _AL_FREE(m->track[c].data);
	 }
      }

      UNLOCK_DATA(m, sizeof(MIDI));
      _AL_FREE(m);
   }
}



/* load_object:
 *  Helper to load an object from a datafile and store it in 'obj'.
 *  Returns 0 on success and -1 on failure.
 */
static int load_object(DATAFILE *obj, PACKFILE *f, int type)
{
   PACKFILE *ff;
   int d, i;

   /* load actual data */
   ff = pack_fopen_chunk(f, FALSE);

   if (ff) {
      d = ff->normal.todo;

      /* look for a load function */
      for (i=0; i<MAX_DATAFILE_TYPES; i++) {
	 if (_datafile_type[i].type == type) {
	    obj->dat = _datafile_type[i].load(ff, d);
	    goto Found;
	 }
      }

      /* if not found, load binary data */
      obj->dat = load_data_object(ff, d);

    Found:
      pack_fclose_chunk(ff);

      if (!obj->dat)
	 return -1;

      obj->type = type;
      obj->size = d;
      return 0;
   }

   return -1;
}



/* _load_property:
 *  Helper to load a property from a datafile and store it in 'prop'.
 *  Returns 0 on success and -1 on failure.
 */
int _load_property(DATAFILE_PROPERTY *prop, PACKFILE *f)
{
   int type, size;
   char *p;

   type = pack_mgetl(f);
   size = pack_mgetl(f);

   prop->type = type;
   prop->dat = _AL_MALLOC_ATOMIC(size+1); /* '1' for end-of-string delimiter */
   if (!prop->dat) {
      *allegro_errno = ENOMEM;
      pack_fseek(f, size);
      return -1;
   }

   /* read the property */
   pack_fread(prop->dat, size, f);
   prop->dat[size] = 0;

   /* convert to current encoding format if needed */
   if (need_uconvert(prop->dat, U_UTF8, U_CURRENT)) {
      int length = uconvert_size(prop->dat, U_UTF8, U_CURRENT);
      p = _AL_MALLOC_ATOMIC(length);
      if (!p) {
	 *allegro_errno = ENOMEM;
	 return -1;
      }

      do_uconvert(prop->dat, U_UTF8, p, U_CURRENT, length);
      _AL_FREE(prop->dat);
      prop->dat = p;
   }

   return 0;
}



/* _add_property:
 *  Helper to add a new property to a property list. Returns 0 on
 *  success or -1 on failure.
 */
int _add_property(DATAFILE_PROPERTY **list, DATAFILE_PROPERTY *prop)
{
   DATAFILE_PROPERTY *iter;
   int length = 0;

   ASSERT(list);
   ASSERT(prop);

   /* find the length of the list */
   if (*list) {
      iter = *list;
      while (iter->type != DAT_END) {
	 length++;
	 iter++;
      }
   }

   /* grow the list */
   *list = _al_sane_realloc(*list, sizeof(DATAFILE_PROPERTY)*(length+2));
   if (!(*list)) {
      *allegro_errno = ENOMEM;
      return -1;
   }

   /* copy the new property */
   (*list)[length] = *prop;

   /* set end-of-array marker */
   (*list)[length+1].type = DAT_END;
   (*list)[length+1].dat = NULL;

   return 0;
}



/* _destroy_property_list:
 *  Helper to destroy a property list.
 */
void _destroy_property_list(DATAFILE_PROPERTY *list)
{
   int c;

   for (c=0; list[c].type != DAT_END; c++) {
      if (list[c].dat)
	 _AL_FREE(list[c].dat);
   }

   _AL_FREE(list);
}



/* load_file_object:
 *  Loads a datafile object.
 */
static void *load_file_object(PACKFILE *f, long size)
{
   DATAFILE *dat;
   DATAFILE_PROPERTY prop, *list;
   int count, c, type, failed;

   count = pack_mgetl(f);

   dat = _AL_MALLOC(sizeof(DATAFILE)*(count+1));
   if (!dat) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   list = NULL;
   failed = FALSE;

   /* search the packfile for properties or objects */
   for (c=0; c<count;) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
	 if ((_load_property(&prop, f) != 0) || (_add_property(&list, &prop) != 0)) {
	    failed = TRUE;
	    break;
	 }
      }
      else {
	 if (load_object(&dat[c], f, type) != 0) {
	    failed = TRUE;
	    break;
	 }

	 /* attach the property list to the object */
	 dat[c].prop = list;
	 list = NULL;

	 if (datafile_callback)
	    datafile_callback(dat+c);

	 c++;
      }
   }

   /* set end-of-array marker */
   dat[c].type = DAT_END;
   dat[c].dat = NULL;

   /* destroy the property list if not assigned to an object */
   if (list)
      _destroy_property_list(list);

   /* gracefully handle failure */
   if (failed) {
      unload_datafile(dat);
      _AL_FREE(dat);
      dat = NULL;
   }

   return dat;
}



/* load_datafile:
 *  Loads an entire data file into memory, and returns a pointer to it. 
 *  On error, sets errno and returns NULL.
 */
DATAFILE *load_datafile(AL_CONST char *filename)
{
   ASSERT(filename);
   return load_datafile_callback(filename, NULL);
}



/* load_datafile_callback:
 *  Loads an entire data file into memory, and returns a pointer to it. 
 *  On error, sets errno and returns NULL.
 */
DATAFILE *load_datafile_callback(AL_CONST char *filename, void (*callback)(DATAFILE *))
{
   PACKFILE *f;
   DATAFILE *dat;
   int type;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ_PACKED);
   if (!f)
      return NULL;

   if ((f->normal.flags & PACKFILE_FLAG_CHUNK) && (!(f->normal.flags & PACKFILE_FLAG_EXEDAT)))
      type = (_packfile_type == DAT_FILE) ? DAT_MAGIC : 0;
   else
      type = pack_mgetl(f);

   if (type == V1_DAT_MAGIC) {
      dat = read_old_datafile(f, callback);
   }
   else if (type == DAT_MAGIC) {
      datafile_callback = callback;
      dat = load_file_object(f, 0);
      datafile_callback = NULL;
   }
   else
      dat = NULL;

   pack_fclose(f);
   return dat; 
}



/* create_datafile_index
 *  Reads offsets of all objects inside datafile.
 *  On error, sets errno and returns NULL.
 */
DATAFILE_INDEX *create_datafile_index(AL_CONST char *filename)
{
   PACKFILE *f;
   DATAFILE_INDEX *index;
   long pos = 4;
   int type, count, skip, i;

   ASSERT(filename);

   f = pack_fopen(filename, F_READ_PACKED);
   if (!f)
      return NULL;

   if ((f->normal.flags & PACKFILE_FLAG_CHUNK) && (!(f->normal.flags & PACKFILE_FLAG_EXEDAT)))
      type = (_packfile_type == DAT_FILE) ? DAT_MAGIC : 0;
   else {
      type = pack_mgetl(f);   pos += 4;
   }

   /* only support V2 datafile format */
   if (type != DAT_MAGIC)
      return NULL;

   count = pack_mgetl(f);     pos += 4;

   index = _AL_MALLOC(sizeof(DATAFILE_INDEX));
   if (!index) {
      pack_fclose(f);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   index->filename = _al_ustrdup(filename);
   if (!index->filename) {
      pack_fclose(f);
      _AL_FREE(index);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   index->offset = _AL_MALLOC(sizeof(long) * count);
   if (!index->offset) {
      pack_fclose(f);
      _AL_FREE(index->filename);
      _AL_FREE(index);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (i = 0; i < count; ++i) {
      index->offset[i] = pos;

      /* Skip properties */
      while (pos += 4, pack_mgetl(f) == DAT_PROPERTY) {

         /* Skip property name */
         pack_fseek(f, 4);       pos += 4;

         /* Skip rest of property */
         skip = pack_mgetl(f);   pos += 4;      /* Get property size */
         pack_fseek(f, skip);    pos += skip;
      }

      /* Skip rest of object */
      skip = pack_mgetl(f) + 4;  pos += 4;
      pack_fseek(f, skip);       pos += skip;

   }
   pack_fclose(f);
   return index;
}



/* load_datafile_object:
 *  Loads a single object from a datafile.
 */
DATAFILE *load_datafile_object(AL_CONST char *filename, AL_CONST char *objectname)
{
   PACKFILE *f;
   DATAFILE *dat;
   DATAFILE_PROPERTY prop, *list;
   char parent[1024], child[1024], tmp[8];
   char *bufptr, *prevptr, *separator;
   int count, c, type, size, found;

   ASSERT(filename);
   ASSERT(objectname);
  
   /* concatenate to filename#objectname */
   ustrzcpy(parent, sizeof(parent), filename);

   if (ustrcmp(parent, uconvert_ascii("#", tmp)) != 0)
      ustrzcat(parent, sizeof(parent), uconvert_ascii("#", tmp));

   ustrzcat(parent, sizeof(parent), objectname);
   
   /* split into path and actual objectname (for nested files) */
   prevptr = bufptr = parent;
   separator = NULL;
   while ((c = ugetx(&bufptr)) != 0) {
      if ((c == '#') || (c == '/') || (c == OTHER_PATH_SEPARATOR))
	 separator = prevptr;

      prevptr = bufptr;
   }
   
   ustrzcpy(child, sizeof(child), separator + uwidth (separator));

   if (separator == parent)
      usetc(separator + uwidth (separator), 0);
   else
      usetc(separator, 0);

   /* open the parent datafile */
   f = pack_fopen(parent, F_READ_PACKED);
   if (!f)
      return NULL;

   if ((f->normal.flags & PACKFILE_FLAG_CHUNK) && (!(f->normal.flags & PACKFILE_FLAG_EXEDAT)))
      type = (_packfile_type == DAT_FILE) ? DAT_MAGIC : 0;
   else
      type = pack_mgetl(f);

   /* only support V2 datafile format */
   if (type != DAT_MAGIC) {
      pack_fclose(f);
      return NULL;
   }

   count = pack_mgetl(f);

   dat = NULL;
   list = NULL;
   found = FALSE;

   /* search the packfile for properties or objects */
   for (c=0; c<count;) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
	 if ((_load_property(&prop, f) != 0) || (_add_property(&list, &prop) != 0))
	    break;

	 if ((prop.type == DAT_NAME) && (ustricmp(prop.dat, child) == 0))
	    found = TRUE;
      }
      else {
	 if (found) {
	    /* we have found the object, load it */
	    dat = _AL_MALLOC(sizeof(DATAFILE));
	    if (!dat) {
	       *allegro_errno = ENOMEM;
	       break;
	    }

	    if (load_object(dat, f, type) != 0) {
	       _AL_FREE(dat);
	       dat = NULL;
	       break;
	    }

	    /* attach the property list to the object */
	    dat->prop = list;
	    list = NULL;

	    break;
	 }
	 else {
	    /* skip an unwanted object */
	    size = pack_mgetl(f);
	    pack_fseek(f, size+4);  /* '4' for the chunk size */

	    /* destroy the property list */
	    if (list) {
	       _destroy_property_list(list);
	       list = NULL;
	    }

	    c++;
	 }
      }
   }

   /* destroy the property list if not assigned to an object */
   if (list)
      _destroy_property_list(list);

   pack_fclose(f);
   return dat;
}



/* load_datafile_object_indexed
 *  Loads a single object from a datafile using its offset.
 *  On error, returns NULL.
 */
DATAFILE *load_datafile_object_indexed(AL_CONST DATAFILE_INDEX *index, int item)
{
   int type;
   PACKFILE *f;
   DATAFILE *dat;
   DATAFILE_PROPERTY prop, *list = NULL;

   ASSERT(index);

   f = pack_fopen(index->filename, F_READ_PACKED);
   if (!f)
      return NULL;

   dat = _AL_MALLOC(sizeof(DATAFILE));
   if (!dat) {
      pack_fclose(f);
      *allegro_errno = ENOMEM;
      return NULL;
   }

   /* pack_fopen will read first 4 bytes for us */
   pack_fseek(f, index->offset[item] - 4);

   do
      type = pack_mgetl(f);
   while (type == DAT_PROPERTY && _load_property(&prop, f)  == 0 &&
                                  _add_property(&list, &prop) == 0);


   if (load_object(dat, f, type) != 0) {
      pack_fclose(f);
      _AL_FREE(dat);
      _destroy_property_list(list);
      return NULL;
   }

   /* attach the property list to the object */
   dat->prop = list;

   pack_fclose(f);
   return dat;
}



/* _unload_datafile_object:
 *  Helper to destroy a datafile object.
 */
void _unload_datafile_object(DATAFILE *dat)
{
   int i;

   /* destroy the property list */
   if (dat->prop)
      _destroy_property_list(dat->prop);

   /* look for a destructor function */
   for (i=0; i<MAX_DATAFILE_TYPES; i++) {
      if (_datafile_type[i].type == dat->type) {
	 if (dat->dat) {
	    if (_datafile_type[i].destroy)
	       _datafile_type[i].destroy(dat->dat);
	    else
	       _AL_FREE(dat->dat);
	 }
	 return;
      }
   }

   /* if not found, just free the data */
   if (dat->dat)
      _AL_FREE(dat->dat);
}



/* unload_datafile:
 *  Frees all the objects in a datafile.
 */
void unload_datafile(DATAFILE *dat)
{
   int i;

   if (dat) {
      for (i=0; dat[i].type != DAT_END; i++)
	 _unload_datafile_object(dat+i);

      _AL_FREE(dat);
   }
}



/* unload_datafile_index
 *  Frees memory used by datafile index
 */
void destroy_datafile_index(DATAFILE_INDEX *index)
{
   if (index) {
      _AL_FREE(index->filename);
      _AL_FREE(index->offset);
      _AL_FREE(index);
   }
}



/* unload_datafile_object:
 *  Unloads a single datafile object, returned by load_datafile_object().
 */
void unload_datafile_object(DATAFILE *dat)
{
   if (dat) {
      _unload_datafile_object(dat);
      _AL_FREE(dat);
   }
}



/* find_datafile_object:
 *  Returns a pointer to the datafile object with the given name
 */
DATAFILE *find_datafile_object(AL_CONST DATAFILE *dat, AL_CONST char *objectname)
{
   char name[512];
   int recurse = FALSE;
   int pos, c;
   ASSERT(dat);
   ASSERT(objectname);

   /* split up the object name */
   pos = 0;

   while ((c = ugetxc(&objectname)) != 0) {
      if ((c == '#') || (c == '/') || (c == OTHER_PATH_SEPARATOR)) {
	 recurse = TRUE;
	 break;
      }
      pos += usetc(name+pos, c);
   }

   usetc(name+pos, 0);

   /* search for the requested object */
   for (pos=0; dat[pos].type != DAT_END; pos++) {
      if (ustricmp(name, get_datafile_property(dat+pos, DAT_NAME)) == 0) {
	 if (recurse) {
	    if (dat[pos].type == DAT_FILE)
	       return find_datafile_object(dat[pos].dat, objectname);
	    else
	       return NULL;
	 }
	 else
	    return (DATAFILE*)dat+pos;
      }
   }

   /* oh dear, the object isn't there... */
   return NULL; 
}



/* get_datafile_property:
 *  Returns the specified property string for the datafile object, or
 *  an empty string if the property does not exist.
 */
AL_CONST char *get_datafile_property(AL_CONST DATAFILE *dat, int type)
{
   DATAFILE_PROPERTY *prop;
   ASSERT(dat);

   prop = dat->prop;
   if (prop) {
      while (prop->type != DAT_END) {
	 if (prop->type == type)
	    return (prop->dat) ? prop->dat : empty_string;

	 prop++;
      }
   }

   return empty_string;
}



/* fixup_datafile:
 *  For use with compiled datafiles, to initialise them if they are not
 *  constructed and to convert truecolor images into the appropriate
 *  pixel format.
 */
void fixup_datafile(DATAFILE *data)
{
   BITMAP *bmp;
   RLE_SPRITE *rle;
   int i, c, r, g, b, a, x, y;
   int bpp, depth;
   unsigned char *p8;
   unsigned short *p16;
   uint32_t *p32;
   signed short *s16;
   int32_t *s32;
   int eol_marker;
   ASSERT(data);

   /* initialise the datafile if needed */
   if (!constructed_datafiles)
      initialise_datafile(data);

   for (i=0; data[i].type != DAT_END; i++) {

      switch (data[i].type) {

	 case DAT_BITMAP:
	    /* fix up a bitmap object */
	    bmp = data[i].dat;
	    bpp = bitmap_color_depth(bmp);

	    switch (bpp) {

	    #ifdef ALLEGRO_COLOR16

	       case 15:
		  /* fix up a 15 bit hicolor bitmap */
		  if (_color_depth == 16) {
		     GFX_VTABLE *vtable = _get_vtable(16);

		     if (vtable != NULL) {
			depth = 16;
			bmp->vtable = vtable;
		     }
		     else
			depth = 15;
		  }
		  else
		     depth = 15;

		  for (y=0; y<bmp->h; y++) {
		     p16 = (unsigned short *)bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = p16[x];
			r = _rgb_scale_5[c & 0x1F];
			g = _rgb_scale_5[(c >> 5) & 0x1F];
			b = _rgb_scale_5[(c >> 10) & 0x1F];
			p16[x] = makecol_depth(depth, r, g, b);
		     }
		  }
		  break;


	       case 16:
		  /* fix up a 16 bit hicolor bitmap */
		  if (_color_depth == 15) {
		     GFX_VTABLE *vtable = _get_vtable(15);

		     if (vtable != NULL) {
			depth = 15;
			bmp->vtable = vtable;
		     }
		     else
			depth = 16;
		  }
		  else
		     depth = 16;

		  for (y=0; y<bmp->h; y++) {
		     p16 = (unsigned short *)bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = p16[x];
			r = _rgb_scale_5[c & 0x1F];
			g = _rgb_scale_6[(c >> 5) & 0x3F];
			b = _rgb_scale_5[(c >> 11) & 0x1F];
			if (_color_conv & COLORCONV_KEEP_TRANS && depth == 15 &&
			   c == 0xf83f) g = 8; /* don't end up as mask color */
			p16[x] = makecol_depth(depth, r, g, b);
		     }
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR24

	       case 24:
		  /* fix up a 24 bit truecolor bitmap */
		  for (y=0; y<bmp->h; y++) {
		     p8 = bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = READ3BYTES(p8+x*3);
			r = (c & 0xFF);
			g = (c >> 8) & 0xFF;
			b = (c >> 16) & 0xFF;
			WRITE3BYTES(p8+x*3, makecol24(r, g, b));
		     }
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR32

	       case 32:
		  /* fix up a 32 bit truecolor bitmap */
		  for (y=0; y<bmp->h; y++) {
		     p32 = (uint32_t *)bmp->line[y];

		     for (x=0; x<bmp->w; x++) {
			c = p32[x];
			r = (c & 0xFF);
			g = (c >> 8) & 0xFF;
			b = (c >> 16) & 0xFF;
			a = (c >> 24) & 0xFF;
			p32[x] = makeacol32(r, g, b, a);
		     }
		  }
		  break;

	    #endif

	    }
	    break;


	 case DAT_RLE_SPRITE:
	    /* fix up an RLE sprite object */
	    rle = data[i].dat;
	    bpp = rle->color_depth;

	    switch (bpp) {

	    #ifdef ALLEGRO_COLOR16

	       case 15:
		  /* fix up a 15 bit hicolor RLE sprite */
		  if (_color_depth == 16) {
		     depth = 16;
		     rle->color_depth = 16;
		     eol_marker = MASK_COLOR_16;
		  }
		  else {
		     depth = 15;
		     eol_marker = MASK_COLOR_15;
		  }

		  s16 = (signed short *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((unsigned short)*s16 != MASK_COLOR_15) {
			if (*s16 > 0) {
			   x = *s16;
			   s16++;
			   while (x-- > 0) {
			      c = *s16;
			      r = _rgb_scale_5[c & 0x1F];
			      g = _rgb_scale_5[(c >> 5) & 0x1F];
			      b = _rgb_scale_5[(c >> 10) & 0x1F];
			      *s16 = makecol_depth(depth, r, g, b);
			      s16++;
			   }
			}
			else
			   s16++;
		     }
		     *s16 = eol_marker;
		     s16++;
		  }
		  break;


	       case 16:
		  /* fix up a 16 bit hicolor RLE sprite */
		  if (_color_depth == 15) {
		     depth = 15;
		     rle->color_depth = 15;
		     eol_marker = MASK_COLOR_15;
		  }
		  else {
		     depth = 16;
		     eol_marker = MASK_COLOR_16;
		  }

		  s16 = (signed short *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((unsigned short)*s16 != MASK_COLOR_16) {
			if (*s16 > 0) {
			   x = *s16;
			   s16++;
			   while (x-- > 0) {
			      c = *s16;
			      r = _rgb_scale_5[c & 0x1F];
			      g = _rgb_scale_6[(c >> 5) & 0x3F];
			      b = _rgb_scale_5[(c >> 11) & 0x1F];
			      *s16 = makecol_depth(depth, r, g, b);
			      s16++;
			   }
			}
			else
			   s16++;
		     }
		     *s16 = eol_marker;
		     s16++;
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR24

	       case 24:
		  /* fix up a 24 bit truecolor RLE sprite */
		  if (_color_depth == 32) {
		     depth = 32;
		     rle->color_depth = 32;
		     eol_marker = MASK_COLOR_32;
		  }
		  else {
		     depth = 24;
		     eol_marker = MASK_COLOR_24;
		  }

		  s32 = (int32_t *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((uint32_t)*s32 != MASK_COLOR_24) {
			if (*s32 > 0) {
			   x = *s32;
			   s32++;
			   while (x-- > 0) {
			      c = *s32;
			      r = (c & 0xFF);
			      g = (c>>8) & 0xFF;
			      b = (c>>16) & 0xFF;
			      *s32 = makecol_depth(depth, r, g, b);
			      s32++;
			   }
			}
			else
			   s32++;
		     }
		     *s32 = eol_marker;
		     s32++;
		  }
		  break;

	    #endif


	    #ifdef ALLEGRO_COLOR32

	       case 32:
		  /* fix up a 32 bit truecolor RLE sprite */
		  if (_color_depth == 24) {
		     depth = 24;
		     rle->color_depth = 24;
		     eol_marker = MASK_COLOR_24;
		  }
		  else {
		     depth = 32;
		     eol_marker = MASK_COLOR_32;
		  }

		  s32 = (int32_t *)rle->dat;

		  for (y=0; y<rle->h; y++) {
		     while ((uint32_t)*s32 != MASK_COLOR_32) {
			if (*s32 > 0) {
			   x = *s32;
			   s32++;
			   while (x-- > 0) {
			      c = *s32;
			      r = (c & 0xFF);
			      g = (c>>8) & 0xFF;
			      b = (c>>16) & 0xFF;
			      if (depth == 32) {
				 a = (c>>24) & 0xFF;
				 *s32 = makeacol32(r, g, b, a);
			      }
			      else
				 *s32 = makecol24(r, g, b);
			      s32++;
			   }
			}
			else
			   s32++;
		     }
		     *s32 = eol_marker;
		     s32++;
		  }
		  break;

	    #endif

	    }
	    break;
      }
   }
}



/* initialise_bitmap:
 *  Helper used by the output from dat2s/c, for fixing up parts
 *  of a BITMAP structure that can't be statically initialised.
 */
static void initialise_bitmap(BITMAP *bmp)
{
   int i;

   for (i=0; _vtable_list[i].vtable; i++) {
      if (_vtable_list[i].color_depth == (int)(uintptr_t)bmp->vtable) {
	 bmp->vtable = _vtable_list[i].vtable;
	 bmp->write_bank = _stub_bank_switch;
	 bmp->read_bank = _stub_bank_switch;
	 bmp->seg = _default_ds();
	 return;
      }
   }

   ASSERT(FALSE); 
}



/* initialise_datafile:
 *  Helper used by the output from dat2s/c, for fixing up parts
 *  of the data that can't be statically initialised.
 */
static void initialise_datafile(DATAFILE *data)
{
   int c, c2, color_flag;
   FONT *f;
   SAMPLE *s;
   MIDI *m;

   for (c=0; data[c].type != DAT_END; c++) {

      switch (data[c].type) {

	 case DAT_FILE:
	    initialise_datafile(data[c].dat);
	    break;

	 case DAT_BITMAP:
	    initialise_bitmap((BITMAP *)data[c].dat);
	    break;

	 case DAT_FONT:
	    f = data[c].dat;
	    color_flag = (int)(uintptr_t)f->vtable;
	    if (color_flag == 1) {
	       FONT_COLOR_DATA *cf = (FONT_COLOR_DATA *)f->data;
	       while (cf) {
		  for (c2 = cf->begin; c2 < cf->end; c2++)
		     initialise_bitmap((BITMAP *)cf->bitmaps[c2 - cf->begin]);
		  cf = cf->next;
	       }
	       f->vtable = font_vtable_color;
	    }
	    else {
	       f->vtable = font_vtable_mono;
	    }
	    break;

	 case DAT_SAMPLE:
	    s = data[c].dat;
	    LOCK_DATA(s, sizeof(SAMPLE));
	    LOCK_DATA(s->data, s->len * ((s->bits==8) ? 1 : sizeof(short)) * ((s->stereo) ? 2 : 1));
	    break;

	 case DAT_MIDI:
	    m = data[c].dat;
	    LOCK_DATA(m, sizeof(MIDI));
	    for (c2=0; c2<MIDI_TRACKS; c2++) {
	       if (m->track[c2].data) {
		  LOCK_DATA(m->track[c2].data, m->track[c2].len);
	       }
	    }
	    break;
      }
   }
}



/* _construct_datafile:
 *  Constructor called by the output from dat2s/c.
 */
void _construct_datafile(DATAFILE *data)
{
   /* record that datafiles are constructed */
   constructed_datafiles = TRUE;

   initialise_datafile(data);
}



/* _initialize_datafile_types:
 *  Register my loader functions with the code in dataregi.c.
 */
#ifdef ALLEGRO_USE_CONSTRUCTOR
   CONSTRUCTOR_FUNCTION(void _initialize_datafile_types(void));
#endif

void _initialize_datafile_types(void)
{
   register_datafile_object(DAT_FILE,         load_file_object,             (void (*)(void *data))unload_datafile        );
   register_datafile_object(DAT_FONT,         load_font_object,             (void (*)(void *data))destroy_font           );
   register_datafile_object(DAT_SAMPLE,       load_sample_object,           (void (*)(void *data))unload_sample          );
   register_datafile_object(DAT_MIDI,         load_midi_object,             (void (*)(void *data))unload_midi            );
   register_datafile_object(DAT_BITMAP,       load_bitmap_object,           (void (*)(void *data))destroy_bitmap         );
   register_datafile_object(DAT_RLE_SPRITE,   load_rle_sprite_object,       (void (*)(void *data))destroy_rle_sprite     );
   register_datafile_object(DAT_C_SPRITE,     load_compiled_sprite_object,  (void (*)(void *data))destroy_compiled_sprite);
   register_datafile_object(DAT_XC_SPRITE,    load_xcompiled_sprite_object, (void (*)(void *data))destroy_compiled_sprite);
}
