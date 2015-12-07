// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// included by clipboard.cpp

#include "base/serialization.h"
#include "doc/color_scales.h"
#include "doc/image_io.h"
#include "doc/mask_io.h"
#include "doc/palette_io.h"
#include "she/display.h"
#include "she/system.h"
#include "ui/alert.h"

#ifndef LCS_WINDOWS_COLOR_SPACE
#define LCS_WINDOWS_COLOR_SPACE 'Win '
#endif

#ifndef CF_DIBV5
#define CF_DIBV5            17
#endif

namespace {

using namespace doc;
using namespace base::serialization;
using namespace base::serialization::little_endian;

static UINT custom_clipboard_format = 0;

static uint32_t get_shift_from_mask(uint32_t mask)
{
  uint32_t shift = 0;
  for (shift=0; shift<32; ++shift)
    if (mask & (1 << shift))
      return shift;
  return shift;
}

bool win32_open_clipboard(HWND hwnd)
{
  if (!custom_clipboard_format)
    custom_clipboard_format = RegisterClipboardFormat(L"Aseprite.Image.1");

  for (int i=0; i<5; ++i) {
    if (OpenClipboard(hwnd))
      return true;

    Sleep(100);
  }
  return false;
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
static void set_win32_clipboard_bitmap(const Image* image, const Mask* mask, Palette* palette)
{
  HWND hwnd = static_cast<HWND>(she::instance()->defaultDisplay()->nativeHandle());
  if (!win32_open_clipboard(hwnd))
    return;

  if (!EmptyClipboard()) {
    CloseClipboard();
    return;
  }

  if (!image) {
    CloseClipboard();
    return;
  }

  // Set custom clipboard formats
  {
    std::stringstream os;
    write32(os,
            (image   ? 1: 0) |
            (mask    ? 2: 0) |
            (palette ? 4: 0));
    if (image) doc::write_image(os, image);
    if (mask) doc::write_mask(os, mask);
    if (palette) doc::write_palette(os, palette);

    if (os.good()) {
      size_t size = (size_t)os.tellp();
      HGLOBAL hmem = GlobalAlloc(GHND, size+4);
      if (hmem) {
        char* p = (char*)GlobalLock(hmem);
        if (p) {
          *((uint32_t*)p) = size;
          os.seekp(0);
          os.read(p+4, size);
        }
        GlobalUnlock(hmem);
        if (p)
          SetClipboardData(custom_clipboard_format, hmem);
        GlobalFree(hmem);
      }
    }
  }

  // information to create the memory necessary for the bitmap
  int padding = 0;
  int scanline = 0;
  int color_depth = 0;
  int palette_entries = 0;

  switch (image->pixelFormat()) {
    case IMAGE_RGB:
      scanline = sizeof(uint32_t) * image->width();
      color_depth = 32;
      break;
    case IMAGE_GRAYSCALE:
       // this is right! Grayscaled is copied as RGBA in Win32 Clipboard
      scanline = sizeof(uint32_t) * image->width();
      color_depth = 32;
      break;
    case IMAGE_INDEXED:
      ASSERT(palette);
      padding = (4-(image->width()&3))&3;
      scanline = sizeof(uint8_t) * image->width();
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
                             + scanline*image->height());
  if (!hmem) {
    // TODO cannot copy exception
    CloseClipboard();
    return;
  }

  BITMAPV5HEADER* bi = (BITMAPV5HEADER*)GlobalLock(hmem);

  bi->bV5Size = sizeof(BITMAPV5HEADER);
  bi->bV5Width = image->width();
  bi->bV5Height = image->height();
  bi->bV5Planes = 1;
  bi->bV5BitCount = color_depth;
  bi->bV5Compression = BI_RGB;
  bi->bV5SizeImage = scanline*image->height();
  bi->bV5RedMask   = 0x00ff0000;
  bi->bV5GreenMask = 0x0000ff00;
  bi->bV5BlueMask  = 0x000000ff;
  bi->bV5AlphaMask = 0xff000000;
  bi->bV5CSType = LCS_WINDOWS_COLOR_SPACE;
  bi->bV5Intent = LCS_GM_GRAPHICS;
  bi->bV5ClrUsed = palette_entries == 256 ? 0: palette_entries;

  // write pixels
  switch (image->pixelFormat()) {
    case IMAGE_RGB: {
      uint32_t* dst = (uint32_t*)(((uint8_t*)bi)+bi->bV5Size);
      uint32_t c;
      for (int y=image->height()-1; y>=0; --y)
        for (int x=0; x<image->width(); ++x) {
          c = get_pixel_fast<RgbTraits>(image, x, y);
          *(dst++) = ((rgba_getb(c) <<  0) |
                      (rgba_getg(c) <<  8) |
                      (rgba_getr(c) << 16) |
                      (rgba_geta(c) << 24));
        }
      break;
    }
    case IMAGE_GRAYSCALE: {
      uint32_t* dst = (uint32_t*)(((uint8_t*)bi)+bi->bV5Size);
      uint16_t c;
      for (int y=image->height()-1; y>=0; --y)
        for (int x=0; x<image->width(); ++x) {
          c = get_pixel_fast<GrayscaleTraits>(image, x, y);
          *(dst++) = ((graya_getv(c) <<  0) |
                      (graya_getv(c) <<  8) |
                      (graya_getv(c) << 16) |
                      (graya_geta(c) << 24));
        }
      break;
    }
    case IMAGE_INDEXED: {
      ASSERT(palette);
      RGBQUAD* rgbquad = (RGBQUAD*)(((uint8_t*)bi)+bi->bV5Size);
      for (int i=0; i<palette->size(); ++i) {
        rgbquad->rgbRed   = rgba_getr(palette->getEntry(i));
        rgbquad->rgbGreen = rgba_getg(palette->getEntry(i));
        rgbquad->rgbBlue  = rgba_getb(palette->getEntry(i));
        rgbquad++;
      }

      uint8_t* dst = (uint8_t*)(((uint8_t*)bi)+bi->bV5Size
                                    + palette_entries*sizeof(RGBQUAD));
      for (int y=image->height()-1; y>=0; --y) {
        for (int x=0; x<image->width(); ++x) {
          *(dst++) = get_pixel_fast<IndexedTraits>(image, x, y);
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
static void get_win32_clipboard_bitmap(Image*& image, Mask*& mask, Palette*& palette)
{
  image = nullptr;
  mask = nullptr;
  palette = nullptr;

  if (!win32_clipboard_contains_bitmap())
    return;

  HWND hwnd = static_cast<HWND>(she::instance()->defaultDisplay()->nativeHandle());
  if (!win32_open_clipboard(hwnd))
    return;

  // Prefer the custom format (to avoid losing data)
  if (IsClipboardFormatAvailable(custom_clipboard_format)) {
    const char* ptr = (const char*)GetClipboardData(custom_clipboard_format);
    if (ptr) {
      size_t size = *((uint32_t*)ptr);
      if (size > 0) {
        std::stringstream is;
        is.write(ptr+4, size);

        int bits = read32(is);
        if (bits & 1) image   = doc::read_image(is, false);
        if (bits & 2) mask    = doc::read_mask(is);
        if (bits & 4) palette = doc::read_palette(is);
      }

      CloseClipboard();
      if (image)
        return;
    }
  }

  BITMAPINFO* bi = (BITMAPINFO*)GetClipboardData(CF_DIB);
  if (bi) {
    if (bi->bmiHeader.biCompression != BI_RGB &&
        bi->bmiHeader.biCompression != BI_BITFIELDS) {
      ui::Alert::show("Error<<The current Windows clipboard format is not a bitmap.||&OK");
      return;
    }

    try {
      image = Image::create(bi->bmiHeader.biBitCount == 8 ? IMAGE_INDEXED:
                                                            IMAGE_RGB,
                            bi->bmiHeader.biWidth,
                            ABS(bi->bmiHeader.biHeight));

      bool valid_image = false;
      switch (bi->bmiHeader.biBitCount) {

        // 32 BPP
        case 32:
          if (bi->bmiHeader.biCompression == BI_BITFIELDS) {
            uint32_t* src = (uint32_t*)(((uint8_t*)bi)+bi->bmiHeader.biSize+sizeof(RGBQUAD)*3);
            uint32_t c;

            uint32_t r_mask = (uint32_t)*((uint32_t*)&bi->bmiColors[0]);
            uint32_t g_mask = (uint32_t)*((uint32_t*)&bi->bmiColors[1]);
            uint32_t b_mask = (uint32_t)*((uint32_t*)&bi->bmiColors[2]);
            uint32_t r_shift = get_shift_from_mask(r_mask);
            uint32_t g_shift = get_shift_from_mask(g_mask);
            uint32_t b_shift = get_shift_from_mask(b_mask);

            for (int y=image->height()-1; y>=0; --y) {
              uint32_t* dst = (uint32_t*)image->getPixelAddress(0, y);

              for (int x=0; x<image->width(); ++x) {
                c = *(src++);
                *(dst++) = rgba((c & r_mask) >> r_shift,
                                (c & g_mask) >> g_shift,
                                (c & b_mask) >> b_shift, 255);
              }
            }
          }
          else if (bi->bmiHeader.biCompression == BI_RGB) {
            uint32_t* src = (uint32_t*)(((uint8_t*)bi)+bi->bmiHeader.biSize);
            uint32_t c;

            for (int y=image->height()-1; y>=0; --y) {
              uint32_t* dst = (uint32_t*)image->getPixelAddress(0, y);

              for (int x=0; x<image->width(); ++x) {
                c = *(src++);
                *(dst++) = rgba((c & 0x00ff0000) >> 16,
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
          uint8_t* src = (((uint8_t*)bi)+bi->bmiHeader.biSize);
          uint8_t r, g, b;
          int padding = (4-((image->width()*3)&3))&3;

          for (int y=image->height()-1; y>=0; --y) {
            uint32_t* dst = (uint32_t*)image->getPixelAddress(0, y);

            for (int x=0; x<image->width(); ++x) {
              b = *(src++);
              g = *(src++);
              r = *(src++);
              *(dst++) = rgba(r, g, b, 255);
            }
            src += padding;
          }
          valid_image = true;
          break;
        }

        // 16 BPP
        case 16: {
          uint16_t* src = (uint16_t*)(((uint8_t*)bi)+bi->bmiHeader.biSize+sizeof(RGBQUAD)*3);
          uint16_t word;
          uint8_t r, g, b;
          int padding = ((4-((image->width()*2)&3))&3)/2;

          uint32_t r_mask = (uint32_t)*((uint32_t*)&bi->bmiColors[0]);
          uint32_t g_mask = (uint32_t)*((uint32_t*)&bi->bmiColors[1]);
          uint32_t b_mask = (uint32_t)*((uint32_t*)&bi->bmiColors[2]);
          uint32_t r_shift = get_shift_from_mask(r_mask);
          uint32_t g_shift = get_shift_from_mask(g_mask);
          uint32_t b_shift = get_shift_from_mask(b_mask);

          for (int y=image->height()-1; y>=0; --y) {
            for (int x=0; x<image->width(); ++x) {
              word = *(src++);
              r = scale_5bits_to_8bits((word & r_mask) >> r_shift);
              g = scale_6bits_to_8bits((word & g_mask) >> g_shift);
              b = scale_5bits_to_8bits((word & b_mask) >> b_shift);
              put_pixel_fast<RgbTraits>(image, x, y, rgba(r, g, b, 255));
            }
            src += padding;
          }
          valid_image = true;
          break;
        }

        // 8 BPP
        case 8: {
          int colors = bi->bmiHeader.biClrUsed > 0 ? bi->bmiHeader.biClrUsed: 256;
          palette = new Palette(frame_t(0), colors);
          for (int c=0; c<colors; ++c) {
            palette->setEntry(c, rgba(bi->bmiColors[c].rgbRed,
                                       bi->bmiColors[c].rgbGreen,
                                       bi->bmiColors[c].rgbBlue, 255));
          }

          uint8_t* src = (((uint8_t*)bi)+bi->bmiHeader.biSize+sizeof(RGBQUAD)*colors);
          int padding = (4-(image->width()&3))&3;

          for (int y=image->height()-1; y>=0; --y) {
            for (int x=0; x<image->width(); ++x) {
              int c = *(src++);
              put_pixel_fast<IndexedTraits>(image, x, y, MID(0, c, colors-1));
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

static bool get_win32_clipboard_bitmap_size(gfx::Size& size)
{
  bool result = false;

  HWND hwnd = static_cast<HWND>(she::instance()->defaultDisplay()->nativeHandle());

  if (win32_clipboard_contains_bitmap() && win32_open_clipboard(hwnd)) {
    BITMAPINFO* bi = (BITMAPINFO*)GetClipboardData(CF_DIB);
    if (bi) {
      size.w = bi->bmiHeader.biWidth;
      size.h = ABS(bi->bmiHeader.biHeight);
      result = true;
    }
    CloseClipboard();
  }
  return result;
}

}
