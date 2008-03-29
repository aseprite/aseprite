/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#ifndef FILEDATA_H
#define FILEDATA_H

enum {
  FILEDATA_BMP,
  FILEDATA_JPEG,
  FILEDATA_MAX
};

/* data that can be in a file and could be useful to save the file
   later in the same format */
typedef struct FileData
{
  int type;
  int size;
} FileData;

FileData *filedata_new(int type, int size);
void filedata_free(FileData *filedata);

/*********************************************************************
 Data for BMP files
 *********************************************************************/

#define BMPDATA_FORMAT_WINDOWS		12
#define BMPDATA_FORMAT_OS2		40

#define BMPDATA_COMPRESSION_RGB		0
#define BMPDATA_COMPRESSION_RLE8	1
#define BMPDATA_COMPRESSION_RLE4	2
#define BMPDATA_COMPRESSION_BITFIELDS	3

typedef struct BmpData
{
  FileData head;
  int format;			/* BMP format */
  int compression;		/* BMP compression */
  int bits_per_pixel;		/* bits per pixel */
  ase_uint32 red_mask;		/* mask for red channel */
  ase_uint32 green_mask;	/* mask for green channel */
  ase_uint32 blue_mask;		/* mask for blue channel */
} BmpData;

BmpData *bmpdata_new(void);

/*********************************************************************
 Data for JPEG files
 *********************************************************************/

#define JPEGDATA_METHOD_SLOW		0 /* slow but accurate integer algorithm */
#define JPEGDATA_METHOD_FAST		1 /* faster, less accurate integer method */
#define JPEGDATA_METHOD_FLOAT		2 /* floating-point: accurate, fast on fast HW */
#define JPEGDATA_METHOD_DEFAULT		JPEGDATA_METHOD_SLOW

typedef struct JpegData
{
  FileData head;
  float quality;		/* 1.0 maximum quality */
  float smooth;			/* 1.0 maximum smooth */
  int method;
} JpegData;

JpegData *jpegdata_new(void);

#endif /* FILEDATA_H */
