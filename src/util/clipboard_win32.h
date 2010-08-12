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
 */

// included by clipboard.cpp

#ifndef LCS_WINDOWS_COLOR_SPACE
#define LCS_WINDOWS_COLOR_SPACE 'Win '
#endif

#ifndef CF_DIBV5
#define CF_DIBV5            17
#endif

static ase_uint32 get_shift_from_mask(ase_uint32 mask)
{
  ase_uint32 shift = 0;
  for (shift=0; shift<32; ++shift)
    if (mask & (1 << shift))
      return shift;
  return shift;
}

/**
 * Returns true if the Windows clipboard contains a bitmap (CF_DIB
 * format).
 */
static bool win32_clipboard_contains_bitmap()
{
  return IsClipboardFormatAvailable(CF_DIB) ? true: false;
}

/**
 * Changes the Windows clipboard content to the specified image. The
 * palette is optional and only used if the image is IMAGE_INDEXED type.
 */
static void set_win32_clipboard_bitmap(Image* image, Palette* palette)
{
  if (!OpenClipboard(win_get_window()))
    return;

  if (!EmptyClipboard()) {
    CloseClipboard();
    return;
  }

  // information to create the memory necessary for the bitmap
  int padding = 0;
  int scanline = 0;
  int color_depth = 0;
  int palette_entries = 0;

  switch (image->imgtype) {
    case IMAGE_RGB:
      scanline = sizeof(ase_uint32) * image->w;
      color_depth = 32;
      break;
    case IMAGE_GRAYSCALE:
       // this is right! Grayscaled is copied as RGBA in Win32 Clipboard
      scanline = sizeof(ase_uint32) * image->w;
      color_depth = 32;
      break;
    case IMAGE_INDEXED:
      padding = (4-(image->w&3))&3;
      scanline = sizeof(ase_uint8) * image->w;
      scanline += padding;
      color_depth = 8;
      palette_entries = palette->size();
      break;
  }
  ASSERT(scanline > 0 && color_depth > 0);

  // create the BITMAPV5HEADER structure
  HGLOBAL hmem = GlobalAlloc(GHND,
			     sizeof(BITMAPV5HEADER)
			     + palette_entries*sizeof(RGBQUAD)
			     + scanline*image->h);
  BITMAPV5HEADER* bi = (BITMAPV5HEADER*)GlobalLock(hmem);

  bi->bV5Size = sizeof(BITMAPV5HEADER);
  bi->bV5Width = image->w;
  bi->bV5Height = image->h;
  bi->bV5Planes = 1;
  bi->bV5BitCount = color_depth;
  bi->bV5Compression = BI_RGB;
  bi->bV5SizeImage = scanline*image->h;
  bi->bV5RedMask   = 0x00ff0000;
  bi->bV5GreenMask = 0x0000ff00;
  bi->bV5BlueMask  = 0x000000ff;
  bi->bV5AlphaMask = 0xff000000;
  bi->bV5CSType = LCS_WINDOWS_COLOR_SPACE;
  bi->bV5Intent = LCS_GM_GRAPHICS;
  bi->bV5ClrUsed = palette_entries == 256 ? 0: palette_entries;

  // write pixels
  switch (image->imgtype) {
    case IMAGE_RGB: {
      ase_uint32* dst = (ase_uint32*)(((ase_uint8*)bi)+bi->bV5Size);
      ase_uint32 c;
      for (int y=image->h-1; y>=0; --y)
	for (int x=0; x<image->w; ++x) {
	  c = image_getpixel_fast<RgbTraits>(image, x, y);
	  *(dst++) = ((_rgba_getb(c) <<  0) |
		      (_rgba_getg(c) <<  8) |
		      (_rgba_getr(c) << 16) |
		      (_rgba_geta(c) << 24));
	}
      break;
    }
    case IMAGE_GRAYSCALE: {
      ase_uint32* dst = (ase_uint32*)(((ase_uint8*)bi)+bi->bV5Size);
      ase_uint16 c;
      for (int y=image->h-1; y>=0; --y)
	for (int x=0; x<image->w; ++x) {
	  c = image_getpixel_fast<GrayscaleTraits>(image, x, y);
	  *(dst++) = ((_graya_getv(c) <<  0) |
		      (_graya_getv(c) <<  8) |
		      (_graya_getv(c) << 16) |
		      (_graya_geta(c) << 24));
	}
      break;
    }
    case IMAGE_INDEXED: {
      Palette* palette = get_current_palette();
      RGBQUAD* rgbquad = (RGBQUAD*)(((ase_uint8*)bi)+bi->bV5Size);
      for (int i=0; i<palette->size(); ++i) {
	rgbquad->rgbRed   = _rgba_getr(palette->getEntry(i));
	rgbquad->rgbGreen = _rgba_getg(palette->getEntry(i));
	rgbquad->rgbBlue  = _rgba_getb(palette->getEntry(i));
	rgbquad++;
      }

      ase_uint8* dst = (ase_uint8*)(((ase_uint8*)bi)+bi->bV5Size
				    + palette_entries*sizeof(RGBQUAD));
      for (int y=image->h-1; y>=0; --y) {
	for (int x=0; x<image->w; ++x) {
	  *(dst++) = image_getpixel_fast<IndexedTraits>(image, x, y);
	}
	dst += padding;
      }
      break;
    }
  }

  GlobalUnlock(hmem);
  SetClipboardData(CF_DIBV5, hmem);
  CloseClipboard();

  GlobalFree(hmem);
}

/**
 * Creates an Image from the current Windows Clipboard content.
 */
static void get_win32_clipboard_bitmap(Image*& image, Palette*& palette)
{
  image = NULL;
  palette = NULL;

  if (!win32_clipboard_contains_bitmap())
    return;

  if (!OpenClipboard(win_get_window()))
    return;

  BITMAPINFO* bi = (BITMAPINFO*)GetClipboardData(CF_DIB);
  if (bi) {
    if (bi->bmiHeader.biCompression != BI_RGB &&
	bi->bmiHeader.biCompression != BI_BITFIELDS) {
      jalert("Error<<The current Windows clipboard format is not a bitmap.||&OK");
      return;
    }

    try {
      image = image_new(bi->bmiHeader.biBitCount == 8 ? IMAGE_INDEXED:
							IMAGE_RGB,
			bi->bmiHeader.biWidth,
			ABS(bi->bmiHeader.biHeight));

      bool valid_image = false;
      switch (bi->bmiHeader.biBitCount) {

	// 32 BPP
	case 32:
	  if (bi->bmiHeader.biCompression == BI_BITFIELDS) {
	    ase_uint32* src = (ase_uint32*)(((ase_uint8*)bi)+bi->bmiHeader.biSize+sizeof(RGBQUAD)*3);
	    ase_uint32 c;

	    ase_uint32 r_mask = (ase_uint32)*((ase_uint32*)&bi->bmiColors[0]);
	    ase_uint32 g_mask = (ase_uint32)*((ase_uint32*)&bi->bmiColors[1]);
	    ase_uint32 b_mask = (ase_uint32)*((ase_uint32*)&bi->bmiColors[2]);
	    ase_uint32 r_shift = get_shift_from_mask(r_mask);
	    ase_uint32 g_shift = get_shift_from_mask(g_mask);
	    ase_uint32 b_shift = get_shift_from_mask(b_mask);

	    for (int y=image->h-1; y>=0; --y) {
	      ase_uint32* dst = (ase_uint32*)image->line[y];

	      for (int x=0; x<image->w; ++x) {
		c = *(src++);
		*(dst++) = _rgba((c & r_mask) >> r_shift,
				 (c & g_mask) >> g_shift,
				 (c & b_mask) >> b_shift, 255);
	      }
	    }
	  }
	  else if (bi->bmiHeader.biCompression == BI_RGB) {
	    ase_uint32* src = (ase_uint32*)(((ase_uint8*)bi)+bi->bmiHeader.biSize);
	    ase_uint32 c;

	    for (int y=image->h-1; y>=0; --y) {
	      ase_uint32* dst = (ase_uint32*)image->line[y];

	      for (int x=0; x<image->w; ++x) {
		c = *(src++);
		*(dst++) = _rgba((c & 0x00ff0000) >> 16,
				 (c & 0x0000ff00) >> 8,
				 (c & 0x000000ff) >> 0,
				 (c & 0xff000000) >> 24);
	      }
	    }
	  }
	  valid_image = true;
	  break;

	// 24 BPP
	case 24: {
	  ase_uint8* src = (((ase_uint8*)bi)+bi->bmiHeader.biSize);
	  ase_uint8 r, g, b;
	  int padding = (4-((image->w*3)&3))&3;

	  for (int y=image->h-1; y>=0; --y) {
	    ase_uint32* dst = (ase_uint32*)image->line[y];

	    for (int x=0; x<image->w; ++x) {
	      b = *(src++);
	      g = *(src++);
	      r = *(src++);
	      *(dst++) = _rgba(r, g, b, 255);
	    }
	    src += padding;
	  }
	  valid_image = true;
	  break;
	}

	// 16 BPP
	case 16: {
	  // TODO I am not sure if this really works
	  ase_uint8* src = (((ase_uint8*)bi)+bi->bmiHeader.biSize);
	  ase_uint8 b1, b2, r, g, b;
	  int padding = (4-((image->w*2)&3))&3;

	  for (int y=image->h-1; y>=0; --y) {
	    for (int x=0; x<image->w; ++x) {
	      b1 = *(src++);
	      b2 = *(src++);
	      b = _rgb_scale_5[((b1 & 0xf800) >> 11)];
	      g = _rgb_scale_6[((b2 & 0x07e0) >> 5)];
	      r = _rgb_scale_5[(b2 & 0x001f)];
	      image_putpixel_fast<RgbTraits>(image, x, y, _rgba(r, g, b, 255));
	    }
	    src += padding;
	  }
	  valid_image = true;
	  break;
	}

	// 8 BPP
	case 8: {
	  int colors = bi->bmiHeader.biClrUsed > 0 ? bi->bmiHeader.biClrUsed: 256;
	  palette = new Palette(0, colors);
	  for (int c=0; c<colors; ++c) {
	    palette->setEntry(c, _rgba(bi->bmiColors[c].rgbRed,
				       bi->bmiColors[c].rgbGreen,
				       bi->bmiColors[c].rgbBlue, 255));
	  }

	  ase_uint8* src = (((ase_uint8*)bi)+bi->bmiHeader.biSize+sizeof(RGBQUAD)*colors);
	  int padding = (4-(image->w&3))&3;

	  for (int y=image->h-1; y>=0; --y) {
	    for (int x=0; x<image->w; ++x) {
	      int c = *(src++);
	      image_putpixel_fast<IndexedTraits>(image, x, y, MID(0, c, colors-1));
	    }

	    src += padding;
	  }

	  valid_image = true;
	  break;
	}

      }

      if (!valid_image) {
	delete image;
	delete palette;
	image = NULL;
	palette = NULL;
      }
    }
    catch (...) {
      delete image;
      delete palette;
      image = NULL;
      palette = NULL;
    }
  }

  CloseClipboard();
}
