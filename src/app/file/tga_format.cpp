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

class TgaDecoder {
  struct Header {
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

  enum ImageType {
    NoImage = 0,
    UncompressedIndexed = 1,
    UncompressedRgb = 2,
    UncompressedGray = 3,
    RleIndexed = 9,
    RleRgb = 10,
    RleGray = 11,
  };

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
      if ((v & 0x8000) == 0)
        alpha = 0;
      ++m_alphaHistogram[alpha];
    }
    return doc::rgba(scale_5bits_to_8bits((v >> 10) & 0x1F),
                     scale_5bits_to_8bits((v >> 5) & 0x1F),
                     scale_5bits_to_8bits(v & 0x1F),
                     alpha);
  }

  FILE* m_f;
  Header m_header;
  bool m_hasAlpha = false;
  Palette m_palette;
  ImageDataIterator m_iterator;
  std::vector<uint32_t> m_alphaHistogram;
};

}

// Loads a 256 color or 24 bit uncompressed TGA file, returning a bitmap
// structure and storing the palette data in the specified palette (this
// should be an array of at least 256 RGB structures).
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
// Writes a bitmap into a TGA file, using the specified palette (this
// should be an array of at least 256 RGB structures).
bool TgaFormat::onSave(FileOp* fop)
{
  const Image* image = fop->sequenceImage();
  unsigned char image_palette[256][3];
  int x, y, c, r, g, b;
  int depth = (image->pixelFormat() == IMAGE_RGB) ? 32 : 8;
  bool need_pal = (image->pixelFormat() == IMAGE_INDEXED)? true: false;

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  fputc(0, f);                          // id length (no id saved)
  fputc((need_pal) ? 1 : 0, f);         // palette type
  // image type
  fputc((image->pixelFormat() == IMAGE_RGB      ) ? 2 :
        (image->pixelFormat() == IMAGE_GRAYSCALE) ? 3 :
        (image->pixelFormat() == IMAGE_INDEXED  ) ? 1 : 0, f);
  fputw(0, f);                         // first colour
  fputw((need_pal) ? 256 : 0, f);      // number of colours
  fputc((need_pal) ? 24 : 0, f);       // palette entry size
  fputw(0, f);                         // left
  fputw(0, f);                         // top
  fputw(image->width(), f);            // width
  fputw(image->height(), f);           // height
  fputc(depth, f);                     // bits per pixel

  // descriptor (bottom to top, 8-bit alpha)
  fputc(
    (image->pixelFormat() == IMAGE_RGB &&
     !fop->document()->sprite()->isOpaque() ? 8: 0), f);

  if (need_pal) {
    for (y=0; y<256; y++) {
      fop->sequenceGetColor(y, &r, &g, &b);
      image_palette[y][2] = r;
      image_palette[y][1] = g;
      image_palette[y][0] = b;
    }
    fwrite(image_palette, 1, 768, f);
  }

  switch (image->pixelFormat()) {

    case IMAGE_RGB:
      for (y=image->height()-1; y>=0; y--) {
        for (x=0; x<image->width(); x++) {
          c = get_pixel(image, x, y);
          fputc(rgba_getb(c), f);
          fputc(rgba_getg(c), f);
          fputc(rgba_getr(c), f);
          fputc(rgba_geta(c), f);
        }

        fop->setProgress((float)(image->height()-y) / (float)(image->height()));
      }
      break;

    case IMAGE_GRAYSCALE:
      for (y=image->height()-1; y>=0; y--) {
        for (x=0; x<image->width(); x++)
          fputc(graya_getv(get_pixel(image, x, y)), f);

        fop->setProgress((float)(image->height()-y) / (float)(image->height()));
      }
      break;

    case IMAGE_INDEXED:
      for (y=image->height()-1; y>=0; y--) {
        for (x=0; x<image->width(); x++)
          fputc(get_pixel(image, x, y), f);

        fop->setProgress((float)(image->height()-y) / (float)(image->height()));
      }
      break;
  }

  const char* tga2_footer = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0";
  fwrite(tga2_footer, 1, 26, f);

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
