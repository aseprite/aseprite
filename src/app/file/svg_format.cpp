// Aseprite
// Copyright (c) 2018-2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/pref/preferences.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "ui/window.h"

#include "svg_options.xml.h"

namespace app {

using namespace base;

class SvgFormat : public FileFormat {
  // Data for SVG files
  class SvgOptions : public FormatOptions {
  public:
    SvgOptions() : pixelScale(1) { }
    int pixelScale;
  };
  
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
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS |
      FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  base::SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) override;
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
  const base::SharedPtr<SvgOptions> svg_options = fop->formatOptions();
  const int pixelScaleValue = MID(0, svg_options->pixelScale, 10000);
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();
  auto printcol = [f](int x, int y,int r, int g, int b, int a, int pxScale) {
    fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"#%02X%02X%02X\" ",
            x*pxScale, y*pxScale, pxScale, pxScale, r, g, b);
    if (a != 255)
      fprintf(f, "opacity=\"%f\" ", (float)a / 255.0);
    fprintf(f, "/>\n");
  };
  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
  fprintf(f, "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">\n",
          image->width()*pixelScaleValue, image->height()*pixelScaleValue);

  switch (image->pixelFormat()) {

    case IMAGE_RGB: {
      for (y=0; y<image->height(); y++) {
        for (x=0; x<image->width(); x++) {
          c = get_pixel_fast<RgbTraits>(image, x, y);
          alpha = rgba_geta(c);
          if (alpha != 0x00)
            printcol(x, y, rgba_getr(c), rgba_getg(c), rgba_getb(c), alpha, pixelScaleValue);
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
            printcol(x, y, v, v, v, alpha, pixelScaleValue);
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
                     image_palette[c][3] & 0xff,
                     pixelScaleValue);
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

// Shows the SVG configuration dialog.
base::SharedPtr<FormatOptions> SvgFormat::onGetFormatOptions(FileOp* fop)
{
  base::SharedPtr<SvgOptions> svg_options;
  if (fop->document()->getFormatOptions())
    svg_options = base::SharedPtr<SvgOptions>(fop->document()->getFormatOptions());
  
  if (!svg_options)
    svg_options.reset(new SvgOptions);
  
#ifdef ENABLE_UI
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();
      
      if (pref.isSet(pref.svg.pixelScale))
        svg_options->pixelScale = pref.svg.pixelScale();
      
     if (pref.svg.showAlert()) {
        app::gen::SvgOptions win;
        win.pxsc()->setTextf("%d", svg_options->pixelScale);
        win.openWindowInForeground();
      
        if (win.closer() == win.ok()) {
          pref.svg.pixelScale((int)win.pxsc()->textInt());
          pref.svg.showAlert(!win.dontShow()->isSelected());
          
          svg_options->pixelScale = pref.svg.pixelScale();
        }
        else {
          svg_options.reset(nullptr);
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return base::SharedPtr<SvgOptions>(nullptr);
    }
  }
#endif
  return svg_options;
}

} // namespace app
