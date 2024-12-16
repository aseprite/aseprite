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
#include "base/convert_to.h"
#include "base/file_handle.h"
#include "base/string.h"
#include "doc/doc.h"
#include "ui/window.h"

#include "css_options.xml.h"

namespace app {

using namespace base;

class CssFormat : public FileFormat {
  class CssOptions : public FormatOptions {
  public:
    CssOptions() : pixelScale(1), gutterSize(0), generateHtml(false), withVars(false) {}
    int pixelScale;
    int gutterSize;
    bool generateHtml;
    bool withVars;
  };

  const char* onGetName() const override { return "css"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("css"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::CSS_STYLE; }

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

FileFormat* CreateCssFormat()
{
  return new CssFormat;
}

bool CssFormat::onLoad(FileOp* fop)
{
  return false;
}

#ifdef ENABLE_SAVE

bool CssFormat::onSave(FileOp* fop)
{
  const ImageRef image = fop->sequenceImageToSave();
  int x, y, c, r, g, b, a, alpha;
  const auto css_options = std::static_pointer_cast<CssOptions>(fop->formatOptions());
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();
  auto print_color = [f](int r, int g, int b, int a) {
    if (a == 255) {
      fprintf(f, "#%02X%02X%02X", r, g, b);
    }
    else {
      fprintf(f, "rgba(%d, %d, %d, %d)", r, g, b, a);
    }
  };
  auto print_shadow_color =
    [f, css_options, print_color](int x, int y, int r, int g, int b, int a, bool comma = true) {
      fprintf(f, comma ? ",\n" : "\n");
      if (css_options->withVars) {
        fprintf(
          f,
          "\tcalc(%d*var(--shadow-mult)) calc(%d*var(--shadow-mult)) var(--blur) var(--spread) ",
          x,
          y);
      }
      else {
        int x_loc = x * (css_options->pixelScale + css_options->gutterSize);
        int y_loc = y * (css_options->pixelScale + css_options->gutterSize);
        fprintf(f, "%dpx %dpx ", x_loc, y_loc);
      }
      print_color(r, g, b, a);
    };
  auto print_shadow_index = [f, css_options](int x, int y, int i, bool comma = true) {
    fprintf(f, comma ? ",\n" : "\n");
    fprintf(
      f,
      "\tcalc(%d*var(--shadow-mult)) calc(%d*var(--shadow-mult)) var(--blur) var(--spread) var(--color-%d)",
      x,
      y,
      i);
  };
  if (css_options->withVars) {
    fprintf(f,
            ":root {\n"
            "\t--blur: 0px;\n"
            "\t--spread: 0px;\n"
            "\t--pixel-size: %dpx;\n"
            "\t--gutter-size: %dpx;\n",
            css_options->pixelScale,
            css_options->gutterSize);
    fprintf(f, "\t--shadow-mult: calc(var(--gutter-size) + var(--pixel-size));\n");
    if (image->pixelFormat() == IMAGE_INDEXED) {
      for (y = 0; y < 256; y++) {
        fop->sequenceGetColor(y, &r, &g, &b);
        fop->sequenceGetAlpha(y, &a);
        fprintf(f, "\t--color-%d: ", y);
        print_color(r, g, b, a);
        fprintf(f, ";\n");
      }
    }
    fprintf(f, "}\n\n");
  }

  fprintf(f, ".pixel-art {\n");
  fprintf(f, "\tposition: relative;\n");
  fprintf(f, "\ttop: 0;\n");
  fprintf(f, "\tleft: 0;\n");
  if (css_options->withVars) {
    fprintf(f, "\theight: var(--pixel-size);\n");
    fprintf(f, "\twidth: var(--pixel-size);\n");
  }
  else {
    fprintf(f, "\theight: %dpx;\n", css_options->pixelScale);
    fprintf(f, "\twidth: %dpx;\n", css_options->pixelScale);
  }
  fprintf(f, "\tbox-shadow:\n");
  int num_printed_pixels = 0;
  switch (image->pixelFormat()) {
    case IMAGE_RGB: {
      for (y = 0; y < image->height(); y++) {
        for (x = 0; x < image->width(); x++) {
          c = get_pixel_fast<RgbTraits>(image.get(), x, y);
          alpha = rgba_geta(c);
          if (alpha != 0x00) {
            print_shadow_color(x,
                               y,
                               rgba_getr(c),
                               rgba_getg(c),
                               rgba_getb(c),
                               alpha,
                               num_printed_pixels > 0);
            num_printed_pixels++;
          }
        }
        fop->setProgress((float)y / (float)(image->height()));
      }
      break;
    }
    case IMAGE_GRAYSCALE: {
      for (y = 0; y < image->height(); y++) {
        for (x = 0; x < image->width(); x++) {
          c = get_pixel_fast<GrayscaleTraits>(image.get(), x, y);
          auto v = graya_getv(c);
          alpha = graya_geta(c);
          if (alpha != 0x00) {
            print_shadow_color(x, y, v, v, v, alpha, num_printed_pixels > 0);
            num_printed_pixels++;
          }
        }
        fop->setProgress((float)y / (float)(image->height()));
      }
      break;
    }
    case IMAGE_INDEXED: {
      unsigned char image_palette[256][4];
      for (y = 0; !css_options->withVars && y < 256; y++) {
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
      for (y = 0; y < image->height(); y++) {
        for (x = 0; x < image->width(); x++) {
          c = get_pixel_fast<IndexedTraits>(image.get(), x, y);
          if (c != mask_color) {
            if (css_options->withVars) {
              print_shadow_index(x, y, c, num_printed_pixels > 0);
            }
            else {
              print_shadow_color(x,
                                 y,
                                 image_palette[c][0] & 0xff,
                                 image_palette[c][1] & 0xff,
                                 image_palette[c][2] & 0xff,
                                 image_palette[c][3] & 0xff,
                                 num_printed_pixels > 0);
            }
            num_printed_pixels++;
          }
        }
        fop->setProgress((float)y / (float)(image->height()));
      }
      break;
    }
  }
  fprintf(f, ";\n}\n");
  if (ferror(f)) {
    fop->setError("Error writing file.\n");
    return false;
  }
  if (css_options->generateHtml) {
    std::string html_filepath = fop->filename() + ".html";
    FileHandle handle(open_file_with_exception_sync_on_close(html_filepath, "wb"));
    FILE* h = handle.get();
    fprintf(h,
            "<html><head><link rel=\"stylesheet\" media=\"all\" "
            "href=\"%s\"></head><body><div "
            "class=\"pixel-art\"></div></body></html>",
            fop->filename().c_str());
    if (ferror(h)) {
      fop->setError("Error writing html file.\n");
      return false;
    }
  }
  return true;
}

#endif

// Shows the CSS configuration dialog.
FormatOptionsPtr CssFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<CssOptions>();

  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();

      if (pref.isSet(pref.css.pixelScale))
        opts->pixelScale = pref.css.pixelScale();

      if (pref.isSet(pref.css.withVars))
        opts->withVars = pref.css.withVars();

      if (pref.isSet(pref.css.generateHtml))
        opts->generateHtml = pref.css.generateHtml();

      if (pref.css.showAlert()) {
        app::gen::CssOptions win;
        win.pixelScale()->setTextf("%d", opts->pixelScale);
        win.withVars()->setSelected(opts->withVars);
        win.generateHtml()->setSelected(opts->generateHtml);
        win.openWindowInForeground();

        if (win.closer() == win.ok()) {
          pref.css.showAlert(!win.dontShow()->isSelected());
          pref.css.pixelScale((int)win.pixelScale()->textInt());
          pref.css.withVars(win.withVars()->isSelected());
          pref.css.generateHtml(win.generateHtml()->isSelected());

          opts->generateHtml = pref.css.generateHtml();
          opts->withVars = pref.css.withVars();
          opts->pixelScale = pref.css.pixelScale();
        }
        else {
          opts.reset();
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<CssOptions>(nullptr);
    }
  }

  return opts;
}

} // namespace app
