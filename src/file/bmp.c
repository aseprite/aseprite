/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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
 *
 * bmp.c - Based on the code of Seymour Shlien and Jonas Petersen.
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/color.h>
#include <allegro/file.h>

#include "console/console.h"
#include "file/file.h"
#include "raster/raster.h"

#endif

static Sprite *load_BMP (const char *filename);
static int save_BMP (Sprite *sprite);

FileType filetype_bmp =
{
  "bmp",
  "bmp",
  load_BMP,
  save_BMP,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_INDEXED |
  FILE_SUPPORT_SEQUENCES
};

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
   unsigned long  biHeight;
   unsigned short biBitCount;
   unsigned long  biCompression;
} BITMAPINFOHEADER;

typedef struct WINBMPINFOHEADER  /* size: 40 */
{
   unsigned long  biWidth;
   unsigned long  biHeight;
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
static int read_bmfileheader (PACKFILE *f, BITMAPFILEHEADER *fileheader)
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
static int read_win_bminfoheader (PACKFILE *f, BITMAPINFOHEADER *infoheader)
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
static int read_os2_bminfoheader (PACKFILE *f, BITMAPINFOHEADER *infoheader)
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
static void read_bmicolors (int ncols, PACKFILE *f,int win_flag)
{
   int i, r, g, b;

   for (i=0; i<ncols; i++) {
      b = pack_getc(f) / 4;
      g = pack_getc(f) / 4;
      r = pack_getc(f) / 4;
      file_sequence_set_color(i, r, g, b);
      if (win_flag)
	 pack_getc(f);
   }
}

/* read_1bit_line:
 *  Support function for reading the 1 bit bitmap file format.
 */
static void read_1bit_line (int length, PACKFILE *f, Image *image, int line)
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
      image_putpixel (image, i, line, pix);
   }
}

/* read_4bit_line:
 *  Support function for reading the 4 bit bitmap file format.
 */
static void read_4bit_line (int length, PACKFILE *f, Image *image, int line)
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
      image_putpixel (image, i, line, pix);
   }
}

/* read_8bit_line:
 *  Support function for reading the 8 bit bitmap file format.
 */
static void read_8bit_line (int length, PACKFILE *f, Image *image, int line)
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
      image_putpixel (image, i, line, pix);
   }
}

/* read_24bit_line:
 *  Support function for reading the 24 bit bitmap file format, doing
 *  our best to convert it down to a 256 color palette.
 */
static void read_24bit_line (int length, PACKFILE *f, Image *image, int line)
{
   int i, nbytes;
   RGB c;

   nbytes=0;

   for (i=0; i<length; i++) {
      c.b = pack_getc(f);
      c.g = pack_getc(f);
      c.r = pack_getc(f);
      image_putpixel (image, i, line, _rgba (c.r, c.g, c.b, 255));
      nbytes += 3;
   }

   nbytes = nbytes % 4;
   if (nbytes != 0)
      for (i=nbytes; i<4; i++)
	 pack_getc(f);
}

/* read_image:
 *  For reading the noncompressed BMP image format.
 */
static void read_image (PACKFILE *f, Image *image, AL_CONST BITMAPINFOHEADER *infoheader)
{
   int i, line;

   for (i=0; i<(int)infoheader->biHeight; i++) {
      line = i;

      switch (infoheader->biBitCount) {

	 case 1:
	    read_1bit_line(infoheader->biWidth, f, image, infoheader->biHeight-i-1);
	    break;

	 case 4:
	    read_4bit_line(infoheader->biWidth, f, image, infoheader->biHeight-i-1);
	    break;

	 case 8:
	    read_8bit_line(infoheader->biWidth, f, image, infoheader->biHeight-i-1);
	    break;

	 case 24:
	    read_24bit_line(infoheader->biWidth, f, image, infoheader->biHeight-i-1);
	    break;
      }

      if (infoheader->biHeight > 1)
	do_progress(100 * i / (infoheader->biHeight-1));
   }
}

/* read_RLE8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 */
static void read_RLE8_compressed_image (PACKFILE *f, Image *image, AL_CONST BITMAPINFOHEADER *infoheader)
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
               image_putpixel (image, pos, line, val);
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
                     image_putpixel (image, pos, line, val0);
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
static void read_RLE4_compressed_image (PACKFILE *f, Image *image, AL_CONST BITMAPINFOHEADER *infoheader)
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
	       image_putpixel (image, pos, line, b[j%2]);
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
		     image_putpixel (image, pos, line, b[j%4]);
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

static void read_bitfields_image (PACKFILE *f, Image *image, int bpp, BITMAPINFOHEADER *infoheader)
{
  int k, i;
  int bytesPerPixel;
  int red, grn, blu;
  unsigned long buffer;

  if (bpp == 15)
    bytesPerPixel = 2;
  else
    bytesPerPixel = bpp / 8;

  for (i=0; i<(int)infoheader->biHeight; i++) {
    for (k=0; k<(int)infoheader->biWidth; k++) {
      pack_fread (&buffer, bytesPerPixel, f);

      if (bpp == 15) {
	red = (buffer >> 10) & 0x1f;
	grn = (buffer >> 5) & 0x1f;
	blu = (buffer) & 0x1f;
	buffer = _rgba (_rgb_scale_5[red],
			_rgb_scale_5[grn],
			_rgb_scale_5[blu], 255);
      }
      else if (bpp == 16) {
	red = (buffer >> 11) & 0x1f;
	grn = (buffer >> 5) & 0x3f;
	blu = (buffer) & 0x1f;
	buffer = _rgba (_rgb_scale_5[red],
			_rgb_scale_6[grn],
			_rgb_scale_5[blu], 255);
      }
      else {
	red = (buffer >> 16) & 0xff;
	grn = (buffer >> 8) & 0xff;
	blu = (buffer) & 0xff;
	buffer = _rgba (red, grn, blu, 255);
      }

      image->method->putpixel (image,
			       k, (infoheader->biHeight - i) - 1, buffer);
    }
  }
}

static Sprite *load_BMP (const char *filename)
{
  BITMAPFILEHEADER fileheader;
  BITMAPINFOHEADER infoheader;
  Image *image;
  Sprite *sprite;
  PACKFILE *f;
  int ncol;
  unsigned long biSize;
  int type, bpp = 0;

  f = pack_fopen(filename, F_READ);
  if (!f) {
    if (!file_sequence_sprite())
      console_printf(_("Error opening file.\n"));
    return NULL;
  }

  if (read_bmfileheader(f, &fileheader) != 0) {
    pack_fclose(f);
    return NULL;
  }
 
  biSize = pack_igetl(f);
 
  if (biSize == WININFOHEADERSIZE) {
    if (read_win_bminfoheader(f, &infoheader) != 0) {
      pack_fclose(f);
      return NULL;
    }
    /* compute number of colors recorded */
    ncol = (fileheader.bfOffBits - 54) / 4;

    if (infoheader.biCompression != BI_BITFIELDS)
      read_bmicolors(ncol, f, 1);
  }
  else if (biSize == OS2INFOHEADERSIZE) {
    if (read_os2_bminfoheader(f, &infoheader) != 0) {
      pack_fclose(f);
      return NULL;
    }
    /* compute number of colors recorded */
    ncol = (fileheader.bfOffBits - 26) / 3;

    if (infoheader.biCompression != BI_BITFIELDS)
      read_bmicolors(ncol, f, 0);
  }
  else {
    pack_fclose(f);
    return NULL;
  }

  if ((infoheader.biBitCount == 24) || (infoheader.biBitCount == 16))
    type = IMAGE_RGB;
  else
    type = IMAGE_INDEXED;

  if (infoheader.biCompression == BI_BITFIELDS) {
    unsigned long redMask, bluMask;

    redMask = pack_igetl (f);
    pack_igetl (f);
    bluMask = pack_igetl (f);

    if ((bluMask == 0x001f) && (redMask == 0x7C00))
      bpp = 15;
    else if ((bluMask == 0x001f) && (redMask == 0xF800))
      bpp = 16;
    else if ((bluMask == 0x0000FF) && (redMask == 0xFF0000))
      bpp = 32;
    else {
      /* Unrecognised bit masks/depth */
      pack_fclose (f);
      return NULL;
    }
  }

  image = file_sequence_image (type,
			       infoheader.biWidth,
			       infoheader.biHeight);
  if (!image) {
    pack_fclose(f);
    return NULL;
  }

  if (type == IMAGE_RGB)
    image_clear (image, _rgba (0, 0, 0, 255));
  else
    image_clear (image, 0);

  sprite = file_sequence_sprite ();

  switch (infoheader.biCompression) {
 
    case BI_RGB:
      read_image (f, image, &infoheader);
      break;
 
    case BI_RLE8:
      read_RLE8_compressed_image (f, image, &infoheader);
      break;
 
    case BI_RLE4:
      read_RLE4_compressed_image (f, image, &infoheader);
      break;

    case BI_BITFIELDS:
      read_bitfields_image (f, image, bpp, &infoheader);
      break;

    default:
      sprite = NULL;
  }

  pack_fclose(f);
  return sprite;
}

static int save_BMP (Sprite *sprite)
{
  Image *image;
  PACKFILE *f;
  int bfSize;
  int biSizeImage;
  int bpp = (sprite->imgtype == IMAGE_RGB) ? 24 : 8;
  int filler = 3 - ((sprite->w*(bpp/8)-1) & 3);
  int c, i, j, r, g, b;

  if (bpp == 8) {
    biSizeImage = (sprite->w + filler) * sprite->h;
    bfSize = (54                      /* header */
	      + 256*4                 /* palette */
	      + biSizeImage);         /* image data */
  }
  else {
    biSizeImage = (sprite->w*3 + filler) * sprite->h;
    bfSize = 54 + biSizeImage;       /* header + image data */
  }

  f = pack_fopen (sprite->filename, F_WRITE);
  if (!f) {
    console_printf (_("Error creating file.\n"));
    return -1;
  }

  image = file_sequence_image_to_save ();

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
  pack_iputl(image->w, f);            /* biWidth */
  pack_iputl(image->h, f);            /* biHeight */
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
      file_sequence_get_color(i, &r, &g, &b);
      pack_putc(_rgb_scale_6[b], f);
      pack_putc(_rgb_scale_6[g], f);
      pack_putc(_rgb_scale_6[r], f);
      pack_putc(0, f);
    }
  }
  else {
    pack_iputl (0, f);                /* biClrUsed */
    pack_iputl (0, f);                /* biClrImportant */
  }

  /* image data */
  for (i=image->h-1; i>=0; i--) {
    for (j=0; j<image->w; j++) {
      if (bpp == 8) {
	if (image->imgtype == IMAGE_INDEXED)
	  pack_putc (image->method->getpixel (image, j, i), f);
	else if (image->imgtype == IMAGE_GRAYSCALE)
	  pack_putc (_graya_getk (image->method->getpixel (image, j, i)), f);
      }
      else {
        c = image->method->getpixel (image, j, i);
        pack_putc (_rgba_getb (c), f);
        pack_putc (_rgba_getg (c), f);
        pack_putc (_rgba_getr (c), f);
      }
    }

    for (j=0; j<filler; j++)
      pack_putc (0, f);

    if (image->h > 1)
      do_progress (100 * (image->h-1-i) / (image->h-1));
  }

  pack_fclose (f);

  if (*allegro_errno) {
    console_printf (_("Error writing bytes.\n"));
    return -1;
  }
  else
    return 0;
}
