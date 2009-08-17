/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#ifndef FILE_FORMAT_OPTIONS_H_INCLUDED
#define FILE_FORMAT_OPTIONS_H_INCLUDED

enum {
  FORMAT_OPTIONS_BMP,
  FORMAT_OPTIONS_JPEG,
  FORMAT_OPTIONS_MAX
};

/* data that can be in a file and could be useful to save the file
   later in the same format */
typedef struct FormatOptions
{
  int type;
  int size;
} FormatOptions;

FormatOptions* format_options_new(int type, int size);
void format_options_free(FormatOptions* filedata);

/*********************************************************************
 Data for BMP files
 *********************************************************************/

#define BMP_OPTIONS_FORMAT_WINDOWS		12
#define BMP_OPTIONS_FORMAT_OS2			40

#define BMP_OPTIONS_COMPRESSION_RGB		0
#define BMP_OPTIONS_COMPRESSION_RLE8		1
#define BMP_OPTIONS_COMPRESSION_RLE4		2
#define BMP_OPTIONS_COMPRESSION_BITFIELDS	3

typedef struct BmpOptions
{
  FormatOptions head;
  int format;			/* BMP format */
  int compression;		/* BMP compression */
  int bits_per_pixel;		/* bits per pixel */
  ase_uint32 red_mask;		/* mask for red channel */
  ase_uint32 green_mask;	/* mask for green channel */
  ase_uint32 blue_mask;		/* mask for blue channel */
} BmpOptions;

BmpOptions *bmp_options_new();

/*********************************************************************
 Data for JPEG files
 *********************************************************************/

typedef struct JpegOptions
{
  FormatOptions head;
  float quality;		/* 1.0 maximum quality */
} JpegOptions;

JpegOptions* jpeg_options_new();

#endif
