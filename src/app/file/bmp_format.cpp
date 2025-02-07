// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.
//
// bmp.c - Based on the code of Seymour Shlien and Jonas Petersen.
//
// Info about BMP format:
// https://en.wikipedia.org/wiki/BMP_file_format
// http://justsolve.archiveteam.org/wiki/BMP
// https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-header-types

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/cfile.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "fmt/format.h"

namespace app {

// Max supported .bmp size (to filter out invalid image sizes)
const uint32_t kMaxBmpSize = 1024 * 1024 * 128; // 128 MB

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
  class BmpOptions : public FormatOptions {
  public:
    int format;          // bmp format.
    int compression;     // bmp compression.
    int bits_per_pixel;  // Bits per pixel.
    uint32_t red_mask;   // Mask for red channel.
    uint32_t green_mask; // Mask for green channel.
    uint32_t blue_mask;  // Mask for blue channel.
    uint32_t alpha_mask; // Mask for alpha channel.
  };

  const char* onGetName() const override { return "bmp"; }

  void onGetExtensions(base::paths& exts) const override { exts.push_back("bmp"); }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::BMP_IMAGE; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_RGB | FILE_SUPPORT_RGBA |
           FILE_SUPPORT_GRAY | FILE_SUPPORT_INDEXED | FILE_SUPPORT_SEQUENCES |
           FILE_ENCODE_ABSTRACT_IMAGE;
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

#define BI_RGB               0
#define BI_RLE8              1
#define BI_RLE4              2
#define BI_BITFIELDS         3
#define BI_ALPHABITFIELDS    6

#define OS2FILEHEADERSIZE    14

#define OS2INFOHEADERSIZE    12
#define OS22INFOHEADERSIZE16 16
#define OS22INFOHEADERSIZE24 24
#define WININFOHEADERSIZE    40
#define BV2INFOHEADERSIZE    52
#define BV3INFOHEADERSIZE    56
#define OS22INFOHEADERSIZE60 60
#define OS22INFOHEADERSIZE64 64
#define BV4INFOHEADERSIZE    108
#define BV5INFOHEADERSIZE    124

struct BITMAPFILEHEADER {
  uint32_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
};

// Used for all Info Header Sizes.
// Contains only the parameters needed to load the image.
struct BITMAPINFOHEADER {
  uint32_t biSize;
  uint32_t biWidth;
  uint32_t biHeight;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biClrUsed;
  uint32_t rMask;
  uint32_t gMask;
  uint32_t bMask;
  uint32_t aMask;

  bool isRGBMasks() const
  {
    return biSize >= BV2INFOHEADERSIZE || biCompression == BI_BITFIELDS ||
           biCompression == BI_ALPHABITFIELDS;
  };
  bool isAlphaMask() const
  {
    return biSize >= BV3INFOHEADERSIZE || biCompression == BI_ALPHABITFIELDS;
  };
};

struct WINBMPINFOHEADER // Size: 16 to 64
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

  uint32_t redMask;
  uint32_t greenMask;
  uint32_t blueMask;
  uint32_t alphaMask;
};

struct OS2BMPINFOHEADER // Size: 12.
{
  uint16_t biWidth;
  uint16_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
};

// TO DO: support ICC profiles and colorimetry
struct CIEXYZ {
  uint32_t ciexyzX; // Fix Point: 2 bits integer part, 30 bits to fractional part
  uint32_t ciexyzY;
  uint32_t ciexyzZ;
};

struct CIEXYZTRIPLE {
  CIEXYZ ciexyzRed;
  CIEXYZ ciexyzGreen;
  CIEXYZ ciexyzBlue;
};

struct BMPV4HEADER // Size: 108.
{
  uint32_t bV4Width;
  uint32_t bV4Height;
  uint16_t bV4Planes;
  uint16_t bV4BitCount;
  uint32_t bV4Compression;
  uint32_t bV4SizeImage;
  uint32_t bV4XPelsPerMeter;
  uint32_t bV4YPelsPerMeter;
  uint32_t bV4ClrUsed;
  uint32_t bV4ClrImportant;

  uint32_t bV4RedMask;
  uint32_t bV4GreenMask;
  uint32_t bV4BlueMask;
  uint32_t bV4AlphaMask;
  // TO DO: support ICC profiles and colorimetry
  uint32_t bV4CSType;
  CIEXYZTRIPLE bV4Endpoints;
  uint32_t bV4GammaRed;
  uint32_t bV4GammaGreen;
  uint32_t bV4GammaBlue;
};

struct BMPV5HEADER // Size: 124.
{
  uint32_t bV5Width;
  uint32_t bV5Height;
  uint16_t bV5Planes;
  uint16_t bV5BitCount;
  uint32_t bV5Compression;

  uint32_t bV5SizeImage;
  uint32_t bV5XPelsPerMeter;
  uint32_t bV5YPelsPerMeter;
  uint32_t bV5ClrUsed;
  uint32_t bV5ClrImportant;

  uint32_t bV5RedMask;
  uint32_t bV5GreenMask;
  uint32_t bV5BlueMask;
  uint32_t bV5AlphaMask;
  // TO DO: support ICC profiles and colorimetry
  uint32_t bV5CSType;
  CIEXYZTRIPLE bV5Endpoints;

  uint32_t bV5GammaRed;
  uint32_t bV5GammaGreen;
  uint32_t bV5GammaBlue;

  uint32_t bV5Intent;
  uint32_t bV5ProfileData;
  uint32_t bV5ProfileSize;
  uint32_t bV5Reserved;
};

/* read_bmfileheader:
 *  Reads a BMP file header and check that it has the BMP magic number.
 */
static int read_bmfileheader(FILE* f, BITMAPFILEHEADER* fileheader)
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
static int read_win_bminfoheader(FILE* f, BITMAPINFOHEADER* infoheader)
{
  WINBMPINFOHEADER win_infoheader;

  int biSize = infoheader->biSize;

  if (biSize != OS22INFOHEADERSIZE16 && biSize != OS22INFOHEADERSIZE24 &&
      biSize != WININFOHEADERSIZE && biSize != BV2INFOHEADERSIZE && biSize != BV3INFOHEADERSIZE &&
      biSize != OS22INFOHEADERSIZE60 && biSize != OS22INFOHEADERSIZE64)
    return -1;

  win_infoheader.biWidth = fgetl(f);
  win_infoheader.biHeight = fgetl(f);
  win_infoheader.biPlanes = fgetw(f);
  win_infoheader.biBitCount = fgetw(f); // = 16 bytes

  win_infoheader.redMask = 0;
  win_infoheader.greenMask = 0;
  win_infoheader.blueMask = 0;
  win_infoheader.alphaMask = 0;
  if (biSize == OS22INFOHEADERSIZE16)
    win_infoheader.biCompression = BI_RGB; // = 16 bytes
  else {
    ASSERT(biSize >= OS22INFOHEADERSIZE24);
    win_infoheader.biCompression = fgetl(f);
    win_infoheader.biSizeImage = fgetl(f); // = 24 bytes

    if (biSize >= WININFOHEADERSIZE) {
      win_infoheader.biXPelsPerMeter = fgetl(f);
      win_infoheader.biYPelsPerMeter = fgetl(f);
      win_infoheader.biClrUsed = fgetl(f);
      win_infoheader.biClrImportant = fgetl(f); // = 40 bytes (WININFOHEADERSIZE)

      // 'biCompression' is needed to execute
      // infoheader->isRGBMasks() and infoheader->isAlphaMask()
      infoheader->biCompression = win_infoheader.biCompression;

      if (infoheader->isRGBMasks()) {
        win_infoheader.redMask = fgetl(f);
        win_infoheader.greenMask = fgetl(f);
        win_infoheader.blueMask = fgetl(f); // = 52 bytes (BV2INFOHEADERSIZE)
        if (infoheader->isAlphaMask()) {
          win_infoheader.alphaMask = fgetl(f); // = 56 bytes (BV3INFOHEADERSIZE)
          if (biSize >= OS22INFOHEADERSIZE60) {
            fgetl(f); // <--discarded         // = 60 bytes
            if (biSize == OS22INFOHEADERSIZE64)
              fgetl(f); // <--discarded         // = 64 bytes
          }
        }
      }
    }
  }

  infoheader->biWidth = win_infoheader.biWidth;
  infoheader->biHeight = win_infoheader.biHeight;
  infoheader->biBitCount = win_infoheader.biBitCount;
  infoheader->biCompression = win_infoheader.biCompression;
  infoheader->biClrUsed = win_infoheader.biClrUsed;
  infoheader->rMask = win_infoheader.redMask;
  infoheader->gMask = win_infoheader.greenMask;
  infoheader->bMask = win_infoheader.blueMask;
  infoheader->aMask = win_infoheader.alphaMask;

  return 0;
}

/* read_os2_bminfoheader:
 *  Reads information from an OS/2 format BMP file header.
 */
static int read_os2_bminfoheader(FILE* f, BITMAPINFOHEADER* infoheader)
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
  infoheader->biClrUsed = -1; // Not defined in this format

  return 0;
}

/* read_v4_bminfoheader:
 *  Reads information from an V4 format BMP file header.
 */
static int read_v4_bminfoheader(FILE* f, BITMAPINFOHEADER* infoheader)
{
  BMPV4HEADER v4_infoheader;

  v4_infoheader.bV4Width = fgetl(f);
  v4_infoheader.bV4Height = fgetl(f);
  v4_infoheader.bV4Planes = fgetw(f);
  v4_infoheader.bV4BitCount = fgetw(f);
  v4_infoheader.bV4Compression = fgetl(f);

  v4_infoheader.bV4SizeImage = fgetl(f);
  v4_infoheader.bV4XPelsPerMeter = fgetl(f);
  v4_infoheader.bV4YPelsPerMeter = fgetl(f);
  v4_infoheader.bV4ClrUsed = fgetl(f);
  v4_infoheader.bV4ClrImportant = fgetl(f);

  v4_infoheader.bV4RedMask = fgetl(f);
  v4_infoheader.bV4GreenMask = fgetl(f);
  v4_infoheader.bV4BlueMask = fgetl(f);
  v4_infoheader.bV4AlphaMask = fgetl(f);

  // TO DO: support ICC profiles and colorimetry
  v4_infoheader.bV4CSType = fgetl(f);

  // CIEXYZTRIPLE {
  v4_infoheader.bV4Endpoints.ciexyzRed.ciexyzX = fgetl(f);
  v4_infoheader.bV4Endpoints.ciexyzRed.ciexyzY = fgetl(f);
  v4_infoheader.bV4Endpoints.ciexyzRed.ciexyzZ = fgetl(f);

  v4_infoheader.bV4Endpoints.ciexyzGreen.ciexyzX = fgetl(f);
  v4_infoheader.bV4Endpoints.ciexyzGreen.ciexyzY = fgetl(f);
  v4_infoheader.bV4Endpoints.ciexyzGreen.ciexyzZ = fgetl(f);

  v4_infoheader.bV4Endpoints.ciexyzBlue.ciexyzX = fgetl(f);
  v4_infoheader.bV4Endpoints.ciexyzBlue.ciexyzY = fgetl(f);
  v4_infoheader.bV4Endpoints.ciexyzBlue.ciexyzZ = fgetl(f);
  // } CIEXYZTRIPLE

  v4_infoheader.bV4GammaRed = fgetl(f);
  v4_infoheader.bV4GammaGreen = fgetl(f);
  v4_infoheader.bV4GammaBlue = fgetl(f);

  infoheader->biWidth = v4_infoheader.bV4Width;
  infoheader->biHeight = v4_infoheader.bV4Height;
  infoheader->biBitCount = v4_infoheader.bV4BitCount;
  infoheader->biCompression = v4_infoheader.bV4Compression;
  infoheader->biClrUsed = v4_infoheader.bV4ClrUsed;

  infoheader->rMask = v4_infoheader.bV4RedMask;
  infoheader->gMask = v4_infoheader.bV4GreenMask;
  infoheader->bMask = v4_infoheader.bV4BlueMask;
  infoheader->aMask = v4_infoheader.bV4AlphaMask;

  return 0;
}

/* read_v5_bminfoheader:
 *  Reads information from an V5 format BMP file header.
 */
static int read_v5_bminfoheader(FILE* f, BITMAPINFOHEADER* infoheader)
{
  BMPV5HEADER v5_infoheader;

  v5_infoheader.bV5Width = fgetl(f);
  v5_infoheader.bV5Height = fgetl(f);
  v5_infoheader.bV5Planes = fgetw(f);
  v5_infoheader.bV5BitCount = fgetw(f);
  v5_infoheader.bV5Compression = fgetl(f);

  v5_infoheader.bV5SizeImage = fgetl(f);
  v5_infoheader.bV5XPelsPerMeter = fgetl(f);
  v5_infoheader.bV5YPelsPerMeter = fgetl(f);
  v5_infoheader.bV5ClrUsed = fgetl(f);
  v5_infoheader.bV5ClrImportant = fgetl(f);

  v5_infoheader.bV5RedMask = fgetl(f);
  v5_infoheader.bV5GreenMask = fgetl(f);
  v5_infoheader.bV5BlueMask = fgetl(f);
  v5_infoheader.bV5AlphaMask = fgetl(f);

  // TO DO: support ICC profiles and colorimetry
  v5_infoheader.bV5CSType = fgetl(f);

  // CIEXYZTRIPLE {
  v5_infoheader.bV5Endpoints.ciexyzRed.ciexyzX = fgetl(f);
  v5_infoheader.bV5Endpoints.ciexyzRed.ciexyzY = fgetl(f);
  v5_infoheader.bV5Endpoints.ciexyzRed.ciexyzZ = fgetl(f);

  v5_infoheader.bV5Endpoints.ciexyzGreen.ciexyzX = fgetl(f);
  v5_infoheader.bV5Endpoints.ciexyzGreen.ciexyzY = fgetl(f);
  v5_infoheader.bV5Endpoints.ciexyzGreen.ciexyzZ = fgetl(f);

  v5_infoheader.bV5Endpoints.ciexyzBlue.ciexyzX = fgetl(f);
  v5_infoheader.bV5Endpoints.ciexyzBlue.ciexyzY = fgetl(f);
  v5_infoheader.bV5Endpoints.ciexyzBlue.ciexyzZ = fgetl(f);
  // } CIEXYZTRIPLE

  v5_infoheader.bV5GammaRed = fgetl(f);
  v5_infoheader.bV5GammaGreen = fgetl(f);
  v5_infoheader.bV5GammaBlue = fgetl(f);

  v5_infoheader.bV5Intent = fgetl(f);
  v5_infoheader.bV5ProfileData = fgetl(f);
  v5_infoheader.bV5ProfileSize = fgetl(f);
  fgetl(f); // <-- Reserved DWORD

  infoheader->biWidth = v5_infoheader.bV5Width;
  infoheader->biHeight = v5_infoheader.bV5Height;
  infoheader->biBitCount = v5_infoheader.bV5BitCount;
  infoheader->biCompression = v5_infoheader.bV5Compression;
  infoheader->biClrUsed = v5_infoheader.bV5ClrUsed;

  infoheader->rMask = v5_infoheader.bV5RedMask;
  infoheader->gMask = v5_infoheader.bV5GreenMask;
  infoheader->bMask = v5_infoheader.bV5BlueMask;
  infoheader->aMask = v5_infoheader.bV5AlphaMask;

  return 0;
}

/* read_bmicolors:
 *  Loads the color palette for 1,4,8 bit formats.
 */
static void read_bmicolors(FileOp* fop, int bytes, FILE* f, bool win_flag)
{
  int i, j, r, g, b;

  for (i = j = 0; i + 3 <= bytes && j < 256;) {
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

  // Set the number of colors in the palette
  fop->sequenceSetNColors(j);

  for (; i < bytes; i++)
    fgetc(f);
}

/* read_1bit_line:
 *  Support function for reading the 1 bit bitmap file format.
 */
static void read_1bit_line(int length, FILE* f, Image* image, int line)
{
  unsigned char b[32];
  unsigned long n;
  int i, j, k;
  int pix;

  for (i = 0; i < length; i++) {
    j = i % 32;
    if (j == 0) {
      n = fgetl(f);
      n = ((n & 0x000000ff) << 24) | ((n & 0x0000ff00) << 8) | ((n & 0x00ff0000) >> 8) |
          ((n & 0xff000000) >> 24);
      for (k = 0; k < 32; k++) {
        b[31 - k] = (char)(n & 1);
        n = n >> 1;
      }
    }
    pix = b[j];
    put_pixel(image, i, line, pix);
  }
}

/* read_2bit_line (not standard):
 *  Support function for reading the 2 bit bitmap file format.
 */
static void read_2bit_line(int length, FILE* f, Image* image, int line)
{
  unsigned char b[16];
  unsigned long n;
  int i, j, k;
  int temp;
  int pix;

  for (i = 0; i < length; i++) {
    j = i % 16;
    if (j == 0) {
      n = fgetl(f);
      for (k = 0; k < 4; k++) {
        temp = n & 255;
        b[k * 4 + 3] = temp & 3;
        temp = temp >> 2;
        b[k * 4 + 2] = temp & 3;
        temp = temp >> 2;
        b[k * 4 + 1] = temp & 3;
        temp = temp >> 2;
        b[k * 4] = temp & 3;
        n = n >> 8;
      }
    }
    pix = b[j];
    put_pixel(image, i, line, pix);
  }
}

/* read_4bit_line:
 *  Support function for reading the 4 bit bitmap file format.
 */
static void read_4bit_line(int length, FILE* f, Image* image, int line)
{
  unsigned char b[8];
  unsigned long n;
  int i, j, k;
  int temp;
  int pix;

  for (i = 0; i < length; i++) {
    j = i % 8;
    if (j == 0) {
      n = fgetl(f);
      for (k = 0; k < 4; k++) {
        temp = n & 255;
        b[k * 2 + 1] = temp & 15;
        temp = temp >> 4;
        b[k * 2] = temp & 15;
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
static void read_8bit_line(int length, FILE* f, Image* image, int line)
{
  unsigned char b[4];
  unsigned long n;
  int i, j, k;
  int pix;

  for (i = 0; i < length; i++) {
    j = i % 4;
    if (j == 0) {
      n = fgetl(f);
      for (k = 0; k < 4; k++) {
        b[k] = (char)(n & 255);
        n = n >> 8;
      }
    }
    pix = b[j];
    put_pixel(image, i, line, pix);
  }
}

static void read_16bit_line(int length, FILE* f, Image* image, int line, bool& withAlpha)
{
  int i, r, g, b, a, word;

  for (i = 0; i < length; i++) {
    word = fgetw(f);

    r = (word >> 10) & 0x1f;
    g = (word >> 5) & 0x1f;
    b = (word) & 0x1f;
    a = (word & 0x8000 ? 255 : 0);
    if (a)
      withAlpha = true;
    put_pixel(image,
              i,
              line,
              rgba(scale_5bits_to_8bits(r), scale_5bits_to_8bits(g), scale_5bits_to_8bits(b), a));
  }

  i = (2 * i) % 4;
  if (i > 0)
    while (i++ < 4)
      fgetc(f);
}

static void read_24bit_line(int length, FILE* f, Image* image, int line)
{
  int i, r, g, b;

  for (i = 0; i < length; i++) {
    b = fgetc(f);
    g = fgetc(f);
    r = fgetc(f);
    put_pixel(image, i, line, rgba(r, g, b, 255));
  }

  i = (3 * i) % 4;
  if (i > 0)
    while (i++ < 4)
      fgetc(f);
}

static void read_32bit_line(int length, FILE* f, Image* image, int line, bool& withAlpha)
{
  int i, r, g, b, a;

  for (i = 0; i < length; i++) {
    b = fgetc(f);
    g = fgetc(f);
    r = fgetc(f);
    a = fgetc(f);
    if (a)
      withAlpha = true;
    put_pixel(image, i, line, rgba(r, g, b, a));
  }
}

/* read_image:
 *  For reading the noncompressed BMP image format.
 */
static void read_image(FILE* f,
                       Image* image,
                       const BITMAPINFOHEADER* infoheader,
                       FileOp* fop,
                       bool& withAlpha)
{
  int i, line, height, dir;

  height = (int)infoheader->biHeight;
  line = height < 0 ? 0 : height - 1;
  dir = height < 0 ? 1 : -1;
  height = ABS(height);

  for (i = 0; i < height; i++, line += dir) {
    switch (infoheader->biBitCount) {
      case 1:  read_1bit_line(infoheader->biWidth, f, image, line); break;
      case 2:  read_2bit_line(infoheader->biWidth, f, image, line); break;
      case 4:  read_4bit_line(infoheader->biWidth, f, image, line); break;
      case 8:  read_8bit_line(infoheader->biWidth, f, image, line); break;
      case 16: read_16bit_line(infoheader->biWidth, f, image, line, withAlpha); break;
      case 24: read_24bit_line(infoheader->biWidth, f, image, line); break;
      case 32: read_32bit_line(infoheader->biWidth, f, image, line, withAlpha); break;
    }

    fop->setProgress((float)(i + 1) / (float)(height));
    if (fop->isStop())
      break;
  }

  if ((infoheader->biBitCount == 32 || infoheader->biBitCount == 16) && !withAlpha) {
    LockImageBits<RgbTraits> imageBits(image, image->bounds());
    auto imgIt = imageBits.begin(), imgEnd = imageBits.end();
    for (; imgIt != imgEnd; ++imgIt)
      *imgIt |= 0xff000000;
  }
}

/* read_rle8_compressed_image:
 *  For reading the 8 bit RLE compressed BMP image format.
 *
 * @note This support compressed top-down bitmaps, the MSDN says that
 *       they can't exist, but Photoshop can create them.
 */
static void read_rle8_compressed_image(FILE* f, Image* image, const BITMAPINFOHEADER* infoheader)
{
  unsigned char count, val, val0;
  int j, pos, line, height, dir;
  int eolflag, eopicflag;

  eopicflag = 0;

  height = (int)infoheader->biHeight;
  line = height < 0 ? 0 : height - 1;
  dir = height < 0 ? 1 : -1;
  height = ABS(height);

  while (eopicflag == 0) {
    pos = 0;     /* x position in bitmap */
    eolflag = 0; /* end of line flag */

    while ((eolflag == 0) && (eopicflag == 0)) {
      count = fgetc(f);
      val = fgetc(f);

      if (count > 0) { /* repeat pixel count times */
        for (j = 0; j < count; j++) {
          put_pixel(image, pos, line, val);
          pos++;
        }
      }
      else {
        switch (val) {
          case 0: /* end of line flag */ eolflag = 1; break;

          case 1: /* end of picture flag */ eopicflag = 1; break;

          case 2: /* displace picture */
            count = fgetc(f);
            val = fgetc(f);
            pos += count;
            line += val * dir;
            break;

          default: /* read in absolute mode */
            for (j = 0; j < val; j++) {
              val0 = fgetc(f);
              put_pixel(image, pos, line, val0);
              pos++;
            }

            if (j % 2 == 1)
              val0 = fgetc(f); /* align on word boundary */
            break;
        }
      }

      if (pos - 1 > (int)infoheader->biWidth)
        eolflag = 1;
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
static void read_rle4_compressed_image(FILE* f, Image* image, const BITMAPINFOHEADER* infoheader)
{
  unsigned char b[8];
  unsigned char count;
  unsigned short val0, val;
  int j, k, pos, line, height, dir;
  int eolflag, eopicflag;

  eopicflag = 0; /* end of picture flag */

  height = (int)infoheader->biHeight;
  line = height < 0 ? 0 : height - 1;
  dir = height < 0 ? 1 : -1;
  height = ABS(height);

  while (eopicflag == 0) {
    pos = 0;
    eolflag = 0; /* end of line flag */

    while ((eolflag == 0) && (eopicflag == 0)) {
      count = fgetc(f);
      val = fgetc(f);

      if (count > 0) { /* repeat pixels count times */
        b[1] = val & 15;
        b[0] = (val >> 4) & 15;
        for (j = 0; j < count; j++) {
          put_pixel(image, pos, line, b[j % 2]);
          pos++;
        }
      }
      else {
        switch (val) {
          case 0: /* end of line */ eolflag = 1; break;

          case 1: /* end of picture */ eopicflag = 1; break;

          case 2: /* displace image */
            count = fgetc(f);
            val = fgetc(f);
            pos += count;
            line += val * dir;
            break;

          default: /* read in absolute mode */
            for (j = 0; j < val; j++) {
              if ((j % 4) == 0) {
                val0 = fgetw(f);
                for (k = 0; k < 2; k++) {
                  b[2 * k + 1] = val0 & 15;
                  val0 = val0 >> 4;
                  b[2 * k] = val0 & 15;
                  val0 = val0 >> 4;
                }
              }
              put_pixel(image, pos, line, b[j % 4]);
              pos++;
            }
            break;
        }
      }

      if (pos - 1 > (int)infoheader->biWidth)
        eolflag = 1;
    }

    line += dir;
    if (line < 0 || line >= height)
      eopicflag = 1;
  }
}

static uint32_t calc_shift(const uint32_t channelMask, int& channelBits)
{
  uint32_t channelShift = 0;
  uint32_t mask = 0;
  if (channelMask) {
    mask = ~channelMask;
    while (mask & 1) {
      ++channelShift;
      mask >>= 1;
    }
    if (mask) {
      mask = ~mask;
      while (mask & 1) {
        channelBits++;
        mask >>= 1;
      }
    }
    else
      channelBits = 32 - channelShift;
  }
  else
    channelBits = 8;
  return channelShift;
}

static int read_bitfields_image(FILE* f,
                                Image* image,
                                BITMAPINFOHEADER* infoheader,
                                uint32_t rmask,
                                uint32_t gmask,
                                uint32_t bmask,
                                uint32_t amask,
                                bool& withAlpha)
{
  uint32_t buffer, rshift, gshift, bshift, ashift;
  int rbits = 0, gbits = 0, bbits = 0, abits = 0;
  int i, j, k, line, height, dir, r, g, b, a;
  int bits_per_pixel;
  int bytes_per_pixel;

  height = (int)infoheader->biHeight;
  line = height < 0 ? 0 : height - 1;
  dir = height < 0 ? 1 : -1;
  height = ABS(height);

  /* calculate shifts */
  rshift = calc_shift(rmask, rbits);
  gshift = calc_shift(gmask, gbits);
  bshift = calc_shift(bmask, bbits);
  ashift = calc_shift(amask, abits);

  /* calculate bits-per-pixel and bytes-per-pixel */
  bits_per_pixel = infoheader->biBitCount;
  bytes_per_pixel = ((bits_per_pixel / 8) + ((bits_per_pixel % 8) > 0 ? 1 : 0));

  for (i = 0; i < height; i++, line += dir) {
    for (j = 0; j < (int)infoheader->biWidth; j++) {
      /* read the DWORD, WORD or BYTE in little-endian order */
      buffer = 0;
      for (k = 0; k < bytes_per_pixel; k++)
        buffer |= fgetc(f) << (k << 3);

      r = (buffer & rmask) >> rshift;
      g = (buffer & gmask) >> gshift;
      b = (buffer & bmask) >> bshift;
      a = (buffer & amask) >> ashift;

      r = (rbits == 8 ? r : scale_xbits_to_8bits(rbits, r));
      g = (gbits == 8 ? g : scale_xbits_to_8bits(gbits, g));
      b = (bbits == 8 ? b : scale_xbits_to_8bits(bbits, b));
      a = (abits == 8 ? a : scale_xbits_to_8bits(abits, a));

      if (a)
        withAlpha = true;
      put_pixel_fast<RgbTraits>(image, j, line, rgba(r, g, b, a));
    }

    j = (bytes_per_pixel * j) % 4;
    if (j > 0)
      while (j++ < 4)
        fgetc(f);
  }

  if (!withAlpha) {
    LockImageBits<RgbTraits> imageBits(image, image->bounds());
    auto imgIt = imageBits.begin(), imgEnd = imageBits.end();
    for (; imgIt != imgEnd; ++imgIt)
      *imgIt |= 0xff000000;
  }

  return 0;
}

bool BmpFormat::onLoad(FileOp* fop)
{
  uint32_t rmask, gmask, bmask, amask;
  BITMAPFILEHEADER fileheader;
  BITMAPINFOHEADER infoheader;
  PixelFormat pixelFormat;
  int format;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();

  if (read_bmfileheader(f, &fileheader) != 0)
    return false;

  infoheader.biSize = fgetl(f);

  if (infoheader.biSize >= 16 && infoheader.biSize <= 64) {
    format = BMP_OPTIONS_FORMAT_WINDOWS;

    if (read_win_bminfoheader(f, &infoheader) != 0) {
      return false;
    }
    if (infoheader.biCompression != BI_BITFIELDS && infoheader.biCompression != BI_ALPHABITFIELDS)
      read_bmicolors(fop, fileheader.bfOffBits - infoheader.biSize - OS2FILEHEADERSIZE, f, true);
    else if (infoheader.biBitCount <= 8)
      return false;
  }
  else if (infoheader.biSize == OS2INFOHEADERSIZE) {
    format = BMP_OPTIONS_FORMAT_OS2;

    if (read_os2_bminfoheader(f, &infoheader) != 0) {
      return false;
    }
    /* compute number of colors recorded */
    if (infoheader.biCompression != BI_BITFIELDS && infoheader.biCompression != BI_ALPHABITFIELDS)
      read_bmicolors(fop, fileheader.bfOffBits - infoheader.biSize - OS2FILEHEADERSIZE, f, false);
    else if (infoheader.biBitCount <= 8)
      return false;
  }
  else if (infoheader.biSize == BV4INFOHEADERSIZE) {
    format = BMP_OPTIONS_FORMAT_WINDOWS;

    if (read_v4_bminfoheader(f, &infoheader) != 0) {
      return false;
    }
    /* compute number of colors recorded */
    if (infoheader.biCompression != BI_BITFIELDS && infoheader.biCompression != BI_ALPHABITFIELDS)
      read_bmicolors(fop, fileheader.bfOffBits - infoheader.biSize - OS2FILEHEADERSIZE, f, true);
    else if (infoheader.biBitCount <= 8)
      return false;
  }
  else if (infoheader.biSize == BV5INFOHEADERSIZE) {
    format = BMP_OPTIONS_FORMAT_WINDOWS;

    if (read_v5_bminfoheader(f, &infoheader) != 0) {
      return false;
    }
    /* compute number of colors recorded */
    if (infoheader.biCompression != BI_BITFIELDS && infoheader.biCompression != BI_ALPHABITFIELDS)
      read_bmicolors(fop, fileheader.bfOffBits - infoheader.biSize - OS2FILEHEADERSIZE, f, true);
    else if (infoheader.biBitCount <= 8)
      return false;
  }
  else {
    return false;
  }

  // Check compatible Compression
  if (infoheader.biCompression == 4 || infoheader.biCompression == 5 ||
      infoheader.biCompression > 6) {
    fop->setError("Unsupported BMP compression.\n");
    return false;
  }

  // Check image size is valid
  {
    if (int(infoheader.biWidth) < 1 || ABS(int(infoheader.biHeight)) == 0) {
      fop->setError("Invalid BMP size.\n");
      return false;
    }

    uint32_t size = infoheader.biWidth * uint32_t(ABS(int(infoheader.biHeight)));
    if (infoheader.biBitCount >= 8)
      size *= (infoheader.biBitCount / 8);
    else if (8 / infoheader.biBitCount > 0)
      size /= (8 / infoheader.biBitCount);

    if (size > kMaxBmpSize) {
      fop->setError(fmt::format("BMP size unsupported ({:.2f} MB > {:.2f} MB).\n",
                                size / 1024.0 / 1024.0,
                                kMaxBmpSize / 1024.0 / 1024.0)
                      .c_str());
      return false;
    }
  }

  if ((infoheader.biBitCount == 32) || (infoheader.biBitCount == 24) ||
      (infoheader.biBitCount == 16))
    pixelFormat = IMAGE_RGB;
  else
    pixelFormat = IMAGE_INDEXED;

  /* bitfields have the 'mask' for each component */
  if (infoheader.isRGBMasks()) {
    rmask = infoheader.rMask;
    gmask = infoheader.gMask;
    bmask = infoheader.bMask;
    amask = (infoheader.isAlphaMask() ? infoheader.aMask : 0);
  }
  else
    rmask = gmask = bmask = amask = 0;

  ImageRef image =
    fop->sequenceImageToLoad(pixelFormat, infoheader.biWidth, ABS((int)infoheader.biHeight));
  if (!image) {
    return false;
  }

  if (pixelFormat == IMAGE_RGB)
    clear_image(image.get(), rgba(0, 0, 0, (infoheader.isAlphaMask() ? 0 : 255)));
  else
    clear_image(image.get(), 0);

  // We indirectly calculate 'on the fly' if the BMP file
  // has all its pixels with alpha value equal to 0 (i.e. BMP
  // without alpha channel) or if there is at least one pixel
  // with non 0 alpha (i.e. BMP works with alpha channel).
  // The result of this analysis will be stored in the boolean 'withAlpha'.
  bool withAlpha = false;
  switch (infoheader.biCompression) {
    case BI_RGB:  read_image(f, image.get(), &infoheader, fop, withAlpha); break;

    case BI_RLE8: read_rle8_compressed_image(f, image.get(), &infoheader); break;

    case BI_RLE4: read_rle4_compressed_image(f, image.get(), &infoheader); break;

    case BI_BITFIELDS:
    case BI_ALPHABITFIELDS:
      if (read_bitfields_image(f, image.get(), &infoheader, rmask, gmask, bmask, amask, withAlpha) <
          0) {
        fop->setError("Unsupported bitfields in the BMP file.\n");
        return false;
      }
      break;

    default: fop->setError("Unsupported BMP compression.\n"); return false;
  }

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }

  // Setup the file-data.
  if (!fop->formatOptions()) {
    auto bmp_options = std::make_shared<BmpOptions>();

    bmp_options->format = format;
    bmp_options->compression = infoheader.biCompression;
    bmp_options->bits_per_pixel = infoheader.biBitCount;
    bmp_options->red_mask = rmask;
    bmp_options->green_mask = gmask;
    bmp_options->blue_mask = bmask;
    if (withAlpha) {
      bmp_options->alpha_mask = amask;
      fop->sequenceSetHasAlpha(true);
    }
    else
      bmp_options->alpha_mask = 0;
    fop->setLoadedFormatOptions(bmp_options);
  }

  return true;
}

#ifdef ENABLE_SAVE
bool BmpFormat::onSave(FileOp* fop)
{
  const FileAbstractImage* img = fop->abstractImageToSave();
  const ImageSpec spec = img->spec();
  const int w = spec.width();
  const int h = spec.height();
  int bfSize;
  int biSizeImage;
  int ncolors = fop->sequenceGetNColors();
  int bpp = 0;
  bool withAlpha = img->needAlpha();

  switch (spec.colorMode()) {
    case ColorMode::RGB:
      if (withAlpha)
        bpp = 32;
      else
        bpp = 24;
      break;
    case ColorMode::GRAYSCALE: bpp = 8; break;
    case ColorMode::INDEXED:   {
      if (ncolors > 16)
        bpp = 8;
      else if (ncolors > 2)
        bpp = 4;
      else
        bpp = 1;
      ncolors = (1 << bpp);
      break;
    }
    default:
      // TODO save ColorMode::BITMAP as 1bpp bmp?
      // Invalid image format
      fop->setError("Unsupported color mode.\n");
      return false;
  }

  int filler = int((32 - ((w * bpp - 1) & 31) - 1) / 8);
  int c, i, j, r, g, b;

  if (bpp <= 8) {
    biSizeImage = (w + filler) * bpp / 8 * h;
    bfSize = (WININFOHEADERSIZE + OS2FILEHEADERSIZE // header
              + ncolors * 4                         // palette
              + biSizeImage);                       // image data
  }
  else {
    biSizeImage = (w * bpp / 8 + filler) * h;
    if (withAlpha)
      bfSize = BV3INFOHEADERSIZE + OS2FILEHEADERSIZE + biSizeImage; // header + image data
    else
      bfSize = WININFOHEADERSIZE + OS2FILEHEADERSIZE + biSizeImage; // header + image data
  }

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  /* file_header */
  fputw(0x4D42, f); /* bfType ("BM") */
  fputl(bfSize, f); /* bfSize */
  fputw(0, f);      /* bfReserved1 */
  fputw(0, f);      /* bfReserved2 */

  if (bpp <= 8) {
    fputl(WININFOHEADERSIZE + OS2FILEHEADERSIZE + ncolors * 4, f); /* bfOffBits */
    /* info_header */
    fputl(WININFOHEADERSIZE, f); /* biSize */
  }
  else if (withAlpha) {
    fputl(BV3INFOHEADERSIZE + OS2FILEHEADERSIZE, f); /* bfOffBits -taking account RBGA masks- */
    /* info_header */
    fputl(BV3INFOHEADERSIZE, f); /* biSize */
  }
  else {
    fputl(WININFOHEADERSIZE + OS2FILEHEADERSIZE, f); /* bfOffBits */
    /* info_header */
    fputl(WININFOHEADERSIZE, f); /* biSize */
  }

  fputl(w, f);   /* biWidth */
  fputl(h, f);   /* biHeight */
  fputw(1, f);   /* biPlanes */
  fputw(bpp, f); /* biBitCount */
  if (withAlpha) /* biCompression */
    fputl(BI_BITFIELDS, f);
  else
    fputl(BI_RGB, f);
  fputl(biSizeImage, f); /* biSizeImage */
  fputl(0xB12, f);       /* biXPelsPerMeter (0xB12 = 72 dpi) */
  fputl(0xB12, f);       /* biYPelsPerMeter */

  if (bpp <= 8) {
    fputl(ncolors, f); /* biClrUsed */
    fputl(ncolors, f); /* biClrImportant */

    // Save the palette
    for (i = 0; i < ncolors; i++) {
      fop->sequenceGetColor(i, &r, &g, &b);
      fputc(b, f);
      fputc(g, f);
      fputc(r, f);
      fputc(0, f);
    }
  }
  else {
    fputl(0, f); /* biClrUsed */
    fputl(0, f); /* biClrImportant */
    if (withAlpha) {
      fputl(0x00ff0000, f);
      fputl(0x0000ff00, f);
      fputl(0x000000ff, f);
      fputl(0xff000000, f);
    }
  }

  // Only used in indexed mode
  int colorsPerByte = std::max(1, 8 / bpp);
  int colorMask;
  switch (bpp) {
    case 8:  colorMask = 0xFF; break;
    case 4:  colorMask = 0x0F; break;
    case 1:  colorMask = 0x01; break;
    default: colorMask = 0; break;
  }

  // Save image pixels (from bottom to top)
  for (i = h - 1; i >= 0; i--) {
    switch (spec.colorMode()) {
      case ColorMode::RGB: {
        auto scanline = (const uint32_t*)img->getScanline(i);
        for (j = 0; j < w; ++j) {
          c = scanline[j];
          fputc(rgba_getb(c), f);
          fputc(rgba_getg(c), f);
          fputc(rgba_getr(c), f);
          if (withAlpha)
            fputc(rgba_geta(c), f);
        }
        break;
      }
      case ColorMode::GRAYSCALE: {
        auto scanline = (const uint16_t*)img->getScanline(i);
        for (j = 0; j < w; ++j) {
          c = scanline[j];
          fputc(graya_getv(c), f);
        }
        break;
      }
      case ColorMode::INDEXED: {
        auto scanline = (const uint8_t*)img->getScanline(i);
        for (j = 0; j < w;) {
          uint8_t value = 0;
          for (int k = colorsPerByte - 1; k >= 0 && j < w; --k, ++j) {
            c = scanline[j];
            value |= (c & colorMask) << (bpp * k);
          }
          fputc(value, f);
        }
        break;
      }
    }

    for (j = 0; j < filler; j++)
      fputc(0, f);

    fop->setProgress((float)(h - i) / (float)h);
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
