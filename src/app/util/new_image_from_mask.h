// Aseprite
// Copyright (C) 2019-2024 Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_NEW_IMAGE_FROM_MASK_H_INCLUDED
#define APP_UTIL_NEW_IMAGE_FROM_MASK_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {
  class Image;
  class Mask;
  class Layer;
}

namespace app {
  class Site;

  doc::Image* new_image_from_mask(const Site& site,
                                  const bool newBlend);
  doc::Image* new_image_from_mask(const Site& site,
                                  const doc::Mask* mask,
                                  const bool newBlend,
                                  bool merged = false);
  doc::Image* new_image_from_mask(const doc::Layer& layer,
                                  doc::frame_t frame,
                                  const doc::Mask* srcMask,
                                  const bool newBlend);
  doc::Image* new_tilemap_from_mask(const Site& site,
                                    const doc::Mask* mask);

  void copy_masked_zones(doc::Image* dst,
                        const doc::Image* src,
                        const doc::Mask* srcMask,
                        int srcXoffset, int srcYoffset);

} // namespace app

#endif
