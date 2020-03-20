// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
      FILE_SUPPORT_SEQUENCES;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateTgaFormat()
{
  return new TgaFormat;
}

namespace {

enum TgaImageType {
  NoImage = 0,
  UncompressedIndexed = 1,
  UncompressedRgb = 2,
  UncompressedGray = 3,
  RleIndexed = 9,
  RleRgb = 10,
  RleGray = 11,
};

struct TgaHeader {
  uint8_t  idLength;
  uint8_t  palType;
  uint8_t  imageType;
  uint16_t palOrigin;
  uint16_t palLength;
  uint8_t  palDepth;
  uint16_t xOrigin;
  uint16_t yOrigin;
  uint16_t width;
  uint16_t height;
  uint8_t  bitsPerPixel;
  uint8_t  imageDescriptor;
};

class TgaDecoder {
public:
  TgaDecoder(FILE* f)
    : m_f(f)
    , m_palette(0, 0)
    , m_alphaHistogram(256) {
    readHeader();
    if (m_header.palType == 1)
      readPaletteData();
  }

  int imageType() const { return m_header.imageType; }
  int bitsPerPixel() const { return m_header.bitsPerPixel; }
  bool hasPalette() const { return (m_header.palLength > 0); }
  const Palette& palette() const { return m_palette; }
  bool hasAlpha() const { return m_hasAlpha; }

  bool isGray() const {
    return (m_header.imageType == UncompressedGray ||
            m_header.imageType == RleGray);
  }

  bool validPalType() const {
    return
      // Indexed with palette
      ((m_header.imageType == UncompressedIndexed || m_header.imageType == RleIndexed) &&
       m_header.bitsPerPixel == 8 &&
       m_header.palType == 1) ||
       // Grayscale without palette
      ((m_header.imageType == UncompressedGray || m_header.imageType == RleGray) &&
       m_header.bitsPerPixel == 8 &&
       m_header.palType == 0) ||
      // Non-indexed without palette
      (m_header.bitsPerPixel > 8 &&
       m_header.palType == 0);
  }

  bool getImageSpec(ImageSpec& spec) const {
    switch (m_header.imageType) {

      case UncompressedIndexed:
      case RleIndexed:
        if (m_header.bitsPerPixel != 8)
          return false;
        spec = ImageSpec(ColorMode::INDEXED,
                         m_header.width,
                         m_header.height);
        return true;

      case UncompressedRgb:
      case RleRgb:
        if (m_header.bitsPerPixel != 15 &&
            m_header.bitsPerPixel != 16 &&
            m_header.bitsPerPixel != 24 &&
            m_header.bitsPerPixel != 32)
          return false;
        spec = ImageSpec(ColorMode::RGB,
                         m_header.width,
                         m_header.height);
        return true;

      case UncompressedGray:
      case RleGray:
        if (m_header.bitsPerPixel != 8)
          return false;
        spec = ImageSpec(ColorMode::GRAYSCALE,
                         m_header.width,
                         m_header.height);
        return true;
    }
    return false;
  }

  bool readImageData(FileOp* fop, Image* image) {
    // Bit 4 means right-to-left, else left-to-right
    // Bit 5 means top-to-bottom, else bottom-to-top
    m_iterator = ImageDataIterator(
      image,
      (m_header.imageDescriptor & 0x10) ? false: true,
      (m_header.imageDescriptor & 0x20) ? true: false,
      m_header.width,
      m_header.height);

    for (int y=0; y<m_header.height; ++y) {
      switch (m_header.imageType) {

        case UncompressedIndexed:
          ASSERT(m_header.bitsPerPixel == 8);
          if (readUncompressedData<uint8_t>(&TgaDecoder::fget8_as_index))
            return true;
          break;

        case UncompressedRgb:
          switch (m_header.bitsPerPixel) {
            case 15:
            case 16:
              if (readUncompressedData<uint32_t>(&TgaDecoder::fget16_as_rgba))
                return true;
              break;
            case 24:
              if (readUncompressedData<uint32_t>(&TgaDecoder::fget24_as_rgba))
                return true;
              break;
            case 32:
              if (readUncompressedData<uint32_t>(&TgaDecoder::fget32_as_rgba))
                return true;
              break;
            default:
              ASSERT(false);
              break;
          }
          break;

        case UncompressedGray:
          ASSERT(m_header.bitsPerPixel == 8);
          if (readUncompressedData<uint16_t>(&TgaDecoder::fget8_as_gray))
            return true;
          break;

        case RleIndexed:
          ASSERT(m_header.bitsPerPixel == 8);
          if (readRleData<uint8_t>(&TgaDecoder::fget8_as_gray))
            return true;
          break;

        case RleRgb:
          switch (m_header.bitsPerPixel) {
            case 15:
            case 16:
              if (readRleData<uint32_t>(&TgaDecoder::fget16_as_rgba))
                return true;
              break;
            case 24:
              if (readRleData<uint32_t>(&TgaDecoder::fget24_as_rgba))
                return true;
              break;
            case 32:
              if (readRleData<uint32_t>(&TgaDecoder::fget32_as_rgba))
                return true;
              break;
            default:
              ASSERT(false);
              break;
          }
          break;

        case RleGray:
          ASSERT(m_header.bitsPerPixel == 8);
          if (readRleData<uint16_t>(&TgaDecoder::fget8_as_gray))
            return true;
          break;
      }
      fop->setProgress(float(y) / float(m_header.height));
      if (fop->isStop())
        break;
    }

    return true;
  }

  // Fix alpha channel for images with invalid alpha channel values
  void postProcessImageData(Image* image) {
    if (image->colorMode() != ColorMode::RGB ||
        !m_hasAlpha)
      return;

    int count = 0;
    for (int i=0; i<256; ++i)
      if (m_alphaHistogram[i] > 0)
        ++count;

    // If all pixels are transparent (alpha=0), make all pixels opaque
    // (alpha=255).
    if (count == 1 && m_alphaHistogram[0] > 0) {
      LockImageBits<RgbTraits> bits(image);
      auto it = bits.begin(), end = bits.end();
      for (; it != end; ++it) {
        color_t c = *it;
        *it = rgba(rgba_getr(c),
                   rgba_getg(c),
                   rgba_getb(c), 255);
      }
    }
  }

private:
  class ImageDataIterator {
  public:
    ImageDataIterator() { }

    ImageDataIterator(Image* image,
                      bool leftToRight,
                      bool topToBottom,
                      int w, int h) {
      m_image = image;
      m_w = w;
      m_h = h;
      m_x = (leftToRight ? 0: w-1);
      m_y = (topToBottom ? 0: h-1);
      m_dx = (leftToRight ? +1: -1);
      m_dy = (topToBottom ? +1: -1);
      calcPtr();
    }

    template<typename T>
    bool next(const T value) {
      *((T*)m_ptr) = value;

      m_x += m_dx;
      m_ptr += m_dx*sizeof(T);

      if ((m_dx < 0 && m_x < 0) ||
          (m_dx > 0 && m_x == m_w)) {
        m_x = (m_dx > 0 ? 0: m_w-1);
        m_y += m_dy;
        if ((m_dy < 0 && m_y < 0) ||
            (m_dy > 0 && m_y == m_h)) {
          return true;
        }
        calcPtr();
      }
      return false;
    }

  private:
    void calcPtr() {
      m_ptr = m_image->getPixelAddress(m_x, m_y);
    }

    Image* m_image;
    int m_x, m_y;
    int m_w, m_h;
    int m_dx, m_dy;
    uint8_t* m_ptr;
  };

  void readHeader() {
    m_header.idLength = fgetc(m_f);
    m_header.palType = fgetc(m_f);
    m_header.imageType = fgetc(m_f);
    m_header.palOrigin = fgetw(m_f);
    m_header.palLength  = fgetw(m_f);
    m_header.palDepth = fgetc(m_f);
    m_header.xOrigin = fgetw(m_f);
    m_header.yOrigin = fgetw(m_f);
    m_header.width = fgetw(m_f);
    m_header.height = fgetw(m_f);
    m_header.bitsPerPixel = fgetc(m_f);
    m_header.imageDescriptor = fgetc(m_f);

    char imageId[256];
    if (m_header.idLength > 0)
      fread(imageId, 1, m_header.idLength, m_f);

#if 0
    // In the best case the "alphaBits" should be valid, but there are
    // invalid TGA files out there which don't indicate the
    // "alphaBits" correctly, so they could be 0 and use the alpha
    // channel anyway on each pixel.
    int alphaBits = (m_header.imageDescriptor & 15);
    TRACEARGS("TGA: bitsPerPixel", (int)m_header.bitsPerPixel,
              "alphaBits", alphaBits);
    m_hasAlpha =
      (m_header.bitsPerPixel == 32 && alphaBits == 8) ||
      (m_header.bitsPerPixel == 16 && alphaBits == 1);
#else
    // So to detect if a 32bpp or 16bpp TGA image has alpha, we'll use
    // the "m_alphaHistogram" to check if there are different alpha
    // values. If there is only one alpha value (all 0 or all 255),
    // we create an opaque image anyway.
    m_hasAlpha =
      (m_header.bitsPerPixel == 32) ||
      (m_header.bitsPerPixel == 16);
#endif
  }

  void readPaletteData() {
    m_palette.resize(m_header.palLength);

    for (int i=0; i<m_header.palLength; ++i) {
      switch (m_header.palDepth) {

        case 15:
        case 16: {
          const int c = fgetw(m_f);
          m_palette.setEntry(
            i,
            doc::rgba(scale_5bits_to_8bits((c >> 10) & 0x1F),
                      scale_5bits_to_8bits((c >> 5) & 0x1F),
                      scale_5bits_to_8bits(c & 0x1F), 255));
          break;
        }

        case 24:
        case 32: {
          const int b = fgetc(m_f);
          const int g = fgetc(m_f);
          const int r = fgetc(m_f);
          int a;
          if (m_header.palDepth == 32)
            a = fgetc(m_f);
          else
            a = 255;
          m_palette.setEntry(i, doc::rgba(r, g, b, a));
          break;
        }
      }
    }
  }

  template<typename T>
  bool readUncompressedData(color_t (TgaDecoder::*readPixel)()) {
    for (int x=0; x<m_header.width; ++x) {
      if (m_iterator.next<T>((this->*readPixel)()))
        return true;
    }
    return false;
  }

  // In the best case (TGA 2.0 spec) this should read just one
  // scanline, but in old TGA versions (1.0) it was possible to save
  // several scanlines with the same RLE data.
  //
  // Returns true when are are done.
  template<typename T>
  bool readRleData(color_t (TgaDecoder::*readPixel)()) {
    for (int x=0; x<m_header.width && !feof(m_f); ) {
      int c = fgetc(m_f);
      if (c & 0x80) {
        c = (c & 0x7f) + 1;
        x += c;
        const T pixel = (this->*readPixel)();
        while (c-- > 0)
          if (m_iterator.next<T>(pixel))
            return true;
      }
      else {
        ++c;
        x += c;
        while (c-- > 0) {
          if (m_iterator.next<T>((this->*readPixel)()))
            return true;
        }
      }
    }
    return false;
  }

  color_t fget8_as_index() {
    return fgetc(m_f);
  }

  color_t fget8_as_gray() {
    return doc::graya(fgetc(m_f), 255);
  }

  color_t fget32_as_rgba() {
    int b = fgetc(m_f);
    int g = fgetc(m_f);
    int r = fgetc(m_f);
    uint8_t a = fgetc(m_f);
    if (!m_hasAlpha)
      a = 255;
    else {
      ++m_alphaHistogram[a];
    }
    return doc::rgba(r, g, b, a);
  }

  color_t fget24_as_rgba() {
    const int b = fgetc(m_f);
    const int g = fgetc(m_f);
    const int r = fgetc(m_f);
    return doc::rgba(r, g, b, 255);
  }

  color_t fget16_as_rgba() {
    const uint16_t v = fgetw(m_f);
    uint8_t alpha = 255;
    if (m_hasAlpha) {
      if ((v & 0x8000) == 0)    // Transparent bit
        alpha = 0;
      ++m_alphaHistogram[alpha];
    }
    return doc::rgba(scale_5bits_to_8bits((v >> 10) & 0x1F),
                     scale_5bits_to_8bits((v >> 5) & 0x1F),
                     scale_5bits_to_8bits(v & 0x1F),
                     alpha);
  }

  FILE* m_f;
  TgaHeader m_header;
  bool m_hasAlpha = false;
  Palette m_palette;
  ImageDataIterator m_iterator;
  std::vector<uint32_t> m_alphaHistogram;
};

} // anonymous namespace

bool TgaFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();

  TgaDecoder decoder(f);

  ImageSpec spec(ColorMode::RGB, 1, 1);
  if (!decoder.getImageSpec(spec)) {
    fop->setError("Unsupported color depth in TGA file: %d bpp, image type=%d.\n",
                  decoder.bitsPerPixel(),
                  decoder.imageType());
    return false;
  }

  if (!decoder.validPalType()) {
    fop->setError("Invalid palette type in TGA file.\n");
    return false;
  }

  // Palette from TGA file
  if (decoder.hasPalette()) {
    const Palette& pal = decoder.palette();
    for (int i=0; i<pal.size(); ++i) {
      color_t c = pal.getEntry(i);
      fop->sequenceSetColor(i,
                            doc::rgba_getr(c),
                            doc::rgba_getg(c),
                            doc::rgba_getb(c));
      if (doc::rgba_geta(c) < 255)
        fop->sequenceSetAlpha(i, doc::rgba_geta(c));
    }
  }
  // Generate grayscale palette
  else if (decoder.isGray()) {
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

  if (!decoder.readImageData(fop, image)) {
    fop->setError("Error loading image data from TGA file.\n");
    return false;
  }

  decoder.postProcessImageData(image);

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
                     const bool compressed) {
    m_header.idLength = 0;
    m_header.palType = 0;
    m_header.imageType = NoImage;
    m_header.palOrigin = 0;
    m_header.palLength = 0;
    m_header.palDepth = 0;
    m_header.xOrigin = 0;
    m_header.yOrigin = 0;
    m_header.width = image->width();
    m_header.height = image->height();
    // TODO make these options configurable
    m_header.bitsPerPixel = 0;
    m_header.imageDescriptor = 0x20; // Top-to-bottom

    switch (image->colorMode()) {
      case ColorMode::RGB:
        m_header.imageType = (compressed ? RleRgb: UncompressedRgb);
        m_header.bitsPerPixel = (isOpaque ? 24: 32);
        m_header.imageDescriptor |= (isOpaque ? 0: 8);
        break;
      case ColorMode::GRAYSCALE:
        // TODO if the grayscale is not opaque, we should use RGB,
        //      this could be done automatically in FileOp in a
        //      generic way for all formats when FILE_SUPPORT_RGBA is
        //      available and FILE_SUPPORT_GRAYA isn't.
        m_header.imageType = (compressed ? RleGray: UncompressedGray);
        m_header.bitsPerPixel = 8;
        break;
      case ColorMode::INDEXED:
        ASSERT(palette);

        m_header.imageType = (compressed ? RleIndexed: UncompressedIndexed);
        m_header.bitsPerPixel = 8;
        m_header.palType = 1;
        m_header.palLength = palette->size();
        if (palette->hasAlpha())
          m_header.palDepth = 32;
        else
          m_header.palDepth = 24;
        break;
    }
  }

  bool needsPalette() const {
    return (m_header.palType == 1);
  }

  void writeHeader() {
    fputc(m_header.idLength, m_f);
    fputc(m_header.palType, m_f);
    fputc(m_header.imageType, m_f);
    fputw(m_header.palOrigin, m_f);
    fputw(m_header.palLength, m_f);
    fputc(m_header.palDepth, m_f);
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
    ASSERT(palette->size() == m_header.palLength);

    for (int i=0; i<palette->size(); ++i) {
      color_t c = palette->getEntry(i);
      fputc(rgba_getb(c), m_f);
      fputc(rgba_getg(c), m_f);
      fputc(rgba_getr(c), m_f);
      if (m_header.palDepth == 32)
        fputc(rgba_geta(c), m_f);
    }
  }

  void writeImageData(FileOp* fop, const Image* image) {
    switch (m_header.imageType) {

      case UncompressedIndexed: {
        for (int y=0; y<image->height(); ++y) {
          for (int x=0; x<image->width(); ++x) {
            color_t c = get_pixel_fast<IndexedTraits>(image, x, y);
            fputc(c, m_f);
          }
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case UncompressedRgb: {
        for (int y=0; y<image->height(); ++y) {
          for (int x=0; x<image->width(); ++x) {
            color_t c = get_pixel_fast<RgbTraits>(image, x, y);
            fputc(rgba_getb(c), m_f);
            fputc(rgba_getg(c), m_f);
            fputc(rgba_getr(c), m_f);
            fputc(rgba_geta(c), m_f);
          }
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case UncompressedGray: {
        for (int y=0; y<image->height(); ++y) {
          for (int x=0; x<image->width(); ++x) {
            color_t c = get_pixel_fast<GrayscaleTraits>(image, x, y);
            fputc(graya_getv(c), m_f);
          }
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case RleIndexed: {
        for (int y=0; y<image->height(); ++y) {
          writeRleScanline<IndexedTraits>(image, y, &TgaEncoder::fput8);
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case RleRgb: {
        ASSERT(m_header.bitsPerPixel == 16 ||
               m_header.bitsPerPixel == 24 ||
               m_header.bitsPerPixel == 32);

        auto fput = (m_header.bitsPerPixel == 32 ? &TgaEncoder::fput32:
                     m_header.bitsPerPixel == 24 ? &TgaEncoder::fput24:
                                                   &TgaEncoder::fput16);

        for (int y=0; y<image->height(); ++y) {
          writeRleScanline<RgbTraits>(image, y, fput);
          fop->setProgress(float(y) / float(image->height()));
        }
        break;
      }

      case RleGray: {
        for (int y=0; y<image->height(); ++y) {
          writeRleScanline<GrayscaleTraits>(image, y, &TgaEncoder::fput8);
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
    uint16_t v = 0;
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
  TgaHeader m_header;
};

} // anonymous namespace

bool TgaFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  const Palette* palette = fop->sequenceGetPalette();

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  TgaEncoder encoder(f);

  encoder.prepareHeader(image,
                        palette,
                        fop->document()->sprite()->isOpaque(),
                        true);  // Always compressed?
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

} // namespace app
