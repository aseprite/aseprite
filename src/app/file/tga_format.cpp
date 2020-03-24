// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/tga_options.h"
#include "base/cfile.h"
#include "base/convert_to.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "doc/image_bits.h"
#include "tga/tga.h"
#include "ui/combobox.h"
#include "ui/listitem.h"

#include "tga_options.xml.h"

namespace app {

using namespace base;

class TgaFormat : public FileFormat {
  const char* onGetName() const override {
    return "tga";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("tga");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::TARGA_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif

  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

FileFormat* CreateTgaFormat()
{
  return new TgaFormat;
}

namespace {

class TgaDecoderDelegate : public tga::DecoderDelegate {
public:
  TgaDecoderDelegate(FileOp* fop) : m_fop(fop) { }
  bool notifyProgress(double progress) override {
    m_fop->setProgress(progress);
    return !m_fop->isStop();
  }
private:
  FileOp* m_fop;
};

bool get_image_spec(const tga::Header& header, ImageSpec& spec)
{
  switch (header.imageType) {

    case tga::UncompressedIndexed:
    case tga::RleIndexed:
      if (header.bitsPerPixel != 8)
        return false;
      spec = ImageSpec(ColorMode::INDEXED,
                       header.width,
                       header.height);
      return true;

    case tga::UncompressedRgb:
    case tga::RleRgb:
      if (header.bitsPerPixel != 15 &&
          header.bitsPerPixel != 16 &&
          header.bitsPerPixel != 24 &&
          header.bitsPerPixel != 32)
        return false;
      spec = ImageSpec(ColorMode::RGB,
                       header.width,
                       header.height);
      return true;

    case tga::UncompressedGray:
    case tga::RleGray:
      if (header.bitsPerPixel != 8)
        return false;
      spec = ImageSpec(ColorMode::GRAYSCALE,
                       header.width,
                       header.height);
      return true;
  }
  return false;
}

} // anonymous namespace

bool TgaFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();
  tga::StdioFileInterface finterface(f);
  tga::Decoder decoder(&finterface);

  tga::Header header;
  if (!decoder.readHeader(header)) {
    fop->setError("Invalid TGA header\n");
    return false;
  }

  ImageSpec spec(ColorMode::RGB, 1, 1);
  if (!get_image_spec(header, spec)) {
    fop->setError("Unsupported color depth in TGA file: %d bpp, image type=%d.\n",
                  header.bitsPerPixel,
                  header.imageType);
    return false;
  }

  // Palette from TGA file
  if (header.hasColormap()) {
    const tga::Colormap& pal = header.colormap;
    for (int i=0; i<pal.size(); ++i) {
      tga::color_t c = pal[i];
      fop->sequenceSetColor(i,
                            tga::getr(c),
                            tga::getg(c),
                            tga::getb(c));
      if (tga::geta(c) < 255)
        fop->sequenceSetAlpha(i, tga::geta(c));
    }
  }
  // Generate grayscale palette
  else if (header.isGray()) {
    for (int i=0; i<256; ++i)
      fop->sequenceSetColor(i, i, i, i);
  }

  if (decoder.hasAlpha())
    fop->sequenceSetHasAlpha(true);

  Image* image = fop->sequenceImage((doc::PixelFormat)spec.colorMode(),
                                    spec.width(),
                                    spec.height());
  if (!image)
    return false;

  tga::Image tgaImage;
  tgaImage.pixels = image->getPixelAddress(0, 0);
  tgaImage.rowstride = image->getRowStrideSize();
  tgaImage.bytesPerPixel = image->getRowStrideSize(1);

  // Read image
  TgaDecoderDelegate delegate(fop);
  if (!decoder.readImage(header, tgaImage, &delegate)) {
    fop->setError("Error loading image data from TGA file.\n");
    return false;
  }

  if (header.isGray()) {
    doc::LockImageBits<GrayscaleTraits> bits(image);
    for (auto it=bits.begin(), end=bits.end(); it != end; ++it) {
      *it = doc::graya(*it, 255);
    }
  }

  if (decoder.hasAlpha())
    fop->sequenceSetHasAlpha(true);

  // Set default options for this TGA
  auto opts = std::make_shared<TgaOptions>();
  opts->bitsPerPixel(header.bitsPerPixel);
  opts->compress(header.isRle());
  fop->setLoadedFormatOptions(opts);

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE

namespace {

class TgaEncoder {
public:
  TgaEncoder(FILE* f)
    : m_f(f) {
  }

  void prepareHeader(const Image* image,
                     const Palette* palette,
                     const bool isOpaque,
                     const bool compressed,
                     int bitsPerPixel) {
    m_header.idLength = 0;
    m_header.colormapType = 0;
    m_header.imageType = tga::NoImage;
    m_header.colormapOrigin = 0;
    m_header.colormapLength = 0;
    m_header.colormapDepth = 0;
    m_header.xOrigin = 0;
    m_header.yOrigin = 0;
    m_header.width = image->width();
    m_header.height = image->height();
    m_header.bitsPerPixel = 0;
    // TODO make this option configurable
    m_header.imageDescriptor = 0x20; // Top-to-bottom

    switch (image->colorMode()) {
      case ColorMode::RGB:
        m_header.imageType = (compressed ? tga::RleRgb: tga::UncompressedRgb);
        m_header.bitsPerPixel = (bitsPerPixel > 8 ?
                                 bitsPerPixel:
                                 (isOpaque ? 24: 32));
        if (!isOpaque) {
          switch (m_header.bitsPerPixel) {
            case 16: m_header.imageDescriptor |= 1; break;
            case 32: m_header.imageDescriptor |= 8; break;
          }
        }
        break;
      case ColorMode::GRAYSCALE:
        // TODO if the grayscale is not opaque, we should use RGB,
        //      this could be done automatically in FileOp in a
        //      generic way for all formats when FILE_SUPPORT_RGBA is
        //      available and FILE_SUPPORT_GRAYA isn't.
        m_header.imageType = (compressed ? tga::RleGray: tga::UncompressedGray);
        m_header.bitsPerPixel = 8;
        break;
      case ColorMode::INDEXED:
        ASSERT(palette);

        m_header.imageType = (compressed ? tga::RleIndexed: tga::UncompressedIndexed);
        m_header.bitsPerPixel = 8;
        m_header.colormapType = 1;
        m_header.colormapLength = palette->size();
        if (palette->hasAlpha())
          m_header.colormapDepth = 32;
        else
          m_header.colormapDepth = 24;
        break;
    }
  }

  bool needsPalette() const {
    return (m_header.colormapType == 1);
  }

  void writeHeader() {
    fputc(m_header.idLength, m_f);
    fputc(m_header.colormapType, m_f);
    fputc(m_header.imageType, m_f);
    fputw(m_header.colormapOrigin, m_f);
    fputw(m_header.colormapLength, m_f);
    fputc(m_header.colormapDepth, m_f);
    fputw(m_header.xOrigin, m_f);
    fputw(m_header.yOrigin, m_f);
    fputw(m_header.width, m_f);
    fputw(m_header.height, m_f);
    fputc(m_header.bitsPerPixel, m_f);
    fputc(m_header.imageDescriptor, m_f);
  }

  void writeFooter() {
    const char* tga2_footer = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0";
    fwrite(tga2_footer, 1, 26, m_f);
  }

  void writePalette(const Palette* palette) {
    ASSERT(palette->size() == m_header.colormapLength);

    for (int i=0; i<palette->size(); ++i) {
      color_t c = palette->getEntry(i);
      fputc(rgba_getb(c), m_f);
      fputc(rgba_getg(c), m_f);
      fputc(rgba_getr(c), m_f);
      if (m_header.colormapDepth == 32)
        fputc(rgba_geta(c), m_f);
    }
  }

  void writeImageData(FileOp* fop, const Image* image) {
    auto fput = (m_header.bitsPerPixel == 32 ? &TgaEncoder::fput32:
                 m_header.bitsPerPixel == 24 ? &TgaEncoder::fput24:
                 m_header.bitsPerPixel == 16 ? &TgaEncoder::fput16:
                 m_header.bitsPerPixel == 15 ? &TgaEncoder::fput16:
                                               &TgaEncoder::fput8);

    switch (m_header.imageType) {

      case tga::UncompressedIndexed: {
        for (int y=0; y<image->height(); ++y) {
          for (int x=0; x<image->width(); ++x) {
            color_t c = get_pixel_fast<IndexedTraits>(image, x, y);
            fput8(c);
          }
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case tga::UncompressedRgb: {
        for (int y=0; y<image->height(); ++y) {
          for (int x=0; x<image->width(); ++x) {
            color_t c = get_pixel_fast<RgbTraits>(image, x, y);
            (this->*fput)(c);
          }
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case tga::UncompressedGray: {
        for (int y=0; y<image->height(); ++y) {
          for (int x=0; x<image->width(); ++x) {
            color_t c = get_pixel_fast<GrayscaleTraits>(image, x, y);
            fput8(graya_getv(c));
          }
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case tga::RleIndexed: {
        for (int y=0; y<image->height(); ++y) {
          writeRleScanline<IndexedTraits>(image, y, fput);
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case tga::RleRgb: {
        ASSERT(m_header.bitsPerPixel == 15 ||
               m_header.bitsPerPixel == 16 ||
               m_header.bitsPerPixel == 24 ||
               m_header.bitsPerPixel == 32);

        for (int y=0; y<image->height(); ++y) {
          writeRleScanline<RgbTraits>(image, y, fput);
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case tga::RleGray: {
        for (int y=0; y<image->height(); ++y) {
          writeRleScanline<GrayscaleTraits>(image, y, fput);
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

    }
  }

private:
  template<typename ImageTraits>
  void writeRleScanline(const Image* image, int y,
                        void (TgaEncoder::*writePixel)(color_t)) {
    int x = 0;
    while (x < image->width()) {
      int count, offset;
      countRepeatedPixels<ImageTraits>(image, x, y, offset, count);

      // Save a sequence of pixels with different colors
      while (offset > 0) {
        const int n = std::min(offset, 128);

        fputc(n - 1, m_f);
        for (int i=0; i<n; ++i) {
          const color_t c = get_pixel_fast<ImageTraits>(image, x++, y);
          (this->*writePixel)(c);
        }
        offset -= n;
      }

      // Save a sequence of pixels with the same color
      while (count*ImageTraits::bytes_per_pixel > 1+ImageTraits::bytes_per_pixel) {
        const int n = std::min(count, 128);
        const color_t c = get_pixel_fast<ImageTraits>(image, x, y);

#if _DEBUG
        for (int i=0; i<n; ++i) {
          ASSERT(get_pixel_fast<ImageTraits>(image, x+i, y) == c);
        }
#endif
        fputc(0x80 | (n - 1), m_f);
        (this->*writePixel)(c);
        count -= n;
        x += n;
      }
    }
    ASSERT(x == image->width());
  }

  template<typename ImageTraits>
  void countRepeatedPixels(const Image* image, int x0, int y,
                           int& offset, int& count) {
    for (int x=x0; x<image->width(); ) {
      color_t p = get_pixel_fast<ImageTraits>(image, x, y);

      int u = x+1;
      for (; u<image->width(); ++u) {
        color_t q = get_pixel_fast<ImageTraits>(image, u, y);
        if (p != q)
          break;
      }

      if ((u - x)*ImageTraits::bytes_per_pixel > 1+ImageTraits::bytes_per_pixel) {
        offset = x - x0;
        count = u - x;
        return;
      }

      x = u;
    }

    offset = image->width() - x0;
    count = 0;
  }

  void fput8(color_t c) {
    fputc(c, m_f);
  }

  void fput16(color_t c) {
    int r = rgba_getr(c);
    int g = rgba_getg(c);
    int b = rgba_getb(c);
    int a = rgba_geta(c);
    uint16_t v =
      ((r>>3) << 10) |
      ((g>>3) << 5) |
      ((b>>3)) |
      (m_header.bitsPerPixel == 16 && a >= 128 ? 0x8000: 0); // TODO configurable threshold
    fputw(v, m_f);
  }

  void fput24(color_t c) {
    fputc(rgba_getb(c), m_f);
    fputc(rgba_getg(c), m_f);
    fputc(rgba_getr(c), m_f);
  }

  void fput32(color_t c) {
    fputc(rgba_getb(c), m_f);
    fputc(rgba_getg(c), m_f);
    fputc(rgba_getr(c), m_f);
    fputc(rgba_geta(c), m_f);
  }

  FILE* m_f;
  tga::Header m_header;
};

} // anonymous namespace

bool TgaFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  const Palette* palette = fop->sequenceGetPalette();

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  const auto tgaOptions = std::static_pointer_cast<TgaOptions>(fop->formatOptions());
  TgaEncoder encoder(f);

  encoder.prepareHeader(
    image, palette,
    // Is alpha channel required?
    fop->document()->sprite()->isOpaque(),
    // Compressed by default
    (tgaOptions ? tgaOptions->compress(): true),
    // Bits per pixel (0 means "calculate what is best")
    (tgaOptions ? tgaOptions->bitsPerPixel(): 0));
  encoder.writeHeader();

  if (encoder.needsPalette())
    encoder.writePalette(palette);

  encoder.writeImageData(fop, image);
  encoder.writeFooter();

  if (ferror(f)) {
    fop->setError("Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}

#endif  // ENABLE_SAVE

FormatOptionsPtr TgaFormat::onAskUserForFormatOptions(FileOp* fop)
{
  const bool origOpts = fop->hasFormatOptionsOfDocument();
  auto opts = fop->formatOptionsOfDocument<TgaOptions>();
#ifdef ENABLE_UI
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();

      // If the TGA options are not original from a TGA file, we can
      // use the default options from the preferences.
      if (!origOpts) {
        if (pref.isSet(pref.tga.bitsPerPixel))
          opts->bitsPerPixel(pref.tga.bitsPerPixel());
        if (pref.isSet(pref.tga.compress))
          opts->compress(pref.tga.compress());
      }

      if (pref.tga.showAlert()) {
        const bool isOpaque = fop->document()->sprite()->isOpaque();
        const std::string defBitsPerPixel = (isOpaque ? "24": "32");
        app::gen::TgaOptions win;

        if (fop->document()->colorMode() == doc::ColorMode::RGB) {
          // TODO implement a better way to create ListItems with values
          auto newItem = [](const char* s) -> ui::ListItem* {
                           auto item = new ui::ListItem(s);
                           item->setValue(s);
                           return item;
                         };

          win.bitsPerPixel()->addItem(newItem("16"));
          win.bitsPerPixel()->addItem(newItem("24"));
          win.bitsPerPixel()->addItem(newItem("32"));

          std::string v = defBitsPerPixel;
          if (opts->bitsPerPixel() > 0)
            v = base::convert_to<std::string>(opts->bitsPerPixel());
          win.bitsPerPixel()->setValue(v);
        }
        else {
          win.bitsPerPixelLabel()->setVisible(false);
          win.bitsPerPixel()->setVisible(false);
        }
        win.compress()->setSelected(opts->compress());

        win.openWindowInForeground();

        if (win.closer() == win.ok()) {
          int bpp = base::convert_to<int>(win.bitsPerPixel()->getValue());

          pref.tga.bitsPerPixel(bpp);
          pref.tga.compress(win.compress()->isSelected());
          pref.tga.showAlert(!win.dontShow()->isSelected());

          opts->bitsPerPixel(pref.tga.bitsPerPixel());
          opts->compress(pref.tga.compress());
        }
        else {
          opts.reset();
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<TgaOptions>(nullptr);
    }
  }
#endif // ENABLE_UI
  return opts;
}

} // namespace app
