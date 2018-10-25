// Aseprite
// Copyright (c) 2018  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/doc.h"

namespace app {

using namespace base;

class SvgFormat : public FileFormat {
  const char* onGetName() const override {
    return "svg";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("svg");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::SVG_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateSvgFormat()
{
  return new SvgFormat;
}

  bool SvgFormat::onLoad(FileOp* fop) { return false;}

#ifdef ENABLE_SAVE

bool SvgFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  int x, y, c, r, g, b, a, alpha;
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();
  auto printcol = [f](int x, int y, int r, int g, int b, int a) {
    fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"1\" height=\"1\" fill=\"#%02X%02X%02X\" ", x, y, r, g, b);
    if (a != 255)
      fprintf(f, "opacity=\"%f\" ", (float)a / 255.0);
    fprintf(f, "/>\n");
  };
  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
  fprintf(f, "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">\n",
          image->width(), image->height());

  switch (image->pixelFormat()) {

    case IMAGE_RGB: {
      for (y=0; y<image->height(); y++) {
        for (x=0; x<image->width(); x++) {
          c = get_pixel_fast<RgbTraits>(image, x, y);
          alpha = rgba_geta(c);
          if (alpha != 0x00)
            printcol(x, y, rgba_getr(c), rgba_getg(c), rgba_getb(c), alpha);
        }
        fop->setProgress((float)y / (float)(image->height()));
      }
      break;
    }
    case IMAGE_GRAYSCALE: {
      for (y=0; y<image->height(); y++) {
        for (x=0; x<image->width(); x++) {
          c = get_pixel_fast<GrayscaleTraits>(image, x, y);
          auto v = graya_getv(c);
          alpha = graya_geta(c);
          if (alpha != 0x00)
            printcol(x, y, v, v, v, alpha);
        }
        fop->setProgress((float)y / (float)(image->height()));
      }
      break;
    }
    case IMAGE_INDEXED: {
      unsigned char image_palette[256][4];
      for (y=0; y<256; y++) {
        fop->sequenceGetColor(y, &r, &g, &b);
        image_palette[y][0] = r;
        image_palette[y][1] = g;
        image_palette[y][2] = b;
        fop->sequenceGetAlpha(y, &a);
        image_palette[y][3] = a;
      }
      color_t mask_color = -1;
      if (fop->document()->sprite()->backgroundLayer() == NULL ||
          !fop->document()->sprite()->backgroundLayer()->isVisible()) {
        mask_color = fop->document()->sprite()->transparentColor();
      }
      for (y=0; y<image->height(); y++) {
        for (x=0; x<image->width(); x++) {
          c = get_pixel_fast<IndexedTraits>(image, x, y);
          if (c != mask_color)
            printcol(x, y, image_palette[c][0] & 0xff,
                     image_palette[c][1] & 0xff,
                     image_palette[c][2] & 0xff,
                     image_palette[c][3] & 0xff);
        }
        fop->setProgress((float)y / (float)(image->height()));
      }
      break;
    }
  }
  fprintf(f, "</svg>");
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
