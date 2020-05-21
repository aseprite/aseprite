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

class TgaDelegate : public tga::Delegate {
public:
  TgaDelegate(FileOp* fop) : m_fop(fop) { }
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
  tga::StdioFileInterface finterface(handle.get());
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
  TgaDelegate delegate(fop);
  if (!decoder.readImage(header, tgaImage, &delegate)) {
    fop->setError("Error loading image data from TGA file.\n");
    return false;
  }

  // Fix alpha values for RGB images
  decoder.postProcessImage(header, tgaImage);

  // Post process gray image pixels (because we use grayscale images
  // with alpha).
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

  if (ferror(handle.get())) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE

namespace {

void prepare_header(tga::Header& header,
                    const doc::Image* image,
                    const doc::Palette* palette,
                    const bool isOpaque,
                    const bool compressed,
                    int bitsPerPixel)
{
  header.idLength = 0;
  header.colormapType = 0;
  header.imageType = tga::NoImage;
  header.colormapOrigin = 0;
  header.colormapLength = 0;
  header.colormapDepth = 0;
  header.xOrigin = 0;
  header.yOrigin = 0;
  header.width = image->width();
  header.height = image->height();
  header.bitsPerPixel = 0;
  // TODO make this option configurable
  header.imageDescriptor = 0x20; // Top-to-bottom

  switch (image->colorMode()) {
    case ColorMode::RGB:
      header.imageType = (compressed ? tga::RleRgb: tga::UncompressedRgb);
      header.bitsPerPixel = (bitsPerPixel > 8 ?
                               bitsPerPixel:
                               (isOpaque ? 24: 32));
      if (!isOpaque) {
        switch (header.bitsPerPixel) {
          case 16: header.imageDescriptor |= 1; break;
          case 32: header.imageDescriptor |= 8; break;
        }
      }
      break;
    case ColorMode::GRAYSCALE:
      // TODO if the grayscale is not opaque, we should use RGB,
      //      this could be done automatically in FileOp in a
      //      generic way for all formats when FILE_SUPPORT_RGBA is
      //      available and FILE_SUPPORT_GRAYA isn't.
      header.imageType = (compressed ? tga::RleGray: tga::UncompressedGray);
      header.bitsPerPixel = 8;
      break;
    case ColorMode::INDEXED:
      ASSERT(palette);

      header.imageType = (compressed ? tga::RleIndexed: tga::UncompressedIndexed);
      header.bitsPerPixel = 8;
      header.colormapType = 1;
      header.colormapLength = palette->size();
      if (palette->hasAlpha())
        header.colormapDepth = 32;
      else
        header.colormapDepth = 24;

      header.colormap = tga::Colormap(palette->size());
      for (int i=0; i<palette->size(); ++i) {
        doc::color_t c = palette->getEntry(i);
        header.colormap[i] =
          tga::rgba(doc::rgba_getr(c),
                    doc::rgba_getg(c),
                    doc::rgba_getb(c),
                    doc::rgba_geta(c));
      }
      break;
  }
}

} // anonymous namespace

bool TgaFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  const Palette* palette = fop->sequenceGetPalette();

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  tga::StdioFileInterface finterface(handle.get());
  tga::Encoder encoder(&finterface);
  tga::Header header;

  const auto tgaOptions = std::static_pointer_cast<TgaOptions>(fop->formatOptions());
  prepare_header(
    header, image, palette,
    // Is alpha channel required?
    fop->document()->sprite()->isOpaque(),
    // Compressed by default
    (tgaOptions ? tgaOptions->compress(): true),
    // Bits per pixel (0 means "calculate what is best")
    (tgaOptions ? tgaOptions->bitsPerPixel(): 0));

  encoder.writeHeader(header);

  tga::Image tgaImage;
  tgaImage.pixels = image->getPixelAddress(0, 0);
  tgaImage.rowstride = image->getRowStrideSize();
  tgaImage.bytesPerPixel = image->getRowStrideSize(1);

  TgaDelegate delegate(fop);
  encoder.writeImage(header, tgaImage);
  encoder.writeFooter();

  if (ferror(handle.get())) {
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
