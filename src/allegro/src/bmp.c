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
 *      BMP reader.
 *
 *      By Seymour Shlien.
 *
 *      OS/2 BMP support and BMP save function by Jonas Petersen.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"


#define BI_RGB          0
#define BI_RLE8         1
#define BI_RLE4         2
#define BI_BITFIELDS    3

#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40


typedef struct BITMAPFILEHEADER
{
   unsigned long  bfType;
   unsigned long  bfSize;
   unsigned short bfReserved1;
   unsigned short bfReserved2;
   unsigned long  bfOffBits;
} BITMAPFILEHEADER;


/* Used for both OS/2 and Windows BMP. 
 * Contains only the parameters needed to load the image 
 */
typedef struct BITMAPINFOHEADER
{
   unsigned long  biWidth;
   signed long    biHeight;
   unsigned short biBitCount;
   unsigned long  biCompression;
} BITMAPINFOHEADER;


typedef struct WINBMPINFOHEADER  /* size: 40 */
{
   unsigned long  biWidth;
   signed long    biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
   unsigned long  biCompression;
   unsigned long  biSizeImage;
   unsigned long  biXPelsPerMeter;
   unsigned long  biYPelsPerMeter;
   unsigned long  biClrUsed;
   unsigned long  biClrImportant;
} WINBMPINFOHEADER;


typedef struct OS2BMPINFOHEADER  /* size: 12 */
{
   unsigned short biWidth;
   unsigned short biHeight;
   unsigned short biPlanes;
   unsigned short biBitCount;
} OS2BMPINFOHEADER;



/* read_bmfileheader:
 *  Reads a BMP file header and check that it has the BMP magic number.
 */
static int read_bmfileheader(PACKFILE *f, BITMAPFILEHEADER *fileheader)
{
   fileheader->bfType = pack_igetw(f);
   fileheader->bfSize= pack_igetl(f);
   fileheader->bfReserved1= pack_igetw(f);
   fileheader->bfReserved2= pack_igetw(f);
   fileheader->bfOffBits= pack_igetl(f);

   if (fileheader->bfType != 19778)
      return -1;

   return 0;
}



/* read_win_bminfoheader:
 *  Reads information from a BMP file header.
 */
static int read_win_bminfoheader(PACKFILE *f, BITMAPINFOHEADER *infoheader)
{
   WINBMPINFOHEADER win_infoheader;

   win_infoheader.biWidth = pack_igetl(f);
   win_infoheader.biHeight = pack_igetl(f);
   win_infoheader.biPlanes = pack_igetw(f);
   win_infoheader.biBitCount = pack_igetw(f);
   win_infoheader.biCompression = pack_igetl(f);
   win_infoheader.biSizeImage = pack_igetl(f);
   win_infoheader.biXPelsPerMeter = pack_igetl(f);
   win_infoheader.biYPelsPerMeter = pack_igetl(f);
   win_infoheader.biClrUsed = pack_igetl(f);
   win_infoheader.biClrImportant = pack_igetl(f);

   infoheader->biWidth = win_infoheader.biWidth;
   infoheader->biHeight = win_infoheader.biHeight;
   infoheader->biBitCount = win_infoheader.biBitCount;
   infoheader->biCompression = win_infoheader.biCompression;

   return 0;
}



/* read_os2_bminfoheader:
 *  Reads information from an OS/2 format BMP file header.
 */
static int read_os2_bminfoheader(PACKFILE *f, BITMAPINFOHEADER *infoheader)
{
   OS2BMPINFOHEADER os2_infoheader;

   os2_infoheader.biWidth = pack_igetw(f);
   os2_infoheader.biHeight = pack_igetw(f);
   os2_infoheader.biPlanes = pack_igetw(f);
   os2_infoheader.biBitCount = pack_igetw(f);

   infoheader->biWidth = os2_infoheader.biWidth;
   infoheader->biHeight = os2_infoheader.biHeight;
   infoheader->biBitCount = os2_infoheader.biBitCount;
   infoheader->biCompression = 0;

   return 0;
}



/* read_bmicolors:
 *  Loads the color palette for 1,4,8 bit formats.
 */
static void read_bmicolors(int bytes, RGB *pal, PACKFILE *f, int win_flag)
{
   int i, j;

   for (i=j=0; (i+3 <= bytes && j < PAL_SIZE); j++) {
      pal[j].b = pack_getc(f) / 4;
      pal[j].g = pack_getc(f) / 4;
      pal[j].r = pack_getc(f) / 4;

      i += 3;

      if (win_flag && i < bytes) {
	 pack_getc(f);
	 i++;
      }
   }

   for (; i<bytes; i++)
      pack_getc(f);
}



/* read_1bit_line:
 *  Support function for reading the 1 bit bitmap file format.
 */
static void read_1bit_line(int length, PACKFILE *f, BITMAP *bmp, int line)
{
   unsigned char b[32];
   unsigned long n;
   int i, j, k;
   int pix;

   for (i=0; i<length; i++) {
      j = i % 32;
      if (j == 0) {
	 n = pack_mgetl(f);
	 for (k=0; k<32; k++) {
	    b[31-k] = (char)(n & 1);
	    n = n >> 1;
	 }
      }
      pix = b[j];
      bmp->line[line][i] = pix;
   }
}



/* read_4bit_line:
 *  Support function for reading the 4 bit bitmap file format.
 */
static void read_4bit_line(int length, PACKFILE *f, BITMAP *bmp, int line)
{
   unsigned char b[8];
   unsigned long n;
   int i, j, k;
   int temp;
   int pix;

   for (i=0; i<length; i++) {
      j = i % 8;
      if (j == 0) {
	 n = pack_igetl(f);
	 for (k=0; k<4; k++) {
	    temp = n & 255;
	    b[k*2+1] = temp & 15;
	    temp = temp >> 4;
	    b[k*2] = temp & 15;
	    n = n >> 8;
	 }
      }
      pix = b[j];
      bmp->line[line][i] = pix;
   }
}



/* read_8bit_line:
 *  Support function for reading the 8 bit bitmap file format.
 */
static void read_8bit_line(int length, PACKFILE *f, BITMAP *bmp, int line)
{
   unsigned char b[4];
   unsigned long n;
   int i, j, k;
   int pix;

   for (i=0; i<length; i++) {
      j = i % 4;
      if (j == 0) {
	 n = pack_igetl(f);
	 for (k=0; k<4; k++) {
	    b[k] = (char)(n & 255);
	    n = n >> 8;
	 }
      }
      pix = b[j];
      bmp->line[line][i] = pix;
   }
}



/* read_16bit_line:
 *  Support function for reading the 16 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void read_16bit_line(int length, PACKFILE *f, BITMAP *bmp, int line)
{
   int i, w;
   RGB c;

   for (i=0; i<length; i++) {
      w = pack_igetw(f);

      /* the format is like a 15-bpp bitmap, not 16bpp */
      c.r = (w >> 10) & 0x1f;
      c.g = (w >> 5) & 0x1f;
      c.b = w & 0x1f;

      bmp_write16(bmp->line[line]+i*2,
	 makecol16(_rgb_scale_5[c.r],
	    _rgb_scale_5[c.g],
	    _rgb_scale_5[c.b]));
   }

   /* padding */
   i = (i * 2) % 4;
   if (i != 0) {
      while (i++ < 4)
	 pack_getc(f);
   }
}



/* read_24bit_line:
 *  Support function for reading the 24 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void read_24bit_line(int length, PACKFILE *f, BITMAP *bmp, int line)
{
   int i;
   RGB c;

   for (i=0; i<length; i++) {
      c.b = pack_getc(f);
      c.g = pack_getc(f);
      c.r = pack_getc(f);
      WRITE3BYTES(bmp->line[line]+i*3, makecol24(c.r, c.g, c.b));
   }

   /* padding */
   i = (i * 3) % 4;
   if (i != 0) {
      while (i++ < 4)
         pack_getc(f);
   }
}



/* read_32bit_line:
 *  Support function for reading the 32 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void read_32bit_line(int length, PACKFILE *f, BITMAP *bmp, int line)
{
   int i;
   RGB c;
   char a;

   for (i=0; i<length; i++) {
      c.b = pack_getc(f);
      c.g = pack_getc(f);
      c.r = pack_getc(f);
      a = pack_getc(f);
      WRITE3BYTES(bmp->line[line]+i*4, makeacol32(c.r, c.g, c.b, a));
   }
}



/* read_bitfields_image:
 *  For reading the bitfield compressed BMP image format.
 */
static void read_bitfields_image(PACKFILE *f, BITMAP *bmp, AL_CONST BITMAPINFOHEADER *infoheader)
{
   int k, i, line, height, dir;
   int bpp;
   int bytes_per_pixel;
   int red, grn, blu;
   unsigned long buffer;

   height = infoheader->biHeight;
   line   = height < 0 ? 0 : height-1;
   dir    = height < 0 ? 1 : -1;
   height = ABS(height);
   
   bpp = bitmap_color_depth(bmp);
   bytes_per_pixel = BYTES_PER_PIXEL(bpp);

   for (i=0; i<height; i++, line+=dir) {
      for (k=0; k<(int)infoheader->biWidth; k++) {

	 pack_fread(&buffer, bytes_per_pixel, f);

	 if (bpp == 15) {
	    red = (buffer >> 10) & 0x1f;
	    grn = (buffer >> 5) & 0x1f;
	    blu = (buffer) & 0x1f;
	    buffer = (red << _rgb_r_shift_15) |
		     (grn << _rgb_g_shift_15) |
		     (blu << _rgb_b_shift_15);
	 }
	 else if (bpp == 16) {
	    red = (buffer >> 11) & 0x1f;
	    grn = (buffer >> 5) & 0x3f;
	    blu = (buffer) & 0x1f;
	    buffer = (red << _rgb_r_shift_16) |
		     (grn << _rgb_g_shift_16) |
		     (blu << _rgb_b_shift_16);
	 }
	 else {
	    red = (buffer >> 16) & 0xff;
	    grn = (buffer >> 8) & 0xff;
	    blu = (buffer) & 0xff;
	    buffer = (red << _rgb_r_shift_32) |
		     (grn << _rgb_g_shift_32) |
		     (blu << _rgb_b_shift_32);
	 }

	 memcpy(&bmp->line[line][k * bytes_per_pixel], &buffer, bytes_per_pixel);
      }

      /* padding */
      k = (k * bytes_per_pixel) % 4;
      if (k > 0) {
	 while (k++ < 4)
	    pack_getc(f);
      }
   }
}


/* read_image:
 *  For reading the noncompressed BMP image format.
 */
static void read_image(PACKFILE *f, BITMAP *bmp, AL_CONST BITMAPINFOHEADER *infoheader)
{
   int i, line, height, dir;

   height = infoheader->biHeight;
   line   = height < 0 ? 0: height-1;
   dir    = height < 0 ? 1: -1;
   height = ABS(height);

   for (i=0; i<height; i++, line+=dir) {
      switch (infoheader->biBitCount) {

	 case 1:
	    read_1bit_line(infoheader->biWidth, f, bmp, line);
	    break;

	 case 4:
	    read_4bit_line(infoheader->biWidth, f, bmp, line);
	    break;

	 case 8:
	    read_8bit_line(infoheader->biWidth, f, bmp, line);
	    break;

	 case 16:
	    read_16bit_line(infoheader->biWidth, f, bmp, line);
	    break;

	 case 24:
	    read_24bit_line(infoheader->biWidth, f, bmp, line);
	    break;

	 case 32:
	    read_32bit_line(infoheader->biWidth, f, bmp, line);
	    break;
      }
   }
}



/* read_RLE8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 */
static void read_RLE8_compressed_image(PACKFILE *f, BITMAP *bmp, AL_CONST BITMAPINFOHEADER *infoheader)
{
   unsigned char count, val, val0;
   int j, pos, line;
   int eolflag, eopicflag;

   eopicflag = 0;
   line = infoheader->biHeight - 1;

   while (eopicflag == 0) {
      pos = 0;                               /* x position in bitmap */
      eolflag = 0;                           /* end of line flag */

      while ((eolflag == 0) && (eopicflag == 0)) {
	 count = pack_getc(f);
	 val = pack_getc(f);

	 if (count > 0) {                    /* repeat pixel count times */
	    for (j=0;j<count;j++) {
	       bmp->line[line][pos] = val;
	       pos++;
	    }
	 }
	 else {
	    switch (val) {

	       case 0:                       /* end of line flag */
		  eolflag=1;
		  break;

	       case 1:                       /* end of picture flag */
		  eopicflag=1;
		  break;

	       case 2:                       /* displace picture */
		  count = pack_getc(f);
		  val = pack_getc(f);
		  pos += count;
		  line -= val;
		  break;

	       default:                      /* read in absolute mode */
		  for (j=0; j<val; j++) {
		     val0 = pack_getc(f);
		     bmp->line[line][pos] = val0;
		     pos++;
		  }

		  if (j%2 == 1)
		     val0 = pack_getc(f);    /* align on word boundary */
		  break;

	    }
	 }

	 if (pos-1 > (int)infoheader->biWidth)
	    eolflag=1;
      }

      line--;
      if (line < 0)
	 eopicflag = 1;
   }
}



/* read_RLE4_compressed_image:
 *  For reading the 4 bit RLE compressed BMP image format.
 */
static void read_RLE4_compressed_image(PACKFILE *f, BITMAP *bmp, AL_CONST BITMAPINFOHEADER *infoheader)
{
   unsigned char b[8];
   unsigned char count;
   unsigned short val0, val;
   int j, k, pos, line;
   int eolflag, eopicflag;

   eopicflag = 0;                            /* end of picture flag */
   line = infoheader->biHeight - 1;

   while (eopicflag == 0) {
      pos = 0;
      eolflag = 0;                           /* end of line flag */

      while ((eolflag == 0) && (eopicflag == 0)) {
	 count = pack_getc(f);
	 val = pack_getc(f);

	 if (count > 0) {                    /* repeat pixels count times */
	    b[1] = val & 15;
	    b[0] = (val >> 4) & 15;
	    for (j=0; j<count; j++) {
	       bmp->line[line][pos] = b[j%2];
	       pos++;
	    }
	 }
	 else {
	    switch (val) {

	       case 0:                       /* end of line */
		  eolflag=1;
		  break;

	       case 1:                       /* end of picture */
		  eopicflag=1;
		  break;

	       case 2:                       /* displace image */
		  count = pack_getc(f);
		  val = pack_getc(f);
		  pos += count;
		  line -= val;
		  break;

	       default:                      /* read in absolute mode */
		  for (j=0; j<val; j++) {
		     if ((j%4) == 0) {
			val0 = pack_igetw(f);
			for (k=0; k<2; k++) {
			   b[2*k+1] = val0 & 15;
			   val0 = val0 >> 4;
			   b[2*k] = val0 & 15;
			   val0 = val0 >> 4;
			}
		     }
		     bmp->line[line][pos] = b[j%4];
		     pos++;
		  }
		  break;
	    }
	 }

	 if (pos-1 > (int)infoheader->biWidth)
	    eolflag=1;
      }

      line--;
      if (line < 0)
	 eopicflag = 1;
   }
}



/* load_bmp:
 *  Loads a Windows BMP file, returning a bitmap structure and storing
 *  the palette data in the specified palette (this should be an array of
 *  at least 256 RGB structures).
 *
 *  Thanks to Seymour Shlien for contributing this function.
 */
BITMAP *load_bmp(AL_CONST char *filename, RGB *pal)
{
   PACKFILE *f;
   BITMAP *bmp;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   bmp = load_bmp_pf(f, pal);

   pack_fclose(f);

   return bmp;
}



/* load_bmp_pf:
 *  Like load_bmp, but starts loading from the current place in the PACKFILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
BITMAP *load_bmp_pf(PACKFILE *f, RGB *pal)
{
   BITMAPFILEHEADER fileheader;
   BITMAPINFOHEADER infoheader;
   BITMAP *bmp;
   PALETTE tmppal;
   int want_palette = TRUE;
   unsigned long biSize;
   int bpp, dest_depth;
   ASSERT(f);

   /* we really need a palette */
   if (!pal) {
      want_palette = FALSE;
      pal = tmppal;
   }

   if (read_bmfileheader(f, &fileheader) != 0) {
      return NULL;
   }

   biSize = pack_igetl(f);

   if (biSize == WININFOHEADERSIZE) {
      if (read_win_bminfoheader(f, &infoheader) != 0) {
	 return NULL;
      }
      if (infoheader.biCompression != BI_BITFIELDS)
	 read_bmicolors(fileheader.bfOffBits - 54, pal, f, 1);
   }
   else if (biSize == OS2INFOHEADERSIZE) {
      if (read_os2_bminfoheader(f, &infoheader) != 0) {
	 return NULL;
      }
      if (infoheader.biCompression != BI_BITFIELDS)
	 read_bmicolors(fileheader.bfOffBits - 26, pal, f, 0);
   }
   else {
      return NULL;
   }

   if (infoheader.biBitCount == 24)
      bpp = 24;
   else if (infoheader.biBitCount == 16)
      bpp = 16;
   else if (infoheader.biBitCount == 32)
      bpp = 32;
   else
      bpp = 8;

   if (infoheader.biCompression == BI_BITFIELDS) {
      unsigned long redMask = pack_igetl(f);
      unsigned long grnMask = pack_igetl(f);
      unsigned long bluMask = pack_igetl(f);

      (void)grnMask;

      if ((bluMask == 0x001f) && (redMask == 0x7C00))
	 bpp = 15;
      else if ((bluMask == 0x001f) && (redMask == 0xF800))
	 bpp = 16;
      else if ((bluMask == 0x0000FF) && (redMask == 0xFF0000))
	 bpp = 32;
      else {
	 /* Unrecognised bit masks/depth, refuse to load. */
	 return NULL;
      }
   }


   dest_depth = _color_load_depth(bpp, FALSE);

   bmp = create_bitmap_ex(bpp, infoheader.biWidth, ABS(infoheader.biHeight));
   if (!bmp) {
      return NULL;
   }

   clear_bitmap(bmp);

   switch (infoheader.biCompression) {

      case BI_RGB:
	 read_image(f, bmp, &infoheader);
	 break;

      case BI_RLE8:
	 read_RLE8_compressed_image(f, bmp, &infoheader);
	 break;

      case BI_RLE4:
	 read_RLE4_compressed_image(f, bmp, &infoheader);
	 break;

      case BI_BITFIELDS:
	 read_bitfields_image(f, bmp, &infoheader);
	 break;

      default:
	 destroy_bitmap(bmp);
	 bmp = NULL;
   }

   if (dest_depth != bpp) {
      /* restore original palette except if it comes from the bitmap */
      if ((bpp != 8) && (!want_palette))
	 pal = NULL;

      if (bmp) {
	 bmp = _fixup_loaded_bitmap(bmp, pal, dest_depth);
      }
   }
   
   /* construct a fake palette if 8-bit mode is not involved */
   if ((bpp != 8) && (dest_depth != 8) && want_palette)
      generate_332_palette(pal);

   return bmp;
}



/* save_bmp:
 *  Writes a bitmap into a BMP file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
int save_bmp(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal) 
{
   PACKFILE *f;
   int ret;
   ASSERT(filename);

   f = pack_fopen(filename, F_WRITE);
   if (!f) 
      return -1;

   ret = save_bmp_pf(f, bmp, pal);

   pack_fclose(f);

   return ret;
}



/* save_bmp_pf:
 *  Like save_bmp but writes into the PACKFILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
int save_bmp_pf(PACKFILE *f, BITMAP *bmp, AL_CONST RGB *pal) 
{
   PALETTE tmppal;
   int bfSize;
   int biSizeImage;
   int depth;
   int bpp;
   int filler;
   int c, i, j;
   ASSERT(f);
   ASSERT(bmp);

   depth = bitmap_color_depth(bmp);
   bpp = (depth == 8) ? 8 : 24;
   filler = 3 - ((bmp->w*(bpp/8)-1) & 3);

   if (!pal) {
      get_palette(tmppal);
      pal = tmppal;
   }

   if (bpp == 8) {
      biSizeImage = (bmp->w + filler) * bmp->h;
      bfSize = (54		       /* header */
		+ 256*4		       /* palette */
		+ biSizeImage);	       /* image data */
   }
   else {
      biSizeImage = (bmp->w*3 + filler) * bmp->h;
      bfSize = 54 + biSizeImage;       /* header + image data */
   }

   *allegro_errno = 0;

   /* file_header */
   pack_iputw(0x4D42, f);              /* bfType ("BM") */
   pack_iputl(bfSize, f);              /* bfSize */
   pack_iputw(0, f);                   /* bfReserved1 */
   pack_iputw(0, f);                   /* bfReserved2 */

   if (bpp == 8)                       /* bfOffBits */
      pack_iputl(54+256*4, f); 
   else
      pack_iputl(54, f); 

   /* info_header */
   pack_iputl(40, f);                  /* biSize */
   pack_iputl(bmp->w, f);              /* biWidth */
   pack_iputl(bmp->h, f);              /* biHeight */
   pack_iputw(1, f);                   /* biPlanes */
   pack_iputw(bpp, f);                 /* biBitCount */
   pack_iputl(0, f);                   /* biCompression */
   pack_iputl(biSizeImage, f);         /* biSizeImage */
   pack_iputl(0xB12, f);               /* biXPelsPerMeter (0xB12 = 72 dpi) */
   pack_iputl(0xB12, f);               /* biYPelsPerMeter */

   if (bpp == 8) {
      pack_iputl(256, f);              /* biClrUsed */
      pack_iputl(256, f);              /* biClrImportant */

      /* palette */
      for (i=0; i<256; i++) {
	 pack_putc(_rgb_scale_6[pal[i].b], f);
	 pack_putc(_rgb_scale_6[pal[i].g], f);
	 pack_putc(_rgb_scale_6[pal[i].r], f);
	 pack_putc(0, f);
      }
   }
   else {
      pack_iputl(0, f);                /* biClrUsed */
      pack_iputl(0, f);                /* biClrImportant */
   }

   /* image data */
   for (i=bmp->h-1; i>=0; i--) {
      for (j=0; j<bmp->w; j++) {
	 if (bpp == 8) {
	    pack_putc(getpixel(bmp, j, i), f);
	 }
	 else {
	    c = getpixel(bmp, j, i);
	    pack_putc(getb_depth(depth, c), f);
	    pack_putc(getg_depth(depth, c), f);
	    pack_putc(getr_depth(depth, c), f);
	 }
      }

      for (j=0; j<filler; j++)
	 pack_putc(0, f);
   }

   if (*allegro_errno)
      return -1;
   else
      return 0;
}

