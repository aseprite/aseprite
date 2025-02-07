// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/random_image.h"

#include "doc/dispatch.h"
#include "doc/image.h"
#include "doc/image_impl.h"

#include <random>

namespace doc { namespace algorithm {

template<typename ImageTraits>
void random_image_templ(Image* image)
{
  using pixel_t = typename ImageTraits::pixel_t;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<color_t> dist(ImageTraits::min_value, ImageTraits::max_value);

  doc::transform_image<ImageTraits>(image, [&](pixel_t) -> pixel_t {
    color_t v = dist(gen);
    if constexpr (ImageTraits::color_mode == ColorMode::RGB) {
      v = rgba_seta(v, 255);
    }
    else if constexpr (ImageTraits::color_mode == ColorMode::GRAYSCALE) {
      v = graya_seta(v, 255);
    }
    return v;
  });
}

void random_image(Image* image)
{
  DOC_DISPATCH_BY_COLOR_MODE(image->colorMode(), random_image_templ, image);
}

}} // namespace doc::algorithm
