// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_BLEND_MODE_H_INCLUDED
#define APP_SCRIPT_BLEND_MODE_H_INCLUDED
#pragma once

#include "base/convert_to.h"
#include "doc/blend_mode.h"
#include "os/paint.h"

namespace app { namespace script {

// Blend modes for doc::BlendMode and os::BlendMode, used in
// Layer.blendMode and GraphicsContext.blendMode.
enum class BlendMode : int {
  CLEAR,
  SRC,
  DST,
  SRC_OVER,
  DST_OVER,
  SRC_IN,
  DST_IN,
  SRC_OUT,
  DST_OUT,
  SRC_ATOP,
  DST_ATOP,
  XOR,
  PLUS,
  MODULATE,
  MULTIPLY,
  SCREEN,
  OVERLAY,
  DARKEN,
  LIGHTEN,
  COLOR_DODGE,
  COLOR_BURN,
  HARD_LIGHT,
  SOFT_LIGHT,
  DIFFERENCE,
  EXCLUSION,
  HUE,
  SATURATION,
  COLOR,
  LUMINOSITY,
  ADDITION,
  SUBTRACT,
  DIVIDE,
};

}} // namespace app::script

namespace base {

template<>
inline os::BlendMode convert_to(const app::script::BlendMode& from)
{
  switch (from) {
    case app::script::BlendMode::CLEAR:       return os::BlendMode::Clear;
    case app::script::BlendMode::SRC:         return os::BlendMode::Src;
    case app::script::BlendMode::DST:         return os::BlendMode::Dst;
    default:
    case app::script::BlendMode::SRC_OVER:    return os::BlendMode::SrcOver;
    case app::script::BlendMode::DST_OVER:    return os::BlendMode::DstOver;
    case app::script::BlendMode::SRC_IN:      return os::BlendMode::SrcIn;
    case app::script::BlendMode::DST_IN:      return os::BlendMode::DstIn;
    case app::script::BlendMode::SRC_OUT:     return os::BlendMode::SrcOut;
    case app::script::BlendMode::DST_OUT:     return os::BlendMode::DstOut;
    case app::script::BlendMode::SRC_ATOP:    return os::BlendMode::SrcATop;
    case app::script::BlendMode::DST_ATOP:    return os::BlendMode::DstATop;
    case app::script::BlendMode::XOR:         return os::BlendMode::Xor;
    case app::script::BlendMode::PLUS:        return os::BlendMode::Plus;
    case app::script::BlendMode::MODULATE:    return os::BlendMode::Modulate;
    case app::script::BlendMode::MULTIPLY:    return os::BlendMode::Multiply;
    case app::script::BlendMode::SCREEN:      return os::BlendMode::Screen;
    case app::script::BlendMode::OVERLAY:     return os::BlendMode::Overlay;
    case app::script::BlendMode::DARKEN:      return os::BlendMode::Darken;
    case app::script::BlendMode::LIGHTEN:     return os::BlendMode::Lighten;
    case app::script::BlendMode::COLOR_DODGE: return os::BlendMode::ColorDodge;
    case app::script::BlendMode::COLOR_BURN:  return os::BlendMode::ColorBurn;
    case app::script::BlendMode::HARD_LIGHT:  return os::BlendMode::HardLight;
    case app::script::BlendMode::SOFT_LIGHT:  return os::BlendMode::SoftLight;
    case app::script::BlendMode::DIFFERENCE:  return os::BlendMode::Difference;
    case app::script::BlendMode::EXCLUSION:   return os::BlendMode::Exclusion;
    case app::script::BlendMode::HUE:         return os::BlendMode::Hue;
    case app::script::BlendMode::SATURATION:  return os::BlendMode::Saturation;
    case app::script::BlendMode::COLOR:       return os::BlendMode::Color;
    case app::script::BlendMode::LUMINOSITY:  return os::BlendMode::Luminosity;
    case app::script::BlendMode::ADDITION:    return os::BlendMode::Plus;
    // Use the default value for undefined conversions
    case app::script::BlendMode::SUBTRACT:
    case app::script::BlendMode::DIVIDE:      return os::BlendMode::SrcOver;
  }
}

template<>
inline app::script::BlendMode convert_to(const os::BlendMode& from)
{
  switch (from) {
    case os::BlendMode::Clear:      return app::script::BlendMode::CLEAR;
    case os::BlendMode::Src:        return app::script::BlendMode::SRC;
    case os::BlendMode::Dst:        return app::script::BlendMode::DST;
    default:
    case os::BlendMode::SrcOver:    return app::script::BlendMode::SRC_OVER;
    case os::BlendMode::DstOver:    return app::script::BlendMode::DST_OVER;
    case os::BlendMode::SrcIn:      return app::script::BlendMode::SRC_IN;
    case os::BlendMode::DstIn:      return app::script::BlendMode::DST_IN;
    case os::BlendMode::SrcOut:     return app::script::BlendMode::SRC_OUT;
    case os::BlendMode::DstOut:     return app::script::BlendMode::DST_OUT;
    case os::BlendMode::SrcATop:    return app::script::BlendMode::SRC_ATOP;
    case os::BlendMode::DstATop:    return app::script::BlendMode::DST_ATOP;
    case os::BlendMode::Xor:        return app::script::BlendMode::XOR;
    case os::BlendMode::Plus:       return app::script::BlendMode::PLUS;
    case os::BlendMode::Modulate:   return app::script::BlendMode::MODULATE;
    case os::BlendMode::Screen:     return app::script::BlendMode::SCREEN;
    case os::BlendMode::Overlay:    return app::script::BlendMode::OVERLAY;
    case os::BlendMode::Darken:     return app::script::BlendMode::DARKEN;
    case os::BlendMode::Lighten:    return app::script::BlendMode::LIGHTEN;
    case os::BlendMode::ColorDodge: return app::script::BlendMode::COLOR_DODGE;
    case os::BlendMode::ColorBurn:  return app::script::BlendMode::COLOR_BURN;
    case os::BlendMode::HardLight:  return app::script::BlendMode::HARD_LIGHT;
    case os::BlendMode::SoftLight:  return app::script::BlendMode::SOFT_LIGHT;
    case os::BlendMode::Difference: return app::script::BlendMode::DIFFERENCE;
    case os::BlendMode::Exclusion:  return app::script::BlendMode::EXCLUSION;
    case os::BlendMode::Multiply:   return app::script::BlendMode::MULTIPLY;
    case os::BlendMode::Hue:        return app::script::BlendMode::HUE;
    case os::BlendMode::Saturation: return app::script::BlendMode::SATURATION;
    case os::BlendMode::Color:      return app::script::BlendMode::COLOR;
    case os::BlendMode::Luminosity: return app::script::BlendMode::LUMINOSITY;
  }
}

template<>
inline doc::BlendMode convert_to(const app::script::BlendMode& from)
{
  switch (from) {
    case app::script::BlendMode::SRC:         return doc::BlendMode::SRC;
    default:
    case app::script::BlendMode::SRC_OVER:    return doc::BlendMode::NORMAL;
    case app::script::BlendMode::PLUS:        return doc::BlendMode::ADDITION;
    case app::script::BlendMode::MULTIPLY:    return doc::BlendMode::MULTIPLY;
    case app::script::BlendMode::SCREEN:      return doc::BlendMode::SCREEN;
    case app::script::BlendMode::OVERLAY:     return doc::BlendMode::OVERLAY;
    case app::script::BlendMode::DARKEN:      return doc::BlendMode::DARKEN;
    case app::script::BlendMode::LIGHTEN:     return doc::BlendMode::LIGHTEN;
    case app::script::BlendMode::COLOR_DODGE: return doc::BlendMode::COLOR_DODGE;
    case app::script::BlendMode::COLOR_BURN:  return doc::BlendMode::COLOR_BURN;
    case app::script::BlendMode::HARD_LIGHT:  return doc::BlendMode::HARD_LIGHT;
    case app::script::BlendMode::SOFT_LIGHT:  return doc::BlendMode::SOFT_LIGHT;
    case app::script::BlendMode::DIFFERENCE:  return doc::BlendMode::DIFFERENCE;
    case app::script::BlendMode::EXCLUSION:   return doc::BlendMode::EXCLUSION;
    case app::script::BlendMode::HUE:         return doc::BlendMode::HSL_HUE;
    case app::script::BlendMode::SATURATION:  return doc::BlendMode::HSL_SATURATION;
    case app::script::BlendMode::COLOR:       return doc::BlendMode::HSL_COLOR;
    case app::script::BlendMode::LUMINOSITY:  return doc::BlendMode::HSL_LUMINOSITY;
    case app::script::BlendMode::ADDITION:    return doc::BlendMode::ADDITION;
    case app::script::BlendMode::SUBTRACT:    return doc::BlendMode::SUBTRACT;
    case app::script::BlendMode::DIVIDE:      return doc::BlendMode::DIVIDE;
    // Use the default value for undefined conversions
    case app::script::BlendMode::CLEAR:
    case app::script::BlendMode::DST:
    case app::script::BlendMode::DST_OVER:
    case app::script::BlendMode::SRC_IN:
    case app::script::BlendMode::DST_IN:
    case app::script::BlendMode::SRC_OUT:
    case app::script::BlendMode::DST_OUT:
    case app::script::BlendMode::SRC_ATOP:
    case app::script::BlendMode::DST_ATOP:
    case app::script::BlendMode::XOR:
    case app::script::BlendMode::MODULATE:    return doc::BlendMode::NORMAL;
  }
}

template<>
inline app::script::BlendMode convert_to(const doc::BlendMode& from)
{
  switch (from) {
    case doc::BlendMode::SRC:            return app::script::BlendMode::SRC;
    default:
    case doc::BlendMode::NORMAL:         return app::script::BlendMode::SRC_OVER;
    case doc::BlendMode::MULTIPLY:       return app::script::BlendMode::MULTIPLY;
    case doc::BlendMode::SCREEN:         return app::script::BlendMode::SCREEN;
    case doc::BlendMode::OVERLAY:        return app::script::BlendMode::OVERLAY;
    case doc::BlendMode::DARKEN:         return app::script::BlendMode::DARKEN;
    case doc::BlendMode::LIGHTEN:        return app::script::BlendMode::LIGHTEN;
    case doc::BlendMode::COLOR_DODGE:    return app::script::BlendMode::COLOR_DODGE;
    case doc::BlendMode::COLOR_BURN:     return app::script::BlendMode::COLOR_BURN;
    case doc::BlendMode::HARD_LIGHT:     return app::script::BlendMode::HARD_LIGHT;
    case doc::BlendMode::SOFT_LIGHT:     return app::script::BlendMode::SOFT_LIGHT;
    case doc::BlendMode::DIFFERENCE:     return app::script::BlendMode::DIFFERENCE;
    case doc::BlendMode::EXCLUSION:      return app::script::BlendMode::EXCLUSION;
    case doc::BlendMode::HSL_HUE:        return app::script::BlendMode::HUE;
    case doc::BlendMode::HSL_SATURATION: return app::script::BlendMode::SATURATION;
    case doc::BlendMode::HSL_COLOR:      return app::script::BlendMode::COLOR;
    case doc::BlendMode::HSL_LUMINOSITY: return app::script::BlendMode::LUMINOSITY;
    case doc::BlendMode::ADDITION:       return app::script::BlendMode::ADDITION;
    case doc::BlendMode::SUBTRACT:       return app::script::BlendMode::SUBTRACT;
    case doc::BlendMode::DIVIDE:         return app::script::BlendMode::DIVIDE;
  }
}

} // namespace base

#endif
