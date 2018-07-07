// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_NEW_IMAGE_FROM_MASK_H_INCLUDED
#define APP_UTIL_NEW_IMAGE_FROM_MASK_H_INCLUDED
#pragma once

namespace doc {
  class Image;
  class Mask;
}

namespace app {
  class Site;

  doc::Image* new_image_from_mask(const Site& site);
  doc::Image* new_image_from_mask(const Site& site,
                                  const doc::Mask* mask,
                                  bool merged = false);

} // namespace app

#endif
