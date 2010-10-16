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
 *      PCX reader.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* load_pcx:
 *  Loads a 256 color PCX file, returning a bitmap structure and storing
 *  the palette data in the specified palette (this should be an array of
 *  at least 256 RGB structures).
 */
BITMAP *load_pcx(AL_CONST char *filename, RGB *pal)
{
   PACKFILE *f;
   BITMAP *bmp;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   bmp = load_pcx_pf(f, pal);

   pack_fclose(f);

   return bmp;
}



/* load_pcx_pf:
 *  Like load_pcx, but starts loading from the current place in the PACKFILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
BITMAP *load_pcx_pf(PACKFILE *f, RGB *pal)
{
   BITMAP *b;
   PALETTE tmppal;
   int want_palette = TRUE;
   int c;
   int width, height;
   int bpp, bytes_per_line;
   int xx, po;
   int x, y;
   char ch;
   int dest_depth;
   ASSERT(f);

   /* we really need a palette */
   if (!pal) {
      want_palette = FALSE;
      pal = tmppal;
   }

   pack_getc(f);                    /* skip manufacturer ID */
   pack_getc(f);                    /* skip version flag */
   pack_getc(f);                    /* skip encoding flag */

   if (pack_getc(f) != 8) {         /* we like 8 bit color planes */
      return NULL;
   }

   width = -(pack_igetw(f));        /* xmin */
   height = -(pack_igetw(f));       /* ymin */
   width += pack_igetw(f) + 1;      /* xmax */
   height += pack_igetw(f) + 1;     /* ymax */

   pack_igetl(f);                   /* skip DPI values */

   for (c=0; c<16; c++) {           /* read the 16 color palette */
      pal[c].r = pack_getc(f) / 4;
      pal[c].g = pack_getc(f) / 4;
      pal[c].b = pack_getc(f) / 4;
   }

   pack_getc(f);

   bpp = pack_getc(f) * 8;          /* how many color planes? */
   if ((bpp != 8) && (bpp != 24)) {
      return NULL;
   }

   dest_depth = _color_load_depth(bpp, FALSE);
   bytes_per_line = pack_igetw(f);

   for (c=0; c<60; c++)             /* skip some more junk */
      pack_getc(f);

   b = create_bitmap_ex(bpp, width, height);
   if (!b) {
      return NULL;
   }

   *allegro_errno = 0;

   for (y=0; y<height; y++) {       /* read RLE encoded PCX data */
      x = xx = 0;
#ifdef ALLEGRO_LITTLE_ENDIAN
      po = _rgb_r_shift_24/8;
#elif defined ALLEGRO_BIG_ENDIAN
      po = 2 - _rgb_r_shift_24/8;
#elif !defined SCAN_DEPEND
   #error endianess not defined
#endif

      while (x < bytes_per_line*bpp/8) {
	 ch = pack_getc(f);
	 if ((ch & 0xC0) == 0xC0) {
	    c = (ch & 0x3F);
	    ch = pack_getc(f);
	 }
	 else
	    c = 1;

	 if (bpp == 8) {
	    while (c--) {
	       if (x < b->w)
		  b->line[y][x] = ch;
	       x++;
	    }
	 }
	 else {
	    while (c--) {
	       if (xx < b->w)
		  b->line[y][xx*3+po] = ch;
	       x++;
	       if (x == bytes_per_line) {
		  xx = 0;
#ifdef ALLEGRO_LITTLE_ENDIAN
		  po = _rgb_g_shift_24/8;
#elif defined ALLEGRO_BIG_ENDIAN
		  po = 2 - _rgb_g_shift_24/8;
#elif !defined SCAN_DEPEND
   #error endianess not defined
#endif
	       }
	       else if (x == bytes_per_line*2) {
		  xx = 0;
#ifdef ALLEGRO_LITTLE_ENDIAN
		  po = _rgb_b_shift_24/8;
#elif defined ALLEGRO_BIG_ENDIAN
		  po = 2 - _rgb_b_shift_24/8;
#elif !defined SCAN_DEPEND
   #error endianess not defined
#endif
	       }
	       else
		  xx++;
	    }
	 }
      }
   }

   if (bpp == 8) {                  /* look for a 256 color palette */
      while ((c = pack_getc(f)) != EOF) { 
	 if (c == 12) {
	    for (c=0; c<256; c++) {
	       pal[c].r = pack_getc(f) / 4;
	       pal[c].g = pack_getc(f) / 4;
	       pal[c].b = pack_getc(f) / 4;
	    }
	    break;
	 }
      }
   }

   if (*allegro_errno) {
      destroy_bitmap(b);
      return NULL;
   }

   if (dest_depth != bpp) {
      /* restore original palette except if it comes from the bitmap */
      if ((bpp != 8) && (!want_palette))
	 pal = NULL;

      b = _fixup_loaded_bitmap(b, pal, dest_depth);
   }

   /* construct a fake palette if 8-bit mode is not involved */
   if ((bpp != 8) && (dest_depth != 8) && want_palette)
      generate_332_palette(pal);

   return b;
}



/* save_pcx:
 *  Writes a bitmap into a PCX file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
int save_pcx(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal)
{
   PACKFILE *f;
   int ret;
   ASSERT(filename);

   f = pack_fopen(filename, F_WRITE);
   if (!f)
      return -1;

   ret = save_pcx_pf(f, bmp, pal);

   pack_fclose(f);
   
   return ret;
}



/* save_pcx_pf:
 *  Like save_pcx but writes into the PACKFILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
int save_pcx_pf(PACKFILE *f, BITMAP *bmp, AL_CONST RGB *pal)
{
   PALETTE tmppal;
   int c;
   int x, y;
   int runcount;
   int depth, planes;
   char runchar;
   char ch;
   ASSERT(f);
   ASSERT(bmp);

   if (!pal) {
      get_palette(tmppal);
      pal = tmppal;
   }

   depth = bitmap_color_depth(bmp);
   if (depth == 8)
      planes = 1;
   else
      planes = 3;

   *allegro_errno = 0;

   pack_putc(10, f);                      /* manufacturer */
   pack_putc(5, f);                       /* version */
   pack_putc(1, f);                       /* run length encoding  */
   pack_putc(8, f);                       /* 8 bits per pixel */
   pack_iputw(0, f);                      /* xmin */
   pack_iputw(0, f);                      /* ymin */
   pack_iputw(bmp->w-1, f);               /* xmax */
   pack_iputw(bmp->h-1, f);               /* ymax */
   pack_iputw(320, f);                    /* HDpi */
   pack_iputw(200, f);                    /* VDpi */

   for (c=0; c<16; c++) {
      pack_putc(_rgb_scale_6[pal[c].r], f);
      pack_putc(_rgb_scale_6[pal[c].g], f);
      pack_putc(_rgb_scale_6[pal[c].b], f);
   }

   pack_putc(0, f);                       /* reserved */
   pack_putc(planes, f);                  /* one or three color planes */
   pack_iputw(bmp->w, f);                 /* number of bytes per scanline */
   pack_iputw(1, f);                      /* color palette */
   pack_iputw(bmp->w, f);                 /* hscreen size */
   pack_iputw(bmp->h, f);                 /* vscreen size */
   for (c=0; c<54; c++)                   /* filler */
      pack_putc(0, f);

   for (y=0; y<bmp->h; y++) {             /* for each scanline... */
      runcount = 0;
      runchar = 0;
      for (x=0; x<bmp->w*planes; x++) {   /* for each pixel... */
	 if (depth == 8) {
	    ch = getpixel(bmp, x, y);
	 }
	 else {
	    if (x<bmp->w) {
	       c = getpixel(bmp, x, y);
	       ch = getr_depth(depth, c);
	    }
	    else if (x<bmp->w*2) {
	       c = getpixel(bmp, x-bmp->w, y);
	       ch = getg_depth(depth, c);
	    }
	    else {
	       c = getpixel(bmp, x-bmp->w*2, y);
	       ch = getb_depth(depth, c);
	    }
	 }
	 if (runcount==0) {
	    runcount = 1;
	    runchar = ch;
	 }
	 else {
	    if ((ch != runchar) || (runcount >= 0x3f)) {
	       if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
		  pack_putc(0xC0 | runcount, f);
	       pack_putc(runchar,f);
	       runcount = 1;
	       runchar = ch;
	    }
	    else
	       runcount++;
	 }
      }
      if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
	 pack_putc(0xC0 | runcount, f);
      pack_putc(runchar,f);
   }

   if (depth == 8) {                      /* 256 color palette */
      pack_putc(12, f); 

      for (c=0; c<256; c++) {
	 pack_putc(_rgb_scale_6[pal[c].r], f);
	 pack_putc(_rgb_scale_6[pal[c].g], f);
	 pack_putc(_rgb_scale_6[pal[c].b], f);
      }
   }

   if (*allegro_errno)
      return -1;
   else
      return 0;
}

