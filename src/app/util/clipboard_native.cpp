// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/clipboard_native.h"

#include "app/i18n/strings.h"
#include "base/serialization.h"
#include "clip/clip.h"
#include "doc/color_scales.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/image_io.h"
#include "doc/mask_io.h"
#include "doc/palette_io.h"
#include "gfx/size.h"
#include "os/display.h"
#include "os/system.h"
#include "ui/alert.h"

#include <sstream>
#include <vector>

namespace app {
namespace clipboard {

using namespace base::serialization;
using namespace base::serialization::little_endian;

namespace {
  clip::format custom_image_format = 0;

  void* native_display_handle() {
    return os::instance()->defaultDisplay()->nativeHandle();
  }

  void custom_error_handler(clip::ErrorCode code) {
    switch (code) {
      case clip::ErrorCode::CannotLock:
        ui::Alert::show(Strings::alerts_clipboard_access_locked());
        break;
      case clip::ErrorCode::ImageNotSupported:
        ui::Alert::show(Strings::alerts_clipboard_image_format_not_supported());
        break;
    }
  }

}

void register_native_clipboard_formats()
{
  clip::set_error_handler(custom_error_handler);
  custom_image_format = clip::register_format("org.aseprite.Image");
}

bool has_native_clipboard_bitmap()
{
  return clip::has(clip::image_format());
}

bool set_native_clipboard_bitmap(const doc::Image* image,
                                 const doc::Mask* mask,
                                 const doc::Palette* palette)
{
  clip::lock l(native_display_handle());
  if (!l.locked())
    return false;

  l.clear();

  if (!image)
    return false;

  // Set custom clipboard formats
  if (custom_image_format) {
    std::stringstream os;
    write32(os,
            (image   ? 1: 0) |
            (mask    ? 2: 0) |
            (palette ? 4: 0));
    if (image) doc::write_image(os, image);
    if (mask) doc::write_mask(os, mask);
    if (palette) doc::write_palette(os, palette);

    if (os.good()) {
      size_t size = (size_t)os.tellp();
      if (size > 0) {
        std::vector<char> data(size);
        os.seekp(0);
        os.read(&data[0], size);

        l.set_data(custom_image_format, &data[0], size);
      }
    }
  }

  clip::image_spec spec;
  spec.width = image->width();
  spec.height = image->height();
  spec.bits_per_pixel = 32;
  spec.bytes_per_row = (image->pixelFormat() == doc::IMAGE_RGB ?
                        image->getRowStrideSize(): 4*spec.width);
  spec.red_mask    = doc::rgba_r_mask;
  spec.green_mask  = doc::rgba_g_mask;
  spec.blue_mask   = doc::rgba_b_mask;
  spec.alpha_mask  = doc::rgba_a_mask;
  spec.red_shift   = doc::rgba_r_shift;
  spec.green_shift = doc::rgba_g_shift;
  spec.blue_shift  = doc::rgba_b_shift;
  spec.alpha_shift = doc::rgba_a_shift;

  switch (image->pixelFormat()) {
    case doc::IMAGE_RGB: {
      // We use the RGB image data directly
      clip::image img(image->getPixelAddress(0, 0), spec);
      l.set_image(img);
      break;
    }
    case doc::IMAGE_GRAYSCALE: {
      clip::image img(spec);
      const doc::LockImageBits<doc::GrayscaleTraits> bits(image);
      auto it = bits.begin();
      uint32_t* dst = (uint32_t*)img.data();
      for (int y=0; y<image->height(); ++y) {
        for (int x=0; x<image->width(); ++x, ++it) {
          doc::color_t c = *it;
          *(dst++) = doc::rgba(doc::graya_getv(c),
                               doc::graya_getv(c),
                               doc::graya_getv(c),
                               doc::graya_geta(c));
        }
      }
      l.set_image(img);
      break;
    }
    case doc::IMAGE_INDEXED: {
      clip::image img(spec);
      const doc::LockImageBits<doc::IndexedTraits> bits(image);
      auto it = bits.begin();
      uint32_t* dst = (uint32_t*)img.data();
      for (int y=0; y<image->height(); ++y) {
        for (int x=0; x<image->width(); ++x, ++it) {
          doc::color_t c = palette->getEntry(*it);

          // Use alpha=0 for mask color
          if (*it == image->maskColor())
            c &= doc::rgba_rgb_mask;

          *(dst++) = c;
        }
      }
      l.set_image(img);
      break;
    }
  }

  return true;
}

bool get_native_clipboard_bitmap(doc::Image** image,
                                 doc::Mask** mask,
                                 doc::Palette** palette)
{
  *image = nullptr;
  *mask = nullptr;
  *palette = nullptr;

  clip::lock l(native_display_handle());
  if (!l.locked())
    return false;

  // Prefer the custom format (to avoid losing mask and palette)
  if (l.is_convertible(custom_image_format)) {
    size_t size = l.get_data_length(custom_image_format);
    if (size > 0) {
      std::vector<char> buf(size);
      if (l.get_data(custom_image_format, &buf[0], size)) {
        std::stringstream is;
        is.write(&buf[0], size);
        is.seekp(0);

        int bits = read32(is);
        if (bits & 1) *image   = doc::read_image(is, false);
        if (bits & 2) *mask    = doc::read_mask(is);
        if (bits & 4) *palette = doc::read_palette(is);
        if (image)
          return true;
      }
    }
  }

  if (!l.is_convertible(clip::image_format()))
    return false;

  clip::image img;
  if (!l.get_image(img))
    return false;

  const clip::image_spec& spec = img.spec();

  std::unique_ptr<doc::Image> dst(
    doc::Image::create(doc::IMAGE_RGB,
                       spec.width, spec.height));

  switch (spec.bits_per_pixel) {
    case 64: {
      doc::LockImageBits<doc::RgbTraits> bits(dst.get(), doc::Image::WriteLock);
      auto it = bits.begin();
      for (unsigned long y=0; y<spec.height; ++y) {
        const uint64_t* src = (const uint64_t*)(img.data()+spec.bytes_per_row*y);
        for (unsigned long x=0; x<spec.width; ++x, ++it, ++src) {
          uint64_t c = *((const uint64_t*)src);
          *it = doc::rgba(
            uint8_t((c & spec.red_mask) >> spec.red_shift >> 8),
            uint8_t((c & spec.green_mask) >> spec.green_shift >> 8),
            uint8_t((c & spec.blue_mask) >> spec.blue_shift >> 8),
            uint8_t((c & spec.alpha_mask) >> spec.alpha_shift >> 8));
        }
      }
      break;
    }
    case 32: {
      doc::LockImageBits<doc::RgbTraits> bits(dst.get(), doc::Image::WriteLock);
      auto it = bits.begin();
      for (unsigned long y=0; y<spec.height; ++y) {
        const uint32_t* src = (const uint32_t*)(img.data()+spec.bytes_per_row*y);
        for (unsigned long x=0; x<spec.width; ++x, ++it, ++src) {
          const uint32_t c = *((const uint32_t*)src);

          // The alpha mask can be zero (which means that the image is
          // just RGB).
          int alpha =
            (spec.alpha_mask ?
             uint8_t((c & spec.alpha_mask) >> spec.alpha_shift): 255);

          *it = doc::rgba(
            uint8_t((c & spec.red_mask  ) >> spec.red_shift  ),
            uint8_t((c & spec.green_mask) >> spec.green_shift),
            uint8_t((c & spec.blue_mask ) >> spec.blue_shift ),
            alpha);
        }
      }
      break;
    }
    case 24: {
      doc::LockImageBits<doc::RgbTraits> bits(dst.get(), doc::Image::WriteLock);
      auto it = bits.begin();
      for (unsigned long y=0; y<spec.height; ++y) {
        const char* src = (const char*)(img.data()+spec.bytes_per_row*y);
        for (unsigned long x=0; x<spec.width; ++x, ++it, src+=3) {
          unsigned long c = *((const unsigned long*)src);
          *it = doc::rgba(
            uint8_t((c & spec.red_mask) >> spec.red_shift),
            uint8_t((c & spec.green_mask) >> spec.green_shift),
            uint8_t((c & spec.blue_mask) >> spec.blue_shift),
            255);
        }
      }
      break;
    }
    case 16: {
      doc::LockImageBits<doc::RgbTraits> bits(dst.get(), doc::Image::WriteLock);
      auto it = bits.begin();
      for (unsigned long y=0; y<spec.height; ++y) {
        const uint16_t* src = (const uint16_t*)(img.data()+spec.bytes_per_row*y);
        for (unsigned long x=0; x<spec.width; ++x, ++it, ++src) {
          const uint16_t c = *((const uint16_t*)src);
          *it = doc::rgba(
            doc::scale_5bits_to_8bits((c & spec.red_mask  ) >> spec.red_shift  ),
            doc::scale_6bits_to_8bits((c & spec.green_mask) >> spec.green_shift),
            doc::scale_5bits_to_8bits((c & spec.blue_mask ) >> spec.blue_shift ),
            255);
        }
      }
      break;
    }
  }

  *image = dst.release();
  return true;
}

bool get_native_clipboard_bitmap_size(gfx::Size* size)
{
  clip::image_spec spec;
  if (clip::get_image_spec(spec)) {
    size->w = spec.width;
    size->h = spec.height;
    return true;
  }
  else
    return false;
}

} // namespace clipboard
} // namespace app
