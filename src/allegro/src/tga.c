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
 *      TGA reader.
 *
 *      By Tim Gunn.
 *
 *      RLE support added by Michal Mertl and Salvador Eduardo Tropea.
 * 
 *      Palette reading improved by Peter Wang.
 *
 *      Big-endian support added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* raw_tga_read8:
 *  Helper for reading 256-color raw data from TGA files.
 */
static INLINE unsigned char *raw_tga_read8(unsigned char *b, int w, PACKFILE *f)
{
   return b + pack_fread(b, w, f);
}



/* rle_tga_read8:
 *  Helper for reading 256-color RLE data from TGA files.
 */
static void rle_tga_read8(unsigned char *b, int w, PACKFILE *f)
{
   int value, count, c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 /* run-length packet */
	 count = (count & 0x7F) + 1;
	 c += count;
	 value = pack_getc(f);
	 while (count--)
	    *b++ = value;
      }
      else {
	 /* raw packet */
	 count++;
	 c += count;
	 b = raw_tga_read8(b, count, f);
      }
   } while (c < w);
}



/* single_tga_read32:
 *  Helper for reading a single 32-bit data from TGA files.
 */
static INLINE int single_tga_read32(PACKFILE *f)
{
   RGB value;
   int alpha;

   value.b = pack_getc(f);
   value.g = pack_getc(f);
   value.r = pack_getc(f);
   alpha = pack_getc(f);

   return makeacol32(value.r, value.g, value.b, alpha);
}



/* raw_tga_read32:
 *  Helper for reading 32-bit raw data from TGA files.
 */
static unsigned int *raw_tga_read32(unsigned int *b, int w, PACKFILE *f)
{
   while (w--)
      *b++ = single_tga_read32(f);

   return b;
}



/* rle_tga_read32:
 *  Helper for reading 32-bit RLE data from TGA files.
 */
static void rle_tga_read32(unsigned int *b, int w, PACKFILE *f)
{
   int color, count, c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 /* run-length packet */
	 count = (count & 0x7F) + 1;
	 c += count;
	 color = single_tga_read32(f);
	 while (count--)
	    *b++ = color;
      }
      else {
	 /* raw packet */
	 count++;
	 c += count;
	 b = raw_tga_read32(b, count, f);
      }
   } while (c < w);
}


/* single_tga_read24:
 *  Helper for reading a single 24-bit data from TGA files.
 */
static INLINE int single_tga_read24(PACKFILE *f)
{
   RGB value;

   value.b = pack_getc(f);
   value.g = pack_getc(f);
   value.r = pack_getc(f);

   return makecol24(value.r, value.g, value.b);
}



/* raw_tga_read24:
 *  Helper for reading 24-bit raw data from TGA files.
 */
static unsigned char *raw_tga_read24(unsigned char *b, int w, PACKFILE *f)
{
   int color;

   while (w--) {
      color = single_tga_read24(f);
      WRITE3BYTES(b, color);
      b += 3;
   }

   return b;
}



/* rle_tga_read24:
 *  Helper for reading 24-bit RLE data from TGA files.
 */
static void rle_tga_read24(unsigned char *b, int w, PACKFILE *f)
{
   int color, count, c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 /* run-length packet */
	 count = (count & 0x7F) + 1;
	 c += count;
	 color = single_tga_read24(f);
	 while (count--) {
	    WRITE3BYTES(b, color);
	    b += 3;
	 }
      }
      else {
	 /* raw packet */
	 count++;
	 c += count;
	 b = raw_tga_read24(b, count, f);
      }
   } while (c < w);
}



/* single_tga_read16:
 *  Helper for reading a single 16-bit data from TGA files.
 */
static INLINE int single_tga_read16(PACKFILE *f)
{
   int value;

   value = pack_igetw(f);

   return (((value >> 10) & 0x1F) << _rgb_r_shift_15) |
	  (((value >> 5) & 0x1F) << _rgb_g_shift_15)  |
	  ((value & 0x1F) << _rgb_b_shift_15);
}



/* raw_tga_read16:
 *  Helper for reading 16-bit raw data from TGA files.
 */
static unsigned short *raw_tga_read16(unsigned short *b, int w, PACKFILE *f)
{
   while (w--)
      *b++ = single_tga_read16(f);

   return b;
}



/* rle_tga_read16:
 *  Helper for reading 16-bit RLE data from TGA files.
 */
static void rle_tga_read16(unsigned short *b, int w, PACKFILE *f)
{
   int color, count, c = 0;

   do {
      count = pack_getc(f);
      if (count & 0x80) {
	 /* run-length packet */
	 count = (count & 0x7F) + 1;
	 c += count;
	 color = single_tga_read16(f);
	 while (count--)
	    *b++ = color;
      }
      else {
	 /* raw packet */
	 count++;
	 c += count;
	 b = raw_tga_read16(b, count, f);
      }
   } while (c < w);
}



/* load_tga:
 *  Loads a TGA file, returning a bitmap structure and storing the
 *  palette data in the specified palette (this should be an array
 *  of at least 256 RGB structures).
 */
BITMAP *load_tga(AL_CONST char *filename, RGB *pal)
{
   PACKFILE *f;
   BITMAP *bmp;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   bmp = load_tga_pf(f, pal);

   pack_fclose(f);

   return bmp;
}



/* load_tga_pf:
 *  Like load_tga, but starts loading from the current place in the PACKFILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
BITMAP *load_tga_pf(PACKFILE *f, RGB *pal)
{
   unsigned char image_id[256], image_palette[256][3];
   unsigned char id_length, palette_type, image_type, palette_entry_size;
   unsigned char bpp, descriptor_bits;
   short unsigned int first_color, palette_colors;
   short unsigned int left, top, image_width, image_height;
   unsigned int c, i, y, yc;
   int dest_depth;
   int compressed;
   BITMAP *bmp;
   PALETTE tmppal;
   int want_palette = TRUE;
   ASSERT(f);

   /* we really need a palette */
   if (!pal) {
      want_palette = FALSE;
      pal = tmppal;
   }

   id_length = pack_getc(f);
   palette_type = pack_getc(f);
   image_type = pack_getc(f);
   first_color = pack_igetw(f);
   palette_colors  = pack_igetw(f);
   palette_entry_size = pack_getc(f);
   left = pack_igetw(f);
   top = pack_igetw(f);
   image_width = pack_igetw(f);
   image_height = pack_igetw(f);
   bpp = pack_getc(f);
   descriptor_bits = pack_getc(f);

   pack_fread(image_id, id_length, f);

   if (palette_type == 1) {

      for (i = 0; i < palette_colors; i++) {

	 switch (palette_entry_size) {

	    case 16: 
	       c = pack_igetw(f);
	       image_palette[i][0] = (c & 0x1F) << 3;
	       image_palette[i][1] = ((c >> 5) & 0x1F) << 3;
	       image_palette[i][2] = ((c >> 10) & 0x1F) << 3;
	       break;

	    case 24:
	    case 32:
	       image_palette[i][0] = pack_getc(f);
	       image_palette[i][1] = pack_getc(f);
	       image_palette[i][2] = pack_getc(f);
	       if (palette_entry_size == 32)
		  pack_getc(f);
	       break;
	 }
      }
   }
   else if (palette_type != 0) {
      return NULL;
   }

   /* Image type:
    *    0 = no image data
    *    1 = uncompressed color mapped
    *    2 = uncompressed true color
    *    3 = grayscale
    *    9 = RLE color mapped
    *   10 = RLE true color
    *   11 = RLE grayscale
    */
   compressed = (image_type & 8);
   image_type &= 7;

   if ((image_type < 1) || (image_type > 3)) {
      return NULL;
   }

   switch (image_type) {

      case 1:
	 /* paletted image */
	 if ((palette_type != 1) || (bpp != 8)) {
	    return NULL;
	 }

	 for(i=0; i<palette_colors; i++) {
	     pal[i].r = image_palette[i][2] >> 2;
	     pal[i].g = image_palette[i][1] >> 2;
	     pal[i].b = image_palette[i][0] >> 2;
	 }

	 dest_depth = _color_load_depth(8, FALSE);
	 break;

      case 2:
	 /* truecolor image */
	 if ((palette_type == 0) && ((bpp == 15) || (bpp == 16))) {
	    bpp = 15;
	    dest_depth = _color_load_depth(15, FALSE);
	 }
	 else if ((palette_type == 0) && ((bpp == 24) || (bpp == 32))) {
	    dest_depth = _color_load_depth(bpp, (bpp == 32));
	 }
	 else {
	    return NULL;
	 }
	 break;

      case 3:
	 /* grayscale image */
	 if ((palette_type != 0) || (bpp != 8)) {
	    return NULL;
	 }

	 for (i=0; i<256; i++) {
	     pal[i].r = i>>2;
	     pal[i].g = i>>2;
	     pal[i].b = i>>2;
	 }

	 dest_depth = _color_load_depth(bpp, FALSE);
	 break;

      default:
	 return NULL;
   }

   bmp = create_bitmap_ex(bpp, image_width, image_height);
   if (!bmp) {
      return NULL;
   }

   *allegro_errno = 0;

   for (y=image_height; y; y--) {
      yc = (descriptor_bits & 0x20) ? image_height-y : y-1;

      switch (image_type) {

	 case 1:
	 case 3:
	    if (compressed)
	       rle_tga_read8(bmp->line[yc], image_width, f);
	    else
	       raw_tga_read8(bmp->line[yc], image_width, f);
	    break;

	 case 2:
	    if (bpp == 32) {
	       if (compressed)
		  rle_tga_read32((unsigned int *)bmp->line[yc], image_width, f);
	       else
		  raw_tga_read32((unsigned int *)bmp->line[yc], image_width, f);
	    }
	    else if (bpp == 24) {
	       if (compressed)
		  rle_tga_read24(bmp->line[yc], image_width, f);
	       else
		  raw_tga_read24(bmp->line[yc], image_width, f);
	    }
	    else {
	       if (compressed)
		  rle_tga_read16((unsigned short *)bmp->line[yc], image_width, f);
	       else
		  raw_tga_read16((unsigned short *)bmp->line[yc], image_width, f);
	    }
	    break;
      }
   }

   if (*allegro_errno) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (dest_depth != bpp) {
      /* restore original palette except if it comes from the bitmap */
      if ((bpp != 8) && (!want_palette))
	 pal = NULL;

      bmp = _fixup_loaded_bitmap(bmp, pal, dest_depth);
   }
   
   /* construct a fake palette if 8-bit mode is not involved */
   if ((bpp != 8) && (dest_depth != 8) && want_palette)
      generate_332_palette(pal);
      
   return bmp;
}



/* save_tga:
 *  Writes a bitmap into a TGA file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
int save_tga(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal)
{
   PACKFILE *f;
   int ret;
   ASSERT(filename);

   f = pack_fopen(filename, F_WRITE);
   if (!f)
      return -1;

   ret = save_tga_pf(f, bmp, pal);

   pack_fclose(f);

   return ret;
}



/* save_tga_pf:
 *  Like save_tga but writes into the PACKFILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
int save_tga_pf(PACKFILE *f, BITMAP *bmp, AL_CONST RGB *pal)
{
   unsigned char image_palette[256][3];
   int x, y, c, r, g, b;
   int depth;
   PALETTE tmppal;
   ASSERT(f);
   ASSERT(bmp);

   if (!pal) {
      get_palette(tmppal);
      pal = tmppal;
   }

   depth = bitmap_color_depth(bmp);

   if (depth == 15)
      depth = 16;

   *allegro_errno = 0;

   pack_putc(0, f);                          /* id length (no id saved) */
   pack_putc((depth == 8) ? 1 : 0, f);       /* palette type */
   pack_putc((depth == 8) ? 1 : 2, f);       /* image type */
   pack_iputw(0, f);                         /* first colour */
   pack_iputw((depth == 8) ? 256 : 0, f);    /* number of colours */
   pack_putc((depth == 8) ? 24 : 0, f);      /* palette entry size */
   pack_iputw(0, f);                         /* left */
   pack_iputw(0, f);                         /* top */
   pack_iputw(bmp->w, f);                    /* width */
   pack_iputw(bmp->h, f);                    /* height */
   pack_putc(depth, f);                      /* bits per pixel */
   pack_putc(_bitmap_has_alpha (bmp) ? 8 : 0, f); /* descriptor (bottom to top, 8-bit alpha) */

   if (depth == 8) {
      for (y=0; y<256; y++) {
	 image_palette[y][2] = _rgb_scale_6[pal[y].r];
	 image_palette[y][1] = _rgb_scale_6[pal[y].g];
	 image_palette[y][0] = _rgb_scale_6[pal[y].b];
      }

      pack_fwrite(image_palette, 768, f);
   }

   switch (bitmap_color_depth(bmp)) {

      #ifdef ALLEGRO_COLOR8

	 case 8:
	    for (y=bmp->h; y; y--)
	       for (x=0; x<bmp->w; x++)
		  pack_putc(getpixel(bmp, x, y-1), f);
	    break;

      #endif

      #ifdef ALLEGRO_COLOR16

	 case 15:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  r = getr15(c);
		  g = getg15(c);
		  b = getb15(c);
		  c = ((r<<7)&0x7C00) | ((g<<2)&0x3E0) | ((b>>3)&0x1F);
		  pack_iputw(c, f);
	       }
	    }
	    break;

	 case 16:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  r = getr16(c);
		  g = getg16(c);
		  b = getb16(c);
		  c = ((r<<7)&0x7C00) | ((g<<2)&0x3E0) | ((b>>3)&0x1F);
		  pack_iputw(c, f);
	       }
	    }
	    break;

      #endif

      #ifdef ALLEGRO_COLOR24

	 case 24:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  pack_putc(getb24(c), f);
		  pack_putc(getg24(c), f);
		  pack_putc(getr24(c), f);
	       }
	    }
	    break;

      #endif

      #ifdef ALLEGRO_COLOR32

	 case 32:
	    for (y=bmp->h; y; y--) {
	       for (x=0; x<bmp->w; x++) {
		  c = getpixel(bmp, x, y-1);
		  pack_putc(getb32(c), f);
		  pack_putc(getg32(c), f);
		  pack_putc(getr32(c), f);
		  pack_putc(geta32(c), f);
	       }
	    }
	    break;

      #endif
   }

   if (*allegro_errno)
      return -1;
   else
      return 0;
}


