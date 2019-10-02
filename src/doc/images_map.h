// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IMAGES_MAP_H_INCLUDED
#define DOC_IMAGES_MAP_H_INCLUDED
#pragma once

#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"

#include <unordered_map>

namespace doc {
  namespace details {

    struct image_hash {
      size_t operator()(const ImageRef& i) const {
        return calculate_image_hash(i.get(), i->bounds());
      }
    };

    struct image_eq {
      bool operator()(const ImageRef& a, const ImageRef& b) const {
        return is_same_image(a.get(), b.get());
      }
    };

  }

  // A hash table used to match Image pixels data <-> an index
  typedef std::unordered_map<ImageRef,
                             uint32_t,
                             details::image_hash,
                             details::image_eq> ImagesMap;

} // namespace doc

#endif
