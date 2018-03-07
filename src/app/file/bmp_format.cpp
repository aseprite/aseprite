// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//
// bmp.c - Based on the code of Seymour Shlien and Jonas Petersen.

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

class BmpFormat : public FileFormat {
  enum {
    BMP_OPTIONS_FORMAT_WINDOWS = 12,
    BMP_OPTIONS_FORMAT_OS2 = 40,
    BMP_OPTIONS_COMPRESSION_RGB = 0,
    BMP_OPTIONS_COMPRESSION_RLE8 = 1,
    BMP_OPTIONS_COMPRESSION_RLE4 = 2,
    BMP_OPTIONS_COMPRESSION_BITFIELDS = 3
  };

  // Data for BMP files
  class BmpOptions : public FormatOptions
  {
  public:
    int format;                 // bmp format.
    int compression;            // bmp compression.
    int bits_per_pixel;         // Bits per pixel.
    uint32_t red_mask;          // Mask for red channel.
    uint32_t green_mask;        // Mask for green channel.
    uint32_t blue_mask;         // Mask for blue channel.
  };

  const char* onGetName() const override {
    return "bmp";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("bmp");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::BMP_IMAGE;
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

FileFormat* CreateBmpFormat()
{
  return new BmpFormat;
}

#define BI_RGB          0
#define BI_RLE8         1
#define BI_RLE4         2
#define BI_BITFIELDS    3

#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40

typedef struct BITMAPFILEHEADER
{
  uint32_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
} BITMAPFILEHEADER;

// Used for both OS/2 and Windows BMP.
// Contains only the parameters needed to load the image.
typedef struct BITMAPINFOHEADER
{
  uint32_t biWidth;
  uint32_t biHeight;
  uint16_t biBitCount;
  uint32_t biCompression;
} BITMAPINFOHEADER;

typedef struct WINBMPINFOHEADER  // Size: 40.
{
  uint32_t biWidth;
  uint32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  uint32_t biXPelsPerMeter;
  uint32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
} WINBMPINFOHEADER;

typedef struct OS2BMPINFOHEADER  // Size: 12.
{
  uint16_t biWidth;
  uint16_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
} OS2BMPINFOHEADER;

/* read_bmfileheader:
 *  Reads a BMP file header and check that it has the BMP magic number.
 */
static int read_bmfileheader(FILE *f, BITMAPFILEHEADER *fileheader)
{
  fileheader->bfType = fgetw(f);
  fileheader->bfSize = fgetl(f);
  fileheader->bfReserved1 = fgetw(f);
  fileheader->bfReserved2 = fgetw(f);
  fileheader->bfOffBits = fgetl(f);

  if (fileheader->bfType != 19778)
    return -1;

  return 0;
}

/* read_win_bminfoheader:
 *  Reads information from a BMP file header.
 */
static int read_win_bminfoheader(FILE *f, BITMAPINFOHEADER *infoheader)
{
  WINBMPINFOHEADER win_infoheader;

  win_infoheader.biWidth = fgetl(f);
  win_infoheader.biHeight = fgetl(f);
  win_infoheader.biPlanes = fgetw(f);
  win_infoheader.biBitCount = fgetw(f);
  win_infoheader.biCompression = fgetl(f);
  win_infoheader.biSizeImage = fgetl(f);
  win_infoheader.biXPelsPerMeter = fgetl(f);
  win_infoheader.biYPelsPerMeter = fgetl(f);
  win_infoheader.biClrUsed = fgetl(f);
  win_infoheader.biClrImportant = fgetl(f);

  infoheader->biWidth = win_infoheader.biWidth;
  infoheader->biHeight = win_infoheader.biHeight;
  infoheader->biBitCount = win_infoheader.biBitCount;
  infoheader->biCompression = win_infoheader.biCompression;

  return 0;
}

/* read_os2_bminfoheader:
 *  Reads information from an OS/2 format BMP file header.
 */
static int read_os2_bminfoheader(FILE *f, BITMAPINFOHEADER *infoheader)
{
  OS2BMPINFOHEADER os2_infoheader;

  os2_infoheader.biWidth = fgetw(f);
  os2_infoheader.biHeight = fgetw(f);
  os2_infoheader.biPlanes = fgetw(f);
  os2_infoheader.biBitCount = fgetw(f);

  infoheader->biWidth = os2_infoheader.biWidth;
  infoheader->biHeight = os2_infoheader.biHeight;
  infoheader->biBitCount = os2_infoheader.biBitCount;
  infoheader->biCompression = 0;

  return 0;
}

/* read_bmicolors:
 *  Loads the color palette for 1,4,8 bit formats.
 */
static void read_bmicolors(FileOp* fop, int bytes, FILE *f, bool win_flag)
{
  int i, j, r, g, b;

  for (i=j=0; i+3 <= bytes && j < 256; ) {
    b = fgetc(f);
    g = fgetc(f);
    r = fgetc(f);

    fop->sequenceSetColor(j, r, g, b);

    j++;
    i += 3;

    if (win_flag && i < bytes) {
      fgetc(f);
      i++;
    }
  }

  for (; i<bytes; i++)
    fgetc(f);
}

/* read_1bit_line:
 *  Support function for reading the 1 bit bitmap file format.
 */
static void read_1bit_line(int length, FILE *f, Image *image, int line)
{
  unsigned char b[32];
  unsigned long n;
  int i, j, k;
  int pix;

  for (i=0; i<length; i++) {
    j = i % 32;
    if (j == 0) {
      n = fgetl(f);
      n =
        ((n&0x000000ff)<<24) |
        ((n&0x0000ff00)<< 8) |
        ((n&0x00ff0000)>> 8) |
        ((n&0xff000000)>>24);
      for (k=0; k<32; k++) {
        b[31-k] = (char)(n & 1);
        n = n >> 1;
      }
    }
    pix = b[j];
    put_pixel(image, i, line, pix);
  }
}

/* read_4bit_line:
 *  Support function for reading the 4 bit bitmap file format.
 */
static void read_4bit_line(int length, FILE *f, Image *image, int line)
{
  unsigned char b[8];
  unsigned long n;
  int i, j, k;
  int temp;
  int pix;

  for (i=0; i<length; i++) {
    j = i % 8;
    if (j == 0) {
      n = fgetl(f);
      for (k=0; k<4; k++) {
        temp = n & 255;
        b[k*2+1] = temp & 15;
        temp = temp >> 4;
        b[k*2] = temp & 15;
        n = n >> 8;
      }
    }
    pix = b[j];
    put_pixel(image, i, line, pix);
  }
}

/* read_8bit_line:
 *  Support function for reading the 8 bit bitmap file format.
 */
static void read_8bit_line(int length, FILE *f, Image *image, int line)
{
  unsigned char b[4];
  unsigned long n;
  int i, j, k;
  int pix;

  for (i=0; i<length; i++) {
    j = i % 4;
    if (j == 0) {
      n = fgetl(f);
      for (k=0; k<4; k++) {
        b[k] = (char)(n & 255);
        n = n >> 8;
      }
    }
    pix = b[j];
    put_pixel(image, i, line, pix);
  }
}

static void read_16bit_line(int length, FILE *f, Image *image, int line)
{
  int i, r, g, b, word;

  for (i=0; i<length; i++) {
    word = fgetw(f);

    r = (word >> 10) & 0x1f;
    g = (word >> 5) & 0x1f;
    b = (word) & 0x1f;

    put_pixel(image, i, line,
              rgba(scale_5bits_to_8bits(r),
                   scale_5bits_to_8bits(g),
                   scale_5bits_to_8bits(b), 255));
  }

  i = (2*i) % 4;
  if (i > 0)
    while (i++ < 4)
      fgetc(f);
}

static void read_24bit_line(int length, FILE *f, Image *image, int line)
{
  int i, r, g, b;

  for (i=0; i<length; i++) {
    b = fgetc(f);
    g = fgetc(f);
    r = fgetc(f);
    put_pixel(image, i, line, rgba(r, g, b, 255));
  }

  i = (3*i) % 4;
  if (i > 0)
    while (i++ < 4)
      fgetc(f);
}

static void read_32bit_line(int length, FILE *f, Image *image, int line)
{
  int i, r, g, b;

  for (i=0; i<length; i++) {
    b = fgetc(f);
    g = fgetc(f);
    r = fgetc(f);
    fgetc(f);
    put_pixel(image, i, line, rgba(r, g, b, 255));
  }
}

/* read_image:
 *  For reading the noncompressed BMP image format.
 */
static void read_image(FILE *f, Image *image, const BITMAPINFOHEADER *infoheader, FileOp *fop)
{
  int i, line, height, dir;

  height = (int)infoheader->biHeight;
  line   = height < 0 ? 0: height-1;
  dir    = height < 0 ? 1: -1;
  height = ABS(height);

  for (i=0; i<height; i++, line+=dir) {
    switch (infoheader->biBitCount) {
      case 1: read_1bit_line(infoheader->biWidth, f, image, line); break;
      case 4: read_4bit_line(infoheader->biWidth, f, image, line); break;
      case 8: read_8bit_line(infoheader->biWidth, f, image, line); break;
      case 16: read_16bit_line(infoheader->biWidth, f, image, line); break;
      case 24: read_24bit_line(infoheader->biWidth, f, image, line); break;
      case 32: read_32bit_line(infoheader->biWidth, f, image, line); break;
    }

    fop->setProgress((float)(i+1) / (float)(height));
    if (fop->isStop())
      break;
  }
}

/* read_rle8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 *
 * @note This support compressed top-down bitmaps, the MSDN says that
 *       they can't exist, but Photoshop can create them.
 */
static void read_rle8_compressed_image(FILE *f, Image *image, const BITMAPINFOHEADER *infoheader)
{
  unsigned char count, val, val0;
  int j, pos, line, height, dir;
  int eolflag, eopicflag;

  eopicflag = 0;

  height = (int)infoheader->biHeight;
  line   = height < 0 ? 0: height-1;
  dir    = height < 0 ? 1: -1;
  height = ABS(height);

  while (eopicflag == 0) {
    pos = 0;                               /* x position in bitmap */
    eolflag = 0;                           /* end of line flag */

    while ((eolflag == 0) && (eopicflag == 0)) {
      count = fgetc(f);
      val = fgetc(f);

      if (count > 0) {                    /* repeat pixel count times */
        for (j=0;j<count;j++) {
          put_pixel(image, pos, line, val);
          pos++;
        }
      }
      else {
        switch (val) {

          case 0:                       /* end of line flag */
            eolflag=1;
            break;

          case 1:                       /* end of picture flag */
            eopicflag=1;
            break;

          case 2:                       /* displace picture */
            count = fgetc(f);
            val = fgetc(f);
            pos += count;
            line += val*dir;
            break;

          default:                      /* read in absolute mode */
            for (j=0; j<val; j++) {
              val0 = fgetc(f);
              put_pixel(image, pos, line, val0);
              pos++;
            }

            if (j%2 == 1)
              val0 = fgetc(f);    /* align on word boundary */
            break;

        }
      }

      if (pos-1 > (int)infoheader->biWidth)
        eolflag=1;
    }

    line += dir;
    if (line < 0 || line >= height)
      eopicflag = 1;
  }
}

/* read_rle4_compressed_image:
 *  For reading the 4 bit RLE compressed BMP image format.
 *
 * @note This support compressed top-down bitmaps, the MSDN says that
 *       they can't exist, but Photoshop can create them.
 */
static void read_rle4_compressed_image(FILE *f, Image *image, const BITMAPINFOHEADER *infoheader)
{
  unsigned char b[8];
  unsigned char count;
  unsigned short val0, val;
  int j, k, pos, line, height, dir;
  int eolflag, eopicflag;

  eopicflag = 0;                            /* end of picture flag */

  height = (int)infoheader->biHeight;
  line   = height < 0 ? 0: height-1;
  dir    = height < 0 ? 1: -1;
  height = ABS(height);

  while (eopicflag == 0) {
    pos = 0;
    eolflag = 0;                           /* end of line flag */

    while ((eolflag == 0) && (eopicflag == 0)) {
      count = fgetc(f);
      val = fgetc(f);

      if (count > 0) {                    /* repeat pixels count times */
        b[1] = val & 15;
        b[0] = (val >> 4) & 15;
        for (j=0; j<count; j++) {
          put_pixel(image, pos, line, b[j%2]);
          pos++;
        }
      }
      else {
        switch (val) {

          case 0:                       /* end of line */
            eolflag=1;
            break;

          case 1:                       /* end of picture */
            eopicflag=1;
            break;

          case 2:                       /* displace image */
            count = fgetc(f);
            val = fgetc(f);
            pos += count;
            line += val*dir;
            break;

          default:                      /* read in absolute mode */
            for (j=0; j<val; j++) {
              if ((j%4) == 0) {
                val0 = fgetw(f);
                for (k=0; k<2; k++) {
                  b[2*k+1] = val0 & 15;
                  val0 = val0 >> 4;
                  b[2*k] = val0 & 15;
                  val0 = val0 >> 4;
                }
              }
              put_pixel(image, pos, line, b[j%4]);
              pos++;
            }
            break;
        }
      }

      if (pos-1 > (int)infoheader->biWidth)
        eolflag=1;
    }

    line += dir;
    if (line < 0 || line >= height)
      eopicflag = 1;
  }
}

static int read_bitfields_image(FILE *f, Image *image, BITMAPINFOHEADER *infoheader,
                                unsigned long rmask, unsigned long gmask, unsigned long bmask)
{
#define CALC_SHIFT(c)                           \
  mask = ~c##mask;                              \
  c##shift = 0;                                 \
  while (mask & 1) {                            \
    ++c##shift;                                 \
    mask >>= 1;                                 \
  }                                             \
  if ((c##mask >> c##shift) == 0x1f)            \
    c##scale = scale_5bits_to_8bits;            \
  else if ((c##mask >> c##shift) == 0x3f)       \
    c##scale = scale_6bits_to_8bits;            \
  else                                          \
    c##scale = NULL;

  unsigned long buffer, mask, rshift, gshift, bshift;
  int i, j, k, line, height, dir, r, g, b;
  int (*rscale)(int);
  int (*gscale)(int);
  int (*bscale)(int);
  int bits_per_pixel;
  int bytes_per_pixel;

  height = (int)infoheader->biHeight;
  line   = height < 0 ? 0: height-1;
  dir    = height < 0 ? 1: -1;
  height = ABS(height);

  /* calculate shifts */
  CALC_SHIFT(r);
  CALC_SHIFT(g);
  CALC_SHIFT(b);

  /* calculate bits-per-pixel and bytes-per-pixel */
  bits_per_pixel = infoheader->biBitCount;
  bytes_per_pixel = ((bits_per_pixel / 8) +
                     ((bits_per_pixel % 8) > 0 ? 1: 0));

  for (i=0; i<height; i++, line+=dir) {
    for (j=0; j<(int)infoheader->biWidth; j++) {
      /* read the DWORD, WORD or BYTE in little-endian order */
      buffer = 0;
      for (k=0; k<bytes_per_pixel; k++)
        buffer |= fgetc(f) << (k<<3);

      r = (buffer & rmask) >> rshift;
      g = (buffer & gmask) >> gshift;
      b = (buffer & bmask) >> bshift;

      r = rscale ? rscale(r): r;
      g = gscale ? gscale(g): g;
      b = bscale ? bscale(b): b;

      put_pixel_fast<RgbTraits>(image, j, line, rgba(r, g, b, 255));
    }

    j = (bytes_per_pixel*j) % 4;
    if (j > 0)
      while (j++ < 4)
        fgetc(f);
  }

  return 0;
}

bool BmpFormat::onLoad(FileOp *fop)
{
  unsigned long rmask, gmask, bmask;
  BITMAPFILEHEADER fileheader;
  BITMAPINFOHEADER infoheader;
  unsigned long biSize;
  PixelFormat pixelFormat;
  int format;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();

  if (read_bmfileheader(f, &fileheader) != 0)
    return false;

  biSize = fgetl(f);

  if (biSize == WININFOHEADERSIZE) {
    format = BMP_OPTIONS_FORMAT_WINDOWS;

    if (read_win_bminfoheader(f, &infoheader) != 0) {
      return false;
    }
    if (infoheader.biCompression != BI_BITFIELDS)
      read_bmicolors(fop, fileheader.bfOffBits - 54, f, true);
  }
  else if (biSize == OS2INFOHEADERSIZE) {
    format = BMP_OPTIONS_FORMAT_OS2;

    if (read_os2_bminfoheader(f, &infoheader) != 0) {
      return false;
    }
    /* compute number of colors recorded */
    if (infoheader.biCompression != BI_BITFIELDS)
      read_bmicolors(fop, fileheader.bfOffBits - 26, f, false);
  }
  else {
    return false;
  }

  if ((infoheader.biBitCount == 32) ||
      (infoheader.biBitCount == 24) ||
      (infoheader.biBitCount == 16))
    pixelFormat = IMAGE_RGB;
  else
    pixelFormat = IMAGE_INDEXED;

  /* bitfields have the 'mask' for each component */
  if (infoheader.biCompression == BI_BITFIELDS) {
    rmask = fgetl(f);
    gmask = fgetl(f);
    bmask = fgetl(f);
  }
  else
    rmask = gmask = bmask = 0;

  Image* image = fop->sequenceImage(pixelFormat,
                                    infoheader.biWidth,
                                    ABS((int)infoheader.biHeight));
  if (!image) {
    return false;
  }

  if (pixelFormat == IMAGE_RGB)
    clear_image(image, rgba(0, 0, 0, 255));
  else
    clear_image(image, 0);

  switch (infoheader.biCompression) {

    case BI_RGB:
      read_image(f, image, &infoheader, fop);
      break;

    case BI_RLE8:
      read_rle8_compressed_image(f, image, &infoheader);
      break;

    case BI_RLE4:
      read_rle4_compressed_image(f, image, &infoheader);
      break;

    case BI_BITFIELDS:
      if (read_bitfields_image(f, image, &infoheader, rmask, gmask, bmask) < 0) {
        fop->setError("Unsupported bitfields in the BMP file.\n");
        return false;
      }
      break;

    default:
      fop->setError("Unsupported BMP compression.\n");
      return false;
  }

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }

  // Setup the file-data.
  if (!fop->formatOptions()) {
    base::SharedPtr<BmpOptions> bmp_options(new BmpOptions());

    bmp_options->format = format;
    bmp_options->compression = infoheader.biCompression;
    bmp_options->bits_per_pixel = infoheader.biBitCount;
    bmp_options->red_mask = rmask;
    bmp_options->green_mask = gmask;
    bmp_options->blue_mask = bmask;

    fop->setFormatOptions(bmp_options);
  }

  return true;
}

#ifdef ENABLE_SAVE
bool BmpFormat::onSave(FileOp *fop)
{
  const Image* image = fop->sequenceImage();
  int bfSize;
  int biSizeImage;
  int bpp = (image->pixelFormat() == IMAGE_RGB) ? 24 : 8;
  int filler = 3 - ((image->width()*(bpp/8)-1) & 3);
  int c, i, j, r, g, b;

  if (bpp == 8) {
    biSizeImage = (image->width() + filler) * image->height();
    bfSize = (54                      /* header */
              + 256*4                 /* palette */
              + biSizeImage);         /* image data */
  }
  else {
    biSizeImage = (image->width()*3 + filler) * image->height();
    bfSize = 54 + biSizeImage;       /* header + image data */
  }

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  /* file_header */
  fputw(0x4D42, f);              /* bfType ("BM") */
  fputl(bfSize, f);              /* bfSize */
  fputw(0, f);                   /* bfReserved1 */
  fputw(0, f);                   /* bfReserved2 */

  if (bpp == 8)                 /* bfOffBits */
    fputl(54+256*4, f);
  else
    fputl(54, f);

  /* info_header */
  fputl(40, f);                  /* biSize */
  fputl(image->width(), f);   /* biWidth */
  fputl(image->height(), f);  /* biHeight */
  fputw(1, f);                   /* biPlanes */
  fputw(bpp, f);                 /* biBitCount */
  fputl(0, f);                   /* biCompression */
  fputl(biSizeImage, f);         /* biSizeImage */
  fputl(0xB12, f);               /* biXPelsPerMeter (0xB12 = 72 dpi) */
  fputl(0xB12, f);               /* biYPelsPerMeter */

  if (bpp == 8) {
    fputl(256, f);              /* biClrUsed */
    fputl(256, f);              /* biClrImportant */

    /* palette */
    for (i=0; i<256; i++) {
      fop->sequenceGetColor(i, &r, &g, &b);
      fputc(b, f);
      fputc(g, f);
      fputc(r, f);
      fputc(0, f);
    }
  }
  else {
    fputl(0, f);                /* biClrUsed */
    fputl(0, f);                /* biClrImportant */
  }

  /* image data */
  for (i=image->height()-1; i>=0; i--) {
    for (j=0; j<image->width(); j++) {
      if (bpp == 8) {
        if (image->pixelFormat() == IMAGE_INDEXED)
          fputc(get_pixel_fast<IndexedTraits>(image, j, i), f);
        else if (image->pixelFormat() == IMAGE_GRAYSCALE)
          fputc(graya_getv(get_pixel_fast<GrayscaleTraits>(image, j, i)), f);
      }
      else {
        c = get_pixel_fast<RgbTraits>(image, j, i);
        fputc(rgba_getb(c), f);
        fputc(rgba_getg(c), f);
        fputc(rgba_getr(c), f);
      }
    }

    for (j=0; j<filler; j++)
      fputc(0, f);

    fop->setProgress((float)(image->height()-i) / (float)image->height());
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
