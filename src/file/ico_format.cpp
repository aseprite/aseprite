/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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
 * ico.c - Based on the code of Elias Pschernig.
 */

#include "config.h"

#include <allegro/color.h>

#include "file/file.h"
#include "raster/raster.h"

static bool load_ICO(FileOp* fop);
static bool save_ICO(FileOp* fop);

FileFormat format_ico =
{
  "ico",
  "ico",
  load_ICO,
  save_ICO,
  NULL,
  FILE_SUPPORT_RGB |
  FILE_SUPPORT_GRAY |
  FILE_SUPPORT_INDEXED
};

struct ICONDIR
{
  ase_uint16 reserved;
  ase_uint16 type;
  ase_uint16 entries;
};

struct ICONDIRENTRY
{
  ase_uint8  width;
  ase_uint8  height;
  ase_uint8  color_count;
  ase_uint8  reserved;
  ase_uint16 planes;
  ase_uint16 bpp;
  ase_uint32 image_size;
  ase_uint32 image_offset;
};

struct BITMAPINFOHEADER
{
  ase_uint32 size;
  ase_uint32 width;
  ase_uint32 height;
  ase_uint16 planes;
  ase_uint16 bpp;
  ase_uint32 compression;
  ase_uint32 imageSize;
  ase_uint32 xPelsPerMeter;
  ase_uint32 yPelsPerMeter;
  ase_uint32 clrUsed;
  ase_uint32 clrImportant;
};

static bool load_ICO(FileOp *fop)
{
  FILE* f = fopen(fop->filename, "rb");
  if (!f)
    return false;

  // Read the icon header
  ICONDIR header;
  header.reserved = fgetw(f);			// Reserved
  header.type     = fgetw(f);			// Resource type: 1=ICON
  header.entries  = fgetw(f);			// Number of icons

  if (header.type != 1) {
    fop_error(fop, _("Invalid ICO file type.\n"));
    fclose(f);
    return false;
  }

  if (header.entries < 1) {
    fop_error(fop, _("This ICO files does not contain images.\n"));
    fclose(f);
    return false;
  }

  // Read all entries
  std::vector<ICONDIRENTRY> entries;
  entries.reserve(header.entries);
  for (ase_uint16 n=0; n<header.entries; ++n) {
    ICONDIRENTRY entry;
    entry.width		 = fgetc(f);	 // width
    entry.height	 = fgetc(f);	 // height
    entry.color_count	 = fgetc(f);	 // color count
    entry.reserved	 = fgetc(f);	 // reserved
    entry.planes	 = fgetw(f);	 // color planes
    entry.bpp		 = fgetw(f);	 // bits per pixel
    entry.image_size	 = fgetl(f);	 // size in bytes of image data
    entry.image_offset	 = fgetl(f);	 // file offset to image data
    entries.push_back(entry);
  }

  // Read the first entry
  const ICONDIRENTRY& entry = entries[0];
  int width = (entry.width == 0 ? 256: entry.width);
  int height = (entry.height == 0 ? 256: entry.height);
  int numcolors = 1 << entry.bpp;
  int imgtype = IMAGE_INDEXED;
  if (entry.bpp > 8)
    imgtype = IMAGE_RGB;

  // Create the sprite with one background layer
  Sprite* sprite = new Sprite(imgtype, width, height, numcolors);
  LayerImage* layer = new LayerImage(sprite);
  sprite->getFolder()->add_layer(layer);

  // Create the first image/cel
  Image* image = image_new(imgtype, width, height);
  int image_index = stock_add_image(sprite->getStock(), image);
  Cel* cel = cel_new(0, image_index);
  layer->add_cel(cel);

  // Go to the entry start in the file
  fseek(f, entry.image_offset, SEEK_SET);

  // Read BITMAPINFOHEADER
  BITMAPINFOHEADER bmpHeader;
  bmpHeader.size		 = fgetl(f);
  bmpHeader.width		 = fgetl(f);
  bmpHeader.height		 = fgetl(f); // XOR height + AND height
  bmpHeader.planes		 = fgetw(f);
  bmpHeader.bpp			 = fgetw(f);
  bmpHeader.compression		 = fgetl(f); // unused in .ico files
  bmpHeader.imageSize		 = fgetl(f);
  bmpHeader.xPelsPerMeter	 = fgetl(f); // unused for ico
  bmpHeader.yPelsPerMeter	 = fgetl(f); // unused for ico
  bmpHeader.clrUsed		 = fgetl(f); // unused for ico
  bmpHeader.clrImportant	 = fgetl(f); // unused for ico

  // Read the palette
  if (entry.bpp <= 8) {
    Palette* pal = new Palette(0, numcolors);

    for (int i=0; i<numcolors; ++i) {
      int b = fgetc(f);
      int g = fgetc(f);
      int r = fgetc(f);
      fgetc(f);

      pal->setEntry(i, _rgba(r, g, b, 255));
    }

    sprite->setPalette(pal, true);
  }

  // Read XOR MASK
  int x, y, c, r, g, b;
  for (y=image->h-1; y>=0; --y) {
    for (x=0; x<image->w; ++x) {
      switch (entry.bpp) {

	case 8:
	  c = fgetc(f);
	  image_putpixel(image, x, y, c);
	  break;

	case 24:
	  b = fgetc(f);
	  g = fgetc(f);
	  r = fgetc(f);
	  image_putpixel(image, x, y, _rgba(r, g, b, 255));
	  break;
      }
    }

    // every scanline must be 32-bit aligned
    while (x & 3) {
      fgetc(f);
      x++;
    } 
  }

  // AND mask
  int m, v;
  for (y=image->h-1; y>=0; --y) {
    for (x=0; x<(image->w+7)/8; ++x) {
      m = fgetc(f);
      v = 128;
      for (b=0; b<8; b++) {
	if ((m & v) == v)
	  image_putpixel(image, x*8+b, y, 0); // TODO mask color
	v >>= 1;
      }
    }

    // every scanline must be 32-bit aligned
    while (x & 3) {
      fgetc(f);
      x++;
    }
  }
  
  // Close the file
  fclose(f);
  
  fop->sprite = sprite;
  return true;
}

static bool save_ICO(FileOp *fop)
{
  Sprite *sprite = fop->sprite;
  int bpp, bw, bitsw;
  int size, offset, n, i;
  int c, x, y, b, m, v;
  int num = sprite->getTotalFrames();

  FILE* f = fopen(fop->filename, "wb");
  if (!f)
    return false;

  offset = 6 + num*16;  // ICONDIR + ICONDIRENTRYs
   
  // Icon directory
  fputw(0, f);			// reserved
  fputw(1, f);			// resource type: 1=ICON
  fputw(num, f);		// number of icons

  // Entries
  for (n=0; n<num; ++n) {
    bpp = (fop->sprite->getImgType() == IMAGE_INDEXED) ? 8 : 24;
    bw = (((sprite->getWidth() * bpp / 8) + 3) / 4) * 4;
    bitsw = ((((sprite->getWidth() + 7) / 8) + 3) / 4) * 4;
    size = sprite->getHeight() * (bw + bitsw) + 40;

    if (bpp == 8)
      size += 256 * 4;

    // ICONDIRENTRY
    fputc(sprite->getWidth(), f);	// width
    fputc(sprite->getHeight(), f);	// height
    fputc(0, f);		// color count
    fputc(0, f);		// reserved
    fputw(1, f);		// color planes
    fputw(bpp, f);		// bits per pixel
    fputl(size, f);		// size in bytes of image data
    fputl(offset, f);		// file offset to image data

    offset += size;
  }

  Image* image = image_new(sprite->getImgType(),
			   sprite->getWidth(),
			   sprite->getHeight());

  for (n=0; n<num; ++n) {
    image_clear(image, 0);
    layer_render(sprite->getFolder(), image, 0, 0, n);

    bpp = (fop->sprite->getImgType() == IMAGE_INDEXED) ? 8 : 24;
    bw = (((image->w * bpp / 8) + 3) / 4) * 4;
    bitsw = ((((image->w + 7) / 8) + 3) / 4) * 4;
    size = image->h * (bw + bitsw) + 40;

    if (bpp == 8)
      size += 256 * 4;

    // BITMAPINFOHEADER
    fputl(40, f);		   // size
    fputl(image->w, f);		   // width
    fputl(image->h * 2, f);	   // XOR height + AND height
    fputw(1, f);		   // planes
    fputw(bpp, f);		   // bitcount
    fputl(0, f);		   // unused for ico
    fputl(size, f);		   // size
    fputl(0, f);		   // unused for ico
    fputl(0, f);		   // unused for ico
    fputl(0, f);		   // unused for ico
    fputl(0, f);		   // unused for ico

    // PALETTE
    if (bpp == 8) {
      Palette *pal = sprite->getPalette(n);

      fputl(0, f);  // color 0 is black, so the XOR mask works

      for (i=1; i<256; i++) {
	fputc(_rgba_getb(pal->getEntry(i)), f);
	fputc(_rgba_getg(pal->getEntry(i)), f);
	fputc(_rgba_getr(pal->getEntry(i)), f);
	fputc(0, f);
      }
    }

    // XOR MASK
    for (y=image->h-1; y>=0; --y) {
      for (x=0; x<image->w; ++x) {
	switch (image->imgtype) {

	  case IMAGE_RGB:
	    c = image_getpixel(image, x, y);
	    fputc(_rgba_getb(c), f);
	    fputc(_rgba_getg(c), f);
	    fputc(_rgba_getr(c), f);
	    break;

	  case IMAGE_GRAYSCALE:
	    c = image_getpixel(image, x, y);
	    fputc(_graya_getv(c), f);
	    fputc(_graya_getv(c), f);
	    fputc(_graya_getv(c), f);
	    break;

	  case IMAGE_INDEXED:
	    c = image_getpixel(image, x, y);
	    fputc(c, f);
	    break;
	}
      }

      // every scanline must be 32-bit aligned
      while (x & 3) {
	fputc(0, f);
	x++;
      } 
    }

    // AND MASK
    for (y=image->h-1; y>=0; --y) {
      for (x=0; x<(image->w+7)/8; ++x) {
	m = 0;
	v = 128;

	for (b=0; b<8; b++) {
	  c = image_getpixel(image, x*8+b, y);

	  switch (image->imgtype) {

	    case IMAGE_RGB:
	      if (_rgba_geta(c) == 0)
		m |= v;
	      break;

	    case IMAGE_GRAYSCALE:
	      if (_graya_geta(c) == 0)
		m |= v;
	      break;

	    case IMAGE_INDEXED:
	      if (c == 0) // TODO configurable background color (or nothing as background)
		m |= v;
	      break;
	  }

	  v >>= 1;
	}

	fputc(m, f);  
      }

	// every scanline must be 32-bit aligned
      while (x & 3) {
	fputc(0, f);
	x++;
      }
    }
  }

  image_free(image);
  fclose(f);

  return true;
}
