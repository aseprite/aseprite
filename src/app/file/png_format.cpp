// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/file/png_format.h"
#include "app/file/png_options.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "gfx/color_space.h"

#include <stdio.h>
#include <stdlib.h>

#include "png.h"

#define PNG_TRACE(...) // TRACE

namespace app {

using namespace base;

class PngFormat : public FileFormat {
  const char* onGetName() const override {
    return "png";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("png");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::PNG_IMAGE;
  }

  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_SEQUENCES |
      FILE_SUPPORT_PALETTE_WITH_ALPHA |
      FILE_ENCODE_ABSTRACT_IMAGE;
  }

  bool onLoad(FileOp* fop) override;
  gfx::ColorSpaceRef loadColorSpace(png_structp png, png_infop info);
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
  void saveColorSpace(png_structp png, png_infop info, const gfx::ColorSpace* colorSpace);
#endif
};

FileFormat* CreatePngFormat()
{
  return new PngFormat;
}

static void report_png_error(png_structp png, png_const_charp error)
{
  ((FileOp*)png_get_error_ptr(png))->setError("libpng: %s\n", error);
}

// TODO this should be information in FileOp parameter of onSave()
static bool fix_one_alpha_pixel = false;

PngEncoderOneAlphaPixel::PngEncoderOneAlphaPixel(bool state)
{
  fix_one_alpha_pixel = state;
}

PngEncoderOneAlphaPixel::~PngEncoderOneAlphaPixel()
{
  fix_one_alpha_pixel = false;
}

namespace {

class DestroyReadPng {
  png_structp png;
  png_infop info;
public:
  DestroyReadPng(png_structp png, png_infop info) : png(png), info(info) { }
  ~DestroyReadPng() {
    png_destroy_read_struct(&png, info ? &info: nullptr, nullptr);
  }
};

class DestroyWritePng {
  png_structp png;
  png_infop info;
public:
  DestroyWritePng(png_structp png, png_infop info) : png(png), info(info) { }
  ~DestroyWritePng() {
    png_destroy_write_struct(&png, info ? &info: nullptr);
  }
};

// As in png_fixed_point_to_float() in skia/src/codec/SkPngCodec.cpp
float png_fixtof(png_fixed_point x)
{
  // We multiply by the same factor that libpng used to convert
  // fixed point -> double.  Since we want floats, we choose to
  // do the conversion ourselves rather than convert
  // fixed point -> double -> float.
  return ((float)x) * 0.00001f;
}

png_fixed_point png_ftofix(float x)
{
  return x * 100000.0f;
}

int png_user_chunk(png_structp png, png_unknown_chunkp unknown)
{
  auto data = (std::shared_ptr<PngOptions>*)png_get_user_chunk_ptr(png);
  std::shared_ptr<PngOptions>& opts = *data;

  PNG_TRACE("PNG: Read unknown chunk '%c%c%c%c'\n",
            unknown->name[0],
            unknown->name[1],
            unknown->name[2],
            unknown->name[3]);

  PngOptions::Chunk chunk;
  chunk.location = unknown->location;
  for (int i=0; i<4; ++i)
    chunk.name.push_back(unknown->name[i]);
  if (unknown->size > 0) {
    chunk.data.resize(unknown->size);
    std::copy(unknown->data,
              unknown->data+unknown->size,
              chunk.data.begin());
  }
  opts->addChunk(std::move(chunk));

  return 1;
}

} // anonymous namespace

bool PngFormat::onLoad(FileOp* fop)
{
  png_uint_32 width, height, y;
  unsigned int sig_read = 0;
  int bit_depth, color_type, interlace_type;
  int num_palette;
  png_colorp palette;
  png_bytepp rows_pointer;
  PixelFormat pixelFormat;

  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* fp = handle.get();

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also supply the
   * the compiler header file version, so that we know if the application
   * was compiled with a compatible version of the library
   */
  png_structp png =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)fop,
                           report_png_error, report_png_error);
  if (png == nullptr) {
    fop->setError("png_create_read_struct\n");
    return false;
  }

  // Do don't check if the sRGB color profile is valid, it gives
  // problems with sRGB IEC61966-2.1 color profile from Photoshop.
  // See this thread: https://community.aseprite.org/t/2656
  png_set_option(png, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);

  // Set a function to read user data chunks
  auto opts = std::make_shared<PngOptions>();
  png_set_read_user_chunk_fn(png, &opts, png_user_chunk);

  /* Allocate/initialize the memory for image information. */
  png_infop info = png_create_info_struct(png);
  DestroyReadPng destroyer(png, info);
  if (info == nullptr) {
    fop->setError("png_create_info_struct\n");
    return false;
  }

  /* Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).
   */
  if (setjmp(png_jmpbuf(png))) {
    fop->setError("Error reading PNG file\n");
    return false;
  }

  /* Set up the input control if you are using standard C streams */
  png_init_io(png, fp);

  /* If we have already read some of the signature */
  png_set_sig_bytes(png, sig_read);

  /* The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).
   */
  png_read_info(png, info);

  png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type,
               &interlace_type, NULL, NULL);


  /* Set up the data transformations you want.  Note that these are all
   * optional.  Only call them if you want/need them.  Many of the
   * transformations only work on specific types of images, and many
   * are mutually exclusive.
   */

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  png_set_strip_16(png);

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images).
   */
  png_set_packing(png);

  /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  /* Turn on interlace handling.  REQUIRED if you are not using
   * png_read_image().  To see how to handle interlacing passes,
   * see the png_read_row() method below:
   */
  int number_passes = png_set_interlace_handling(png);

  /* Optional call to gamma correct and add the background to the palette
   * and update info structure.
   */
  png_read_update_info(png, info);

  /* create the output image */
  switch (png_get_color_type(png, info)) {

    case PNG_COLOR_TYPE_RGB_ALPHA:
      fop->sequenceSetHasAlpha(true);
      [[fallthrough]];
    case PNG_COLOR_TYPE_RGB:
      pixelFormat = IMAGE_RGB;
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
      fop->sequenceSetHasAlpha(true);
      [[fallthrough]];
    case PNG_COLOR_TYPE_GRAY:
      pixelFormat = IMAGE_GRAYSCALE;
      break;

    case PNG_COLOR_TYPE_PALETTE:
      pixelFormat = IMAGE_INDEXED;
      break;

    default:
      fop->setError("Color type not supported\n)");
      return false;
  }

  int imageWidth = png_get_image_width(png, info);
  int imageHeight = png_get_image_height(png, info);
  ImageRef image = fop->sequenceImage(pixelFormat, imageWidth, imageHeight);
  if (!image)
    return false;

  // Transparent color
  png_color_16p png_trans_color = NULL;

  // Read the palette
  if (png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE &&
      png_get_PLTE(png, info, &palette, &num_palette)) {
    fop->sequenceSetNColors(num_palette);

    for (int c=0; c<num_palette; ++c) {
      fop->sequenceSetColor(c,
                            palette[c].red,
                            palette[c].green,
                            palette[c].blue);
    }

    // Read alpha values for palette entries
    png_bytep trans = NULL;     // Transparent palette entries
    int num_trans = 0;
    int mask_entry = -1;

    png_get_tRNS(png, info, &trans, &num_trans, nullptr);

    for (int i = 0; i < num_trans; ++i) {
      fop->sequenceSetAlpha(i, trans[i]);

      if (trans[i] < 255) {
        fop->sequenceSetHasAlpha(true); // Is a transparent sprite
        if (trans[i] == 0) {
          if (mask_entry < 0)
            mask_entry = i;
        }
      }
    }

    if (mask_entry >= 0)
      fop->document()->sprite()->setTransparentColor(mask_entry);
  }
  else {
    png_get_tRNS(png, info, nullptr, nullptr, &png_trans_color);
  }

  // Allocate the memory to hold the image using the fields of info.
  rows_pointer = (png_bytepp)png_malloc(png, sizeof(png_bytep) * height);
  for (y = 0; y < height; y++)
    rows_pointer[y] = (png_bytep)png_malloc(png, png_get_rowbytes(png, info));

  for (int pass=0; pass<number_passes; ++pass) {
    for (y = 0; y < height; y++) {
      png_read_rows(png, rows_pointer+y, nullptr, 1);

      fop->setProgress(
        (double)((double)pass + (double)(y+1) / (double)(height))
        / (double)number_passes);

      if (fop->isStop())
        break;
    }
  }

  // Convert rows_pointer into the doc::Image
  for (y = 0; y < height; y++) {
    // RGB_ALPHA
    if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB_ALPHA) {
      uint8_t* src_address = rows_pointer[y];
      uint32_t* dst_address = (uint32_t*)image->getPixelAddress(0, y);
      unsigned int x, r, g, b, a;

      for (x=0; x<width; x++) {
        r = *(src_address++);
        g = *(src_address++);
        b = *(src_address++);
        a = *(src_address++);
        *(dst_address++) = rgba(r, g, b, a);
      }
    }
    // RGB
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB) {
      uint8_t* src_address = rows_pointer[y];
      uint32_t* dst_address = (uint32_t*)image->getPixelAddress(0, y);
      unsigned int x, r, g, b, a;

      for (x=0; x<width; x++) {
        r = *(src_address++);
        g = *(src_address++);
        b = *(src_address++);

        // Transparent color
        if (png_trans_color &&
            r == png_trans_color->red &&
            g == png_trans_color->green &&
            b == png_trans_color->blue) {
          a = 0;
          if (!fop->sequenceGetHasAlpha())
            fop->sequenceSetHasAlpha(true);
        }
        else
          a = 255;

        *(dst_address++) = rgba(r, g, b, a);
      }
    }
    // GRAY_ALPHA
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY_ALPHA) {
      uint8_t* src_address = rows_pointer[y];
      uint16_t* dst_address = (uint16_t*)image->getPixelAddress(0, y);
      unsigned int x, k, a;

      for (x=0; x<width; x++) {
        k = *(src_address++);
        a = *(src_address++);
        *(dst_address++) = graya(k, a);
      }
    }
    // GRAY
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY) {
      uint8_t* src_address = rows_pointer[y];
      uint16_t* dst_address = (uint16_t*)image->getPixelAddress(0, y);
      unsigned int x, k, a;

      for (x=0; x<width; x++) {
        k = *(src_address++);

        // Transparent color
        if (png_trans_color &&
            k == png_trans_color->gray) {
          a = 0;
          if (!fop->sequenceGetHasAlpha())
            fop->sequenceSetHasAlpha(true);
        }
        else
          a = 255;

        *(dst_address++) = graya(k, a);
      }
    }
    // PALETTE
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE) {
      uint8_t* src_address = rows_pointer[y];
      uint8_t* dst_address = (uint8_t*)image->getPixelAddress(0, y);
      unsigned int x;

      for (x=0; x<width; x++)
        *(dst_address++) = *(src_address++);
    }
    png_free(png, rows_pointer[y]);
  }
  png_free(png, rows_pointer);

  // Setup the color space.
  auto colorSpace = PngFormat::loadColorSpace(png, info);
  if (colorSpace)
    fop->setEmbeddedColorProfile();
  else { // sRGB is the default PNG color space.
    colorSpace = gfx::ColorSpace::MakeSRGB();
  }
  if (colorSpace &&
      fop->document()->sprite()->colorSpace()->type() == gfx::ColorSpace::None) {
    fop->document()->sprite()->setColorSpace(colorSpace);
    fop->document()->notifyColorSpaceChanged();
  }

  ASSERT(opts != nullptr);
  if (!opts->isEmpty())
    fop->setLoadedFormatOptions(opts);

  return true;
}

// Returns a colorSpace object that represents any
// color space information in the encoded data.  If the encoded data
// contains an invalid/unsupported color space, this will return
// NULL. If there is no color space information, it will guess sRGB
//
// Code to read color spaces from png files from Skia (SkPngCodec.cpp)
// by Google Inc.
gfx::ColorSpaceRef PngFormat::loadColorSpace(png_structp png_ptr, png_infop info_ptr)
{
  // First check for an ICC profile
  png_bytep profile;
  png_uint_32 length;
  // The below variables are unused, however, we need to pass them in anyway or
  // png_get_iCCP() will return nothing.
  // Could knowing the |name| of the profile ever be interesting?  Maybe for debugging?
  png_charp name;
  // The |compression| is uninteresting since:
  //   (1) libpng has already decompressed the profile for us.
  //   (2) "deflate" is the only mode of decompression that libpng supports.
  int compression;
  if (PNG_INFO_iCCP == png_get_iCCP(png_ptr, info_ptr,
                                    &name, &compression,
                                    &profile, &length)) {
    auto colorSpace = gfx::ColorSpace::MakeICC(profile, length);
    if (name)
      colorSpace->setName(name);
    return colorSpace;
  }

  // Second, check for sRGB.
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB)) {
    // sRGB chunks also store a rendering intent: Absolute, Relative,
    // Perceptual, and Saturation.
    return gfx::ColorSpace::MakeSRGB();
  }

  // Next, check for chromaticities.
  png_fixed_point wx, wy, rx, ry, gx, gy, bx, by, invGamma;
  if (png_get_cHRM_fixed(png_ptr, info_ptr,
                         &wx, &wy, &rx, &ry, &gx, &gy, &bx, &by)) {
    gfx::ColorSpacePrimaries primaries;
    primaries.wx = png_fixtof(wx); primaries.wy = png_fixtof(wy);
    primaries.rx = png_fixtof(rx); primaries.ry = png_fixtof(ry);
    primaries.gx = png_fixtof(gx); primaries.gy = png_fixtof(gy);
    primaries.bx = png_fixtof(bx); primaries.by = png_fixtof(by);

    if (PNG_INFO_gAMA == png_get_gAMA_fixed(png_ptr, info_ptr, &invGamma)) {
      gfx::ColorSpaceTransferFn fn;
      fn.a = 1.0f;
      fn.b = fn.c = fn.d = fn.e = fn.f = 0.0f;
      fn.g = 1.0f / png_fixtof(invGamma);

      return gfx::ColorSpace::MakeRGB(fn, primaries);
    }

    // Default to sRGB gamma if the image has color space information,
    // but does not specify gamma.
    return gfx::ColorSpace::MakeRGBWithSRGBGamma(primaries);
  }

  // Last, check for gamma.
  if (PNG_INFO_gAMA == png_get_gAMA_fixed(png_ptr, info_ptr, &invGamma)) {
    // Since there is no cHRM, we will guess sRGB gamut.
    return gfx::ColorSpace::MakeSRGBWithGamma(1.0f / png_fixtof(invGamma));
  }

  // No color space.
  return nullptr;
}

#ifdef ENABLE_SAVE

bool PngFormat::onSave(FileOp* fop)
{
  png_infop info;
  png_colorp palette = nullptr;
  png_bytep row_pointer;
  int color_type = 0;

  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* fp = handle.get();

  png_structp png =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)fop,
                            report_png_error, report_png_error);
  if (png == nullptr)
    return false;

  // Remove sRGB profile checks
  png_set_option(png, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON);

  info = png_create_info_struct(png);
  DestroyWritePng destroyer(png, info);
  if (info == nullptr)
    return false;

  if (setjmp(png_jmpbuf(png)))
    return false;

  png_init_io(png, fp);

  const FileAbstractImage* img = fop->abstractImage();
  const ImageSpec spec = img->spec();

  switch (spec.colorMode()) {
    case ColorMode::RGB:
      color_type =
        (img->needAlpha() ||
         fix_one_alpha_pixel ?
         PNG_COLOR_TYPE_RGB_ALPHA:
         PNG_COLOR_TYPE_RGB);
      break;
    case ColorMode::GRAYSCALE:
      color_type =
        (img->needAlpha() ||
         fix_one_alpha_pixel ?
         PNG_COLOR_TYPE_GRAY_ALPHA:
         PNG_COLOR_TYPE_GRAY);
      break;
    case ColorMode::INDEXED:
      if (fix_one_alpha_pixel)
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      else
        color_type = PNG_COLOR_TYPE_PALETTE;
      break;
  }

  const png_uint_32 width = spec.width();
  const png_uint_32 height = spec.height();

  png_set_IHDR(png, info, width, height, 8, color_type,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  // User chunks
  auto opts = fop->formatOptionsOfDocument<PngOptions>();
  if (opts && !opts->isEmpty()) {
    int num_unknowns = opts->size();
    ASSERT(num_unknowns > 0);
    std::vector<png_unknown_chunk> unknowns(num_unknowns);
    int i = 0;
    for (const auto& chunk : opts->chunks()) {
      png_unknown_chunk& unknown = unknowns[i];
      for (int i=0; i<5; ++i) {
        unknown.name[i] =
          (i < int(chunk.name.size()) ? chunk.name[i]: 0);
      }
      PNG_TRACE("PNG: Write unknown chunk '%c%c%c%c'\n",
                unknown.name[0],
                unknown.name[1],
                unknown.name[2],
                unknown.name[3]);
      unknown.data = (png_byte*)&chunk.data[0];
      unknown.size = chunk.data.size();
      unknown.location = chunk.location;
      ++i;
    }
    png_set_unknown_chunks(png, info, &unknowns[0], num_unknowns);
  }

  if (fop->preserveColorProfile() && spec.colorSpace()) {
    saveColorSpace(png, info, spec.colorSpace().get());
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    int c, r, g, b;
    int pal_size = fop->sequenceGetNColors();
    pal_size = std::clamp(pal_size, 1, PNG_MAX_PALETTE_LENGTH);

#if PNG_MAX_PALETTE_LENGTH != 256
#error PNG_MAX_PALETTE_LENGTH should be 256
#endif

    // Save the color palette.
    palette = (png_colorp)png_malloc(png, pal_size * sizeof(png_color));
    for (c = 0; c < pal_size; c++) {
      fop->sequenceGetColor(c, &r, &g, &b);
      palette[c].red   = r;
      palette[c].green = g;
      palette[c].blue  = b;
    }

    png_set_PLTE(png, info, palette, pal_size);

    // If the sprite does not have a (visible) background layer, we
    // put alpha=0 to the transparent color.
    int mask_entry = -1;
    if (fop->document()->sprite()->backgroundLayer() == nullptr ||
        !fop->document()->sprite()->backgroundLayer()->isVisible()) {
      mask_entry = fop->document()->sprite()->transparentColor();
    }

    bool all_opaque = true;
    int num_trans = pal_size;
    png_bytep trans = (png_bytep)png_malloc(png, num_trans);

    for (c=0; c<num_trans; ++c) {
      int alpha = 255;
      fop->sequenceGetAlpha(c, &alpha);
      trans[c] = (c == mask_entry ? 0: alpha);
      if (alpha < 255)
        all_opaque = false;
    }

    if (!all_opaque || mask_entry >= 0)
      png_set_tRNS(png, info, trans, num_trans, nullptr);

    png_free(png, trans);
  }

  png_write_info(png, info);
  png_set_packing(png);

  row_pointer = (png_bytep)png_malloc(png, png_get_rowbytes(png, info));

  for (png_uint_32 y=0; y<height; ++y) {
    uint8_t* dst_address = row_pointer;

    if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB_ALPHA) {
      unsigned int x, c, a;
      bool opaque = true;

      if (spec.colorMode() == ColorMode::RGB) {
        auto src_address = (const uint32_t*)img->getScanline(y);

        for (x=0; x<width; ++x) {
          c = *(src_address++);
          a = rgba_geta(c);

          if (opaque) {
            if (a < 255)
              opaque = false;
            else if (fix_one_alpha_pixel && x == width-1 && y == height-1)
              a = 254;
          }

          *(dst_address++) = rgba_getr(c);
          *(dst_address++) = rgba_getg(c);
          *(dst_address++) = rgba_getb(c);
          *(dst_address++) = a;
        }
      }
      // In case that we are converting an indexed image to RGB just
      // to convert one pixel with alpha=254.
      else if (spec.colorMode() == ColorMode::INDEXED) {
        auto src_address = (const uint8_t*)img->getScanline(y);
        unsigned int x, c;
        int r, g, b, a;
        bool opaque = true;

        for (x=0; x<width; ++x) {
          c = *(src_address++);
          fop->sequenceGetColor(c, &r, &g, &b);
          fop->sequenceGetAlpha(c, &a);

          if (opaque) {
            if (a < 255)
              opaque = false;
            else if (fix_one_alpha_pixel && x == width-1 && y == height-1)
              a = 254;
          }

          *(dst_address++) = r;
          *(dst_address++) = g;
          *(dst_address++) = b;
          *(dst_address++) = a;
        }
      }
    }
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB) {
      auto src_address = (const uint32_t*)img->getScanline(y);
      unsigned int x, c;

      for (x=0; x<width; ++x) {
        c = *(src_address++);
        *(dst_address++) = rgba_getr(c);
        *(dst_address++) = rgba_getg(c);
        *(dst_address++) = rgba_getb(c);
      }
    }
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY_ALPHA) {
      auto src_address = (const uint16_t*)img->getScanline(y);
      unsigned int x, c, a;
      bool opaque = true;

      for (x=0; x<width; x++) {
        c = *(src_address++);
        a = graya_geta(c);

        if (opaque) {
          if (a < 255)
            opaque = false;
          else if (fix_one_alpha_pixel && x == width-1 && y == height-1)
            a = 254;
        }

        *(dst_address++) = graya_getv(c);
        *(dst_address++) = a;
      }
    }
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY) {
      auto src_address = (const uint16_t*)img->getScanline(y);
      unsigned int x, c;

      for (x=0; x<width; ++x) {
        c = *(src_address++);
        *(dst_address++) = graya_getv(c);
      }
    }
    else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE) {
      auto src_address = (const uint8_t*)img->getScanline(y);
      unsigned int x;

      for (x=0; x<width; ++x)
        *(dst_address++) = *(src_address++);
    }

    png_write_rows(png, &row_pointer, 1);

    fop->setProgress((double)(y+1) / (double)(height));
  }

  png_free(png, row_pointer);
  png_write_end(png, info);

  if (spec.colorMode() == ColorMode::INDEXED) {
    png_free(png, palette);
    palette = nullptr;
  }

  return true;
}

void PngFormat::saveColorSpace(png_structp png_ptr, png_infop info_ptr,
                               const gfx::ColorSpace* colorSpace)
{
  switch (colorSpace->type()) {

    case gfx::ColorSpace::None:
      // Do just nothing (png file without profile, like old Aseprite versions)
      break;

    case gfx::ColorSpace::sRGB:
      // TODO save the original intent
      if (!colorSpace->hasGamma()) {
        png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_PERCEPTUAL);
        return;
      }

      // Continue to RGB case...
      [[fallthrough]];

    case gfx::ColorSpace::RGB: {
      if (colorSpace->hasPrimaries()) {
        const gfx::ColorSpacePrimaries* p = colorSpace->primaries();
        png_set_cHRM_fixed(png_ptr, info_ptr,
                           png_ftofix(p->wx), png_ftofix(p->wy),
                           png_ftofix(p->rx), png_ftofix(p->ry),
                           png_ftofix(p->gx), png_ftofix(p->gy),
                           png_ftofix(p->bx), png_ftofix(p->by));
      }
      if (colorSpace->hasGamma()) {
        png_set_gAMA_fixed(png_ptr, info_ptr,
                           png_ftofix(1.0f / colorSpace->gamma()));
      }
      break;
    }

    case gfx::ColorSpace::ICC: {
      png_set_iCCP(png_ptr, info_ptr,
                   (png_const_charp)colorSpace->name().c_str(),
                   PNG_COMPRESSION_TYPE_DEFAULT,
                   (png_const_bytep)colorSpace->iccData(),
                   (png_uint_32)colorSpace->iccSize());
      break;
    }

  }
}

#endif  // ENABLE_SAVE

} // namespace app
