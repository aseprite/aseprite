// Aseprite
// Copyright (c) 2018-2023  Igara Studio S.A.
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
    SvgOptions() : pixelScale(1) {}
    int pixelScale;
  };

  const char* onGetName() const override { return "svg"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("svg"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::SVG_IMAGE; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_SAVE | FILE_SUPPORT_RGB | FILE_SUPPORT_RGBA | FILE_SUPPORT_GRAY |
           FILE_SUPPORT_GRAYA | FILE_SUPPORT_INDEXED | FILE_SUPPORT_SEQUENCES |
           FILE_SUPPORT_GET_FORMAT_OPTIONS | FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

FileFormat* CreateSvgFormat()
{
  return new SvgFormat;
}

bool SvgFormat::onLoad(FileOp* fop)
{
  return false;
}

#ifdef ENABLE_SAVE

bool SvgFormat::onSave(FileOp* fop)
{
  const ImageRef image = fop->sequenceImageToSave();
  int x, y, c, r, g, b, a, alpha;
  const auto svg_options = std::static_pointer_cast<SvgOptions>(fop->formatOptions());
  const int pixelScaleValue = std::clamp(svg_options->pixelScale, 0, 10000);
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  auto printRect =
    [f](int x, int y, int width, int height, int r, int g, int b, int a, int pxScale) {
      fprintf(f,
              "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"#%02X%02X%02X\" ",
              x * pxScale,
              y * pxScale,
              width * pxScale,
              height * pxScale,
              r,
              g,
              b);
      if (a != 255)
        fprintf(f, "opacity=\"%f\" ", (float)a / 255.0);
      fprintf(f, "/>\n");
    };

  // Helper function for greedy merge of same-colored pixels
  auto mergePixels = [&](auto getPixelColor) {
    std::vector<std::vector<bool>> processed(image->height(),
                                             std::vector<bool>(image->width(), false));

    for (y = 0; y < image->height(); y++) {
      for (x = 0; x < image->width(); x++) {
        if (processed[y][x])
          continue;

        auto [pixelR, pixelG, pixelB, pixelA, shouldSkip] = getPixelColor(x, y);
        if (shouldSkip) {
          processed[y][x] = true;
          continue;
        }

        // Find the largest rectangle starting from this pixel
        int maxWidth = 0;
        int maxHeight = 0;

        // Find maximum width for current row
        for (int w = x; w < image->width(); w++) {
          if (processed[y][w])
            break;
          auto [checkR, checkG, checkB, checkA, skip] = getPixelColor(w, y);
          if (skip || checkR != pixelR || checkG != pixelG || checkB != pixelB || checkA != pixelA)
            break;
          maxWidth = w - x + 1;
        }

        // Find maximum height with current width
        for (int h = y; h < image->height(); h++) {
          bool canExtend = true;
          for (int w = x; w < x + maxWidth; w++) {
            if (processed[h][w]) {
              canExtend = false;
              break;
            }
            auto [checkR, checkG, checkB, checkA, skip] = getPixelColor(w, h);
            if (skip || checkR != pixelR || checkG != pixelG || checkB != pixelB ||
                checkA != pixelA) {
              canExtend = false;
              break;
            }
          }
          if (!canExtend)
            break;
          maxHeight = h - y + 1;
        }

        // Mark rectangle as processed and output it
        for (int h = y; h < y + maxHeight; h++) {
          for (int w = x; w < x + maxWidth; w++) {
            processed[h][w] = true;
          }
        }

        printRect(x, y, maxWidth, maxHeight, pixelR, pixelG, pixelB, pixelA, pixelScaleValue);
      }
      fop->setProgress((float)y / (float)(image->height()));
    }
  };

  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
  fprintf(
    f,
    "<svg version=\"1.1\" width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\" shape-rendering=\"crispEdges\">\n",
    image->width() * pixelScaleValue,
    image->height() * pixelScaleValue);

  switch (image->pixelFormat()) {
    case IMAGE_RGB: {
      auto getPixelColor = [&](int px, int py) -> std::tuple<int, int, int, int, bool> {
        c = get_pixel_fast<RgbTraits>(image.get(), px, py);
        alpha = rgba_geta(c);
        if (alpha == 0x00)
          return { 0, 0, 0, 0, true };
        return { rgba_getr(c), rgba_getg(c), rgba_getb(c), alpha, false };
      };
      mergePixels(getPixelColor);
      break;
    }
    case IMAGE_GRAYSCALE: {
      auto getPixelColor = [&](int px, int py) -> std::tuple<int, int, int, int, bool> {
        c = get_pixel_fast<GrayscaleTraits>(image.get(), px, py);
        auto v = graya_getv(c);
        alpha = graya_geta(c);
        if (alpha == 0x00)
          return { 0, 0, 0, 0, true };
        return { v, v, v, alpha, false };
      };
      mergePixels(getPixelColor);
      break;
    }
    case IMAGE_INDEXED: {
      unsigned char image_palette[256][4];
      for (int i = 0; i < 256; i++) {
        fop->sequenceGetColor(i, &r, &g, &b);
        image_palette[i][0] = r;
        image_palette[i][1] = g;
        image_palette[i][2] = b;
        fop->sequenceGetAlpha(i, &a);
        image_palette[i][3] = a;
      }
      color_t mask_color = -1;
      if (fop->document()->sprite()->backgroundLayer() == NULL ||
          !fop->document()->sprite()->backgroundLayer()->isVisible()) {
        mask_color = fop->document()->sprite()->transparentColor();
      }

      auto getPixelColor = [&](int px, int py) -> std::tuple<int, int, int, int, bool> {
        c = get_pixel_fast<IndexedTraits>(image.get(), px, py);
        if (c == mask_color)
          return { 0, 0, 0, 0, true };
        return { image_palette[c][0] & 0xff,
                 image_palette[c][1] & 0xff,
                 image_palette[c][2] & 0xff,
                 image_palette[c][3] & 0xff,
                 false };
      };
      mergePixels(getPixelColor);
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
FormatOptionsPtr SvgFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<SvgOptions>();
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();

      if (pref.isSet(pref.svg.pixelScale))
        opts->pixelScale = pref.svg.pixelScale();

      if (pref.svg.showAlert()) {
        app::gen::SvgOptions win;
        win.pxsc()->setTextf("%d", opts->pixelScale);
        win.openWindowInForeground();

        if (win.closer() == win.ok()) {
          pref.svg.pixelScale((int)win.pxsc()->textInt());
          pref.svg.showAlert(!win.dontShow()->isSelected());

          opts->pixelScale = pref.svg.pixelScale();
        }
        else {
          opts.reset();
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<SvgOptions>(nullptr);
    }
  }
  return opts;
}

} // namespace app
