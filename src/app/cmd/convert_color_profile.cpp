// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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

namespace app {
namespace cmd {

void convert_color_profile(doc::Sprite* sprite, const gfx::ColorSpacePtr& newCS)
{
  os::System* system = os::instance();

  ASSERT(sprite->colorSpace());
  ASSERT(newCS);

  auto srcOCS = system->createColorSpace(sprite->colorSpace());
  auto dstOCS = system->createColorSpace(newCS);

  ASSERT(srcOCS);
  ASSERT(dstOCS);

  auto conversion = system->convertBetweenColorSpace(srcOCS, dstOCS);
  // Convert images
  if (sprite->pixelFormat() == doc::IMAGE_RGB) {
    for (Cel* cel : sprite->uniqueCels()) {
      ImageRef old_image = cel->imageRef();

      ImageSpec spec = old_image->spec();
      spec.setColorSpace(newCS);
      ImageRef new_image(Image::create(spec));

      if (conversion) {
        for (int y=0; y<spec.height(); ++y) {
          conversion->convert((uint32_t*)new_image->getPixelAddress(0, y),
                              (const uint32_t*)old_image->getPixelAddress(0, y),
                              spec.width());
        }
      }
      else {
        new_image->copy(old_image.get(), gfx::Clip(0, 0, old_image->bounds()));
      }

      sprite->replaceImage(old_image->id(), new_image);
    }
  }

  if (conversion) {
    // Convert palette
    if (sprite->pixelFormat() != doc::IMAGE_GRAYSCALE) {
      for (auto& pal : sprite->getPalettes()) {
        Palette newPal(pal->frame(), pal->size());

        for (int i=0; i<pal->size(); ++i) {
          color_t oldCol = pal->entry(i);
          color_t newCol = pal->entry(i);
          conversion->convert((uint32_t*)&newCol,
                              (const uint32_t*)&oldCol, 1);
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

ConvertColorProfile::ConvertColorProfile(doc::Sprite* sprite, const gfx::ColorSpacePtr& newCS)
  : WithSprite(sprite)
{
  os::System* system = os::instance();

  ASSERT(sprite->colorSpace());
  ASSERT(newCS);

  auto srcOCS = system->createColorSpace(sprite->colorSpace());
  auto dstOCS = system->createColorSpace(newCS);

  ASSERT(srcOCS);
  ASSERT(dstOCS);

  auto conversion = system->convertBetweenColorSpace(srcOCS, dstOCS);
  // Convert images
  if (sprite->pixelFormat() == doc::IMAGE_RGB) {
    for (Cel* cel : sprite->uniqueCels()) {
      ImageRef old_image = cel->imageRef();

      ImageSpec spec = old_image->spec();
      spec.setColorSpace(newCS);
      ImageRef new_image(Image::create(spec));

      if (conversion) {
        for (int y=0; y<spec.height(); ++y) {
          conversion->convert((uint32_t*)new_image->getPixelAddress(0, y),
                              (const uint32_t*)old_image->getPixelAddress(0, y),
                              spec.width());
        }
      }
      else {
        new_image->copy(old_image.get(), gfx::Clip(0, 0, old_image->bounds()));
      }

      m_seq.add(new cmd::ReplaceImage(sprite, old_image, new_image));
    }
  }

  if (conversion) {
    // Convert palette
    if (sprite->pixelFormat() != doc::IMAGE_GRAYSCALE) {
      for (auto& pal : sprite->getPalettes()) {
        Palette newPal(pal->frame(), pal->size());

        for (int i=0; i<pal->size(); ++i) {
          color_t oldCol = pal->entry(i);
          color_t newCol = pal->entry(i);
          conversion->convert((uint32_t*)&newCol,
                              (const uint32_t*)&oldCol, 1);
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

} // namespace cmd
} // namespace app
