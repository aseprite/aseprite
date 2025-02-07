// Aseprite Document Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/blend_image.h"

#include "doc/blend_internals.h"
#include "doc/image_impl.h"

namespace doc {

template<typename DstTraits, typename SrcTraits>
void blend_image_templ(Image* dst,
                       const Image* src,
                       const gfx::Clip& area,
                       const Palette* pal,
                       const int opacity,
                       const BlendMode blendMode)
{
  if constexpr (DstTraits::color_mode == ColorMode::INDEXED ||
                SrcTraits::color_mode == ColorMode::INDEXED) {
    ASSERT(pal != nullptr);
    if (pal == nullptr)
      return;
  }
  BlenderHelper<DstTraits, SrcTraits> blender(dst, src, pal, blendMode, true);
  LockImageBits<DstTraits> dstBits(dst);
  const LockImageBits<SrcTraits> srcBits(src);
  auto dstIt = dstBits.begin_area(area.dstBounds());
  auto srcIt = srcBits.begin_area(area.srcBounds());
  auto dstEnd = dstBits.end_area(area.dstBounds());
  for (; dstIt < dstEnd; ++dstIt, ++srcIt)
    *dstIt = blender(*dstIt, *srcIt, opacity);
}

void blend_image(Image* dst,
                 const Image* src,
                 gfx::Clip area,
                 const Palette* pal,
                 const int opacity,
                 const BlendMode blendMode)
{
  if (!area.clip(dst->width(), dst->height(), src->width(), src->height()))
    return;

  switch (dst->pixelFormat()) {
    case IMAGE_RGB:
      switch (src->pixelFormat()) {
        case IMAGE_RGB:
          return blend_image_templ<RgbTraits, RgbTraits>(dst, src, area, pal, opacity, blendMode);
        case IMAGE_GRAYSCALE:
          return blend_image_templ<RgbTraits, GrayscaleTraits>(dst,
                                                               src,
                                                               area,
                                                               pal,
                                                               opacity,
                                                               blendMode);
        case IMAGE_INDEXED:
          return blend_image_templ<RgbTraits, IndexedTraits>(dst,
                                                             src,
                                                             area,
                                                             pal,
                                                             opacity,
                                                             blendMode);
      }
      break;

    case IMAGE_GRAYSCALE:
      switch (src->pixelFormat()) {
        case IMAGE_RGB:
          return blend_image_templ<GrayscaleTraits, RgbTraits>(dst,
                                                               src,
                                                               area,
                                                               pal,
                                                               opacity,
                                                               blendMode);
        case IMAGE_GRAYSCALE:
          return blend_image_templ<GrayscaleTraits, GrayscaleTraits>(dst,
                                                                     src,
                                                                     area,
                                                                     pal,
                                                                     opacity,
                                                                     blendMode);
        case IMAGE_INDEXED:
          return blend_image_templ<GrayscaleTraits, IndexedTraits>(dst,
                                                                   src,
                                                                   area,
                                                                   pal,
                                                                   opacity,
                                                                   blendMode);
      }
      break;

    case IMAGE_INDEXED:
      switch (src->pixelFormat()) {
        case IMAGE_RGB:
          return blend_image_templ<IndexedTraits, RgbTraits>(dst,
                                                             src,
                                                             area,
                                                             pal,
                                                             opacity,
                                                             blendMode);
        case IMAGE_GRAYSCALE:
          return blend_image_templ<IndexedTraits, GrayscaleTraits>(dst,
                                                                   src,
                                                                   area,
                                                                   pal,
                                                                   opacity,
                                                                   blendMode);
        case IMAGE_INDEXED:
          return blend_image_templ<IndexedTraits, IndexedTraits>(dst,
                                                                 src,
                                                                 area,
                                                                 pal,
                                                                 opacity,
                                                                 blendMode);
      }
      break;

    case IMAGE_TILEMAP: return dst->copy(src, area);
  }
}

} // namespace doc
