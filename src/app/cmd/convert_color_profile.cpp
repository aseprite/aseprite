// Aseprite
// Copyright (C) 2018-2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/convert_color_profile.h"

#include "app/cmd/assign_color_profile.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_palette.h"
#include "app/doc.h"
#include "doc/cels_range.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "os/color_space.h"
#include "os/system.h"

namespace app { namespace cmd {

static doc::ImageRef convert_image_color_space(const doc::Image* srcImage,
                                               const gfx::ColorSpaceRef& newCS,
                                               os::ColorSpaceConversion* conversion)
{
  ImageSpec spec = srcImage->spec();
  spec.setColorSpace(newCS);
  ImageRef dstImage(Image::create(spec));

  if (!conversion) {
    dstImage->copy(srcImage, gfx::Clip(0, 0, srcImage->bounds()));
    return dstImage;
  }

  if (spec.colorMode() == doc::ColorMode::RGB) {
    for (int y = 0; y < spec.height(); ++y) {
      conversion->convertRgba((uint32_t*)dstImage->getPixelAddress(0, y),
                              (const uint32_t*)srcImage->getPixelAddress(0, y),
                              spec.width());
    }
  }
  else if (spec.colorMode() == doc::ColorMode::GRAYSCALE) {
    // TODO create a set of functions to create pixel format
    // conversions (this should be available when we add new kind of
    // pixel formats).
    std::vector<uint8_t> buf(spec.width() * spec.height());

    auto it = buf.begin();
    for (int y = 0; y < spec.height(); ++y) {
      auto srcPtr = (const uint16_t*)srcImage->getPixelAddress(0, y);
      for (int x = 0; x < spec.width(); ++x, ++srcPtr, ++it)
        *it = doc::graya_getv(*srcPtr);
    }

    conversion->convertGray(&buf[0], &buf[0], spec.width() * spec.height());

    it = buf.begin();
    for (int y = 0; y < spec.height(); ++y) {
      auto srcPtr = (const uint16_t*)srcImage->getPixelAddress(0, y);
      auto dstPtr = (uint16_t*)dstImage->getPixelAddress(0, y);
      for (int x = 0; x < spec.width(); ++x, ++dstPtr, ++srcPtr, ++it)
        *dstPtr = doc::graya(*it, doc::graya_geta(*srcPtr));
    }
  }

  return dstImage;
}

void convert_color_profile(doc::Sprite* sprite, const gfx::ColorSpaceRef& newCS)
{
  ASSERT(sprite->colorSpace());
  ASSERT(newCS);

  os::System* system = os::instance();
  auto srcOCS = system->makeColorSpace(sprite->colorSpace());
  auto dstOCS = system->makeColorSpace(newCS);
  ASSERT(srcOCS);
  ASSERT(dstOCS);

  auto conversion = system->convertBetweenColorSpace(srcOCS, dstOCS);

  // Convert images
  if (sprite->pixelFormat() != doc::IMAGE_INDEXED) {
    for (Cel* cel : sprite->uniqueCels()) {
      ImageRef old_image = cel->imageRef();
      if (old_image.get()->pixelFormat() != IMAGE_TILEMAP) {
        ImageRef new_image = convert_image_color_space(old_image.get(), newCS, conversion.get());

        sprite->replaceImage(old_image->id(), new_image);
      }
    }
  }

  if (conversion) {
    // Convert palette
    if (sprite->pixelFormat() != doc::IMAGE_GRAYSCALE) {
      for (auto& pal : sprite->getPalettes()) {
        Palette newPal(pal->frame(), pal->size());

        for (int i = 0; i < pal->size(); ++i) {
          color_t oldCol = pal->entry(i);
          color_t newCol = pal->entry(i);
          conversion->convertRgba((uint32_t*)&newCol, (const uint32_t*)&oldCol, 1);
          newPal.setEntry(i, newCol);
        }

        if (*pal != newPal)
          sprite->setPalette(&newPal, false);
      }
    }
  }

  sprite->setColorSpace(newCS);

  // Generate notification so the Doc::osColorSpace() is regenerated
  static_cast<Doc*>(sprite->document())->notifyColorSpaceChanged();
}

void convert_color_profile(doc::Image* image,
                           doc::Palette* palette,
                           const gfx::ColorSpaceRef& oldCS,
                           const gfx::ColorSpaceRef& newCS)
{
  ASSERT(oldCS);
  ASSERT(newCS);

  os::System* system = os::instance();
  auto srcOCS = system->makeColorSpace(oldCS);
  auto dstOCS = system->makeColorSpace(newCS);
  ASSERT(srcOCS);
  ASSERT(dstOCS);

  auto conversion = system->convertBetweenColorSpace(srcOCS, dstOCS);
  if (conversion) {
    switch (image->pixelFormat()) {
      case doc::IMAGE_RGB:
      case doc::IMAGE_GRAYSCALE: {
        ImageRef newImage = convert_image_color_space(image, newCS, conversion.get());

        image->copy(newImage.get(), gfx::Clip(image->bounds()));
        break;
      }

      case doc::IMAGE_INDEXED: {
        for (int i = 0; i < palette->size(); ++i) {
          color_t oldCol, newCol;
          oldCol = newCol = palette->entry(i);
          conversion->convertRgba((uint32_t*)&newCol, (const uint32_t*)&oldCol, 1);
          palette->setEntry(i, newCol);
        }
        break;
      }
    }
  }
}

ConvertColorProfile::ConvertColorProfile(doc::Sprite* sprite, const gfx::ColorSpaceRef& newCS)
  : WithSprite(sprite)
{
  os::System* system = os::instance();

  ASSERT(sprite->colorSpace());
  ASSERT(newCS);

  auto srcOCS = system->makeColorSpace(sprite->colorSpace());
  auto dstOCS = system->makeColorSpace(newCS);

  ASSERT(srcOCS);
  ASSERT(dstOCS);

  auto conversion = system->convertBetweenColorSpace(srcOCS, dstOCS);

  // Convert images
  if (sprite->pixelFormat() != doc::IMAGE_INDEXED) {
    for (Cel* cel : sprite->uniqueCels()) {
      ImageRef old_image = cel->imageRef();
      if (old_image.get()->pixelFormat() != IMAGE_TILEMAP) {
        ImageRef new_image = convert_image_color_space(old_image.get(), newCS, conversion.get());

        m_seq.add(new cmd::ReplaceImage(sprite, old_image, new_image));
      }
    }
  }

  if (conversion) {
    // Convert palette
    if (sprite->pixelFormat() != doc::IMAGE_GRAYSCALE) {
      for (auto& pal : sprite->getPalettes()) {
        Palette newPal(pal->frame(), pal->size());

        for (int i = 0; i < pal->size(); ++i) {
          color_t oldCol = pal->entry(i);
          color_t newCol = pal->entry(i);
          conversion->convertRgba((uint32_t*)&newCol, (const uint32_t*)&oldCol, 1);
          newPal.setEntry(i, newCol);
        }

        if (*pal != newPal)
          m_seq.add(new cmd::SetPalette(sprite, pal->frame(), &newPal));
      }
    }
  }

  m_seq.add(new cmd::AssignColorProfile(sprite, newCS));
}

void ConvertColorProfile::onExecute()
{
  m_seq.execute(context());
}

void ConvertColorProfile::onUndo()
{
  m_seq.undo();
}

void ConvertColorProfile::onRedo()
{
  m_seq.redo();
}

}} // namespace app::cmd
