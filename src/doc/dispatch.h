// Aseprite Document Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_DISPATCH_H_INCLUDED
#define DOC_DISPATCH_H_INCLUDED
#pragma once

#include "doc/color_mode.h"
#include "doc/image_traits.h"

// Calls the given func<ImageTraits>() with the parameters "..."  for
// each color mode implementation (RGB, Grayscale, etc.) depending on
// the given "colorMode".
#define DOC_DISPATCH_BY_COLOR_MODE(colorMode, func, ...)                                           \
  switch (colorMode) {                                                                             \
    case doc::ColorMode::RGB:       return func<doc::RgbTraits>(__VA_ARGS__);                      \
    case doc::ColorMode::GRAYSCALE: return func<doc::GrayscaleTraits>(__VA_ARGS__);                \
    case doc::ColorMode::INDEXED:   return func<doc::IndexedTraits>(__VA_ARGS__);                  \
    case doc::ColorMode::BITMAP:    return func<doc::BitmapTraits>(__VA_ARGS__);                   \
    case doc::ColorMode::TILEMAP:   return func<doc::TilemapTraits>(__VA_ARGS__);                    \
  }

#define DOC_DISPATCH_BY_COLOR_MODE_EXCLUDE_BITMAP(colorMode, func, ...)                            \
  switch (colorMode) {                                                                             \
    case doc::ColorMode::RGB:       return func<doc::RgbTraits>(__VA_ARGS__);                      \
    case doc::ColorMode::GRAYSCALE: return func<doc::GrayscaleTraits>(__VA_ARGS__);                \
    case doc::ColorMode::INDEXED:   return func<doc::IndexedTraits>(__VA_ARGS__);                  \
    case doc::ColorMode::TILEMAP:   return func<doc::TilemapTraits>(__VA_ARGS__);                  \
    case doc::ColorMode::BITMAP:    ASSERT(false); break;                                             \
  }

#endif
