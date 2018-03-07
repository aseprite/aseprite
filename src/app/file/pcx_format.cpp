// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//
// pcx.c - Based on the code of Shawn Hargreaves.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/doc.h"

namespace app {

using namespace base;

class PcxFormat : public FileFormat {

  const char* onGetName() const override {
    return "pcx";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("pcx");
    exts.push_back("pcc");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::PCX_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreatePcxFormat()
{
  return new PcxFormat;
}

bool PcxFormat::onLoad(FileOp* fop)
{
  int c, r, g, b;
  int width, height;
  int bpp, bytes_per_line;
  int xx, po;
  int x, y;
  char ch = 0;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();

  fgetc(f);                    /* skip manufacturer ID */
  fgetc(f);                    /* skip version flag */
  fgetc(f);                    /* skip encoding flag */

  if (fgetc(f) != 8) {         /* we like 8 bit color planes */
    fop->setError("This PCX doesn't have 8 bit color planes.\n");
    return false;
  }

  width = -(fgetw(f));          /* xmin */
  height = -(fgetw(f));         /* ymin */
  width += fgetw(f) + 1;        /* xmax */
  height += fgetw(f) + 1;       /* ymax */

  fgetl(f);                     /* skip DPI values */

  for (c=0; c<16; c++) {        /* read the 16 color palette */
    r = fgetc(f);
    g = fgetc(f);
    b = fgetc(f);
    fop->sequenceSetColor(c, r, g, b);
  }

  fgetc(f);

  bpp = fgetc(f) * 8;          /* how many color planes? */
  if ((bpp != 8) && (bpp != 24)) {
    return false;
  }

  bytes_per_line = fgetw(f);

  for (c=0; c<60; c++)             /* skip some more junk */
    fgetc(f);

  Image* image = fop->sequenceImage(bpp == 8 ?
                                    IMAGE_INDEXED:
                                    IMAGE_RGB,
                                    width, height);
  if (!image) {
    return false;
  }

  if (bpp == 24)
    clear_image(image, rgba(0, 0, 0, 255));

  for (y=0; y<height; y++) {       /* read RLE encoded PCX data */
    x = xx = 0;
    po = rgba_r_shift;

    while (x < bytes_per_line*bpp/8) {
      ch = fgetc(f);
      if ((ch & 0xC0) == 0xC0) {
        c = (ch & 0x3F);
        ch = fgetc(f);
      }
      else
        c = 1;

      if (bpp == 8) {
        while (c--) {
          if (x < image->width())
            put_pixel_fast<IndexedTraits>(image, x, y, ch);

          x++;
        }
      }
      else {
        while (c--) {
          if (xx < image->width())
            put_pixel_fast<RgbTraits>(image, xx, y,
                                      get_pixel_fast<RgbTraits>(image, xx, y) | ((ch & 0xff) << po));

          x++;
          if (x == bytes_per_line) {
            xx = 0;
            po = rgba_g_shift;
          }
          else if (x == bytes_per_line*2) {
            xx = 0;
            po = rgba_b_shift;
          }
          else
            xx++;
        }
      }
    }

    fop->setProgress((float)(y+1) / (float)(height));
    if (fop->isStop())
      break;
  }

  if (!fop->isStop()) {
    if (bpp == 8) {                  /* look for a 256 color palette */
      while ((c = fgetc(f)) != EOF) {
        if (c == 12) {
          for (c=0; c<256; c++) {
            r = fgetc(f);
            g = fgetc(f);
            b = fgetc(f);
            fop->sequenceSetColor(c, r, g, b);
          }
          break;
        }
      }
    }
  }

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE
bool PcxFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  int c, r, g, b;
  int x, y;
  int runcount;
  int depth, planes;
  char runchar;
  char ch = 0;

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  if (image->pixelFormat() == IMAGE_RGB) {
    depth = 24;
    planes = 3;
  }
  else {
    depth = 8;
    planes = 1;
  }

  fputc(10, f);                      /* manufacturer */
  fputc(5, f);                       /* version */
  fputc(1, f);                       /* run length encoding  */
  fputc(8, f);                       /* 8 bits per pixel */
  fputw(0, f);                       /* xmin */
  fputw(0, f);                       /* ymin */
  fputw(image->width()-1, f);     /* xmax */
  fputw(image->height()-1, f);    /* ymax */
  fputw(320, f);                     /* HDpi */
  fputw(200, f);                     /* VDpi */

  for (c=0; c<16; c++) {
    fop->sequenceGetColor(c, &r, &g, &b);
    fputc(r, f);
    fputc(g, f);
    fputc(b, f);
  }

  fputc(0, f);                      /* reserved */
  fputc(planes, f);                 /* one or three color planes */
  fputw(image->width(), f);      /* number of bytes per scanline */
  fputw(1, f);                      /* color palette */
  fputw(image->width(), f);      /* hscreen size */
  fputw(image->height(), f);     /* vscreen size */
  for (c=0; c<54; c++)              /* filler */
    fputc(0, f);

  for (y=0; y<image->height(); y++) {           /* for each scanline... */
    runcount = 0;
    runchar = 0;
    for (x=0; x<image->width()*planes; x++) {  /* for each pixel... */
      if (depth == 8) {
        if (image->pixelFormat() == IMAGE_INDEXED)
          ch = get_pixel_fast<IndexedTraits>(image, x, y);
        else if (image->pixelFormat() == IMAGE_GRAYSCALE) {
          c = get_pixel_fast<GrayscaleTraits>(image, x, y);
          ch = graya_getv(c);
        }
      }
      else {
        if (x < image->width()) {
          c = get_pixel_fast<RgbTraits>(image, x, y);
          ch = rgba_getr(c);
        }
        else if (x<image->width()*2) {
          c = get_pixel_fast<RgbTraits>(image, x-image->width(), y);
          ch = rgba_getg(c);
        }
        else {
          c = get_pixel_fast<RgbTraits>(image, x-image->width()*2, y);
          ch = rgba_getb(c);
        }
      }
      if (runcount == 0) {
        runcount = 1;
        runchar = ch;
      }
      else {
        if ((ch != runchar) || (runcount >= 0x3f)) {
          if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
            fputc(0xC0 | runcount, f);
          fputc(runchar, f);
          runcount = 1;
          runchar = ch;
        }
        else
          runcount++;
      }
    }

    if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
      fputc(0xC0 | runcount, f);

    fputc(runchar, f);

    fop->setProgress((float)(y+1) / (float)(image->height()));
  }

  if (depth == 8) {                      /* 256 color palette */
    fputc(12, f);

    for (c=0; c<256; c++) {
      fop->sequenceGetColor(c, &r, &g, &b);
      fputc(r, f);
      fputc(g, f);
      fputc(b, f);
    }
  }

  if (ferror(f)) {
    fop->setError("Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}
#endif

} // namespace app
