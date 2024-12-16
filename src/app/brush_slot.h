// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_BRUSH_SLOT_H_INCLUDED
#define APP_BRUSH_SLOT_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/shade.h"
#include "app/tools/ink_type.h"
#include "doc/brush.h"

namespace app {

// Custom brush slot
class BrushSlot {
public:
  enum class Flags {
    Locked = 0x0001,
    BrushType = 0x0002,
    BrushSize = 0x0004,
    BrushAngle = 0x0008,
    FgColor = 0x0010,
    BgColor = 0x0020,
    InkType = 0x0040,
    InkOpacity = 0x0080,
    Shade = 0x0100,
    PixelPerfect = 0x0200,
    ImageColor = 0x0400,
  };

  BrushSlot(Flags flags = Flags(0),
            const doc::BrushRef& brush = doc::BrushRef(nullptr),
            const app::Color& fgColor = app::Color::fromMask(),
            const app::Color& bgColor = app::Color::fromMask(),
            tools::InkType inkType = tools::InkType::DEFAULT,
            int inkOpacity = 255,
            const Shade& shade = Shade(),
            bool pixelPerfect = false)
    : m_flags(flags)
    , m_brush(brush)
    , m_fgColor(fgColor)
    , m_bgColor(bgColor)
    , m_inkType(inkType)
    , m_inkOpacity(inkOpacity)
    , m_shade(shade)
    , m_pixelPerfect(pixelPerfect)
  {
  }

  Flags flags() const { return m_flags; }
  void setFlags(Flags flags) { m_flags = flags; }

  bool isEmpty() const { return int(m_flags) == 0; }

  bool hasFlag(Flags flag) const { return ((int(m_flags) & int(flag)) == int(flag)); }

  bool hasBrush() const
  {
    return (brush() &&
            (hasFlag(Flags::BrushType) || hasFlag(Flags::BrushSize) || hasFlag(Flags::BrushAngle)));
  }

  // Can be null if the user deletes the brush.
  doc::BrushRef brush() const { return m_brush; }
  app::Color fgColor() const { return m_fgColor; }
  app::Color bgColor() const { return m_bgColor; }
  tools::InkType inkType() const { return m_inkType; }
  int inkOpacity() const { return m_inkOpacity; }
  const Shade& shade() const { return m_shade; }
  bool pixelPerfect() const { return m_pixelPerfect; }

  // True if the user locked the brush using the shortcut key to
  // access it.
  bool locked() const { return hasFlag(Flags::Locked); }

  void setLocked(bool locked)
  {
    if (locked)
      m_flags = static_cast<Flags>(int(m_flags) | int(Flags::Locked));
    else
      m_flags = static_cast<Flags>(int(m_flags) & ~int(Flags::Locked));
  }

private:
  Flags m_flags;
  doc::BrushRef m_brush;
  app::Color m_fgColor;
  app::Color m_bgColor;
  tools::InkType m_inkType;
  int m_inkOpacity;
  Shade m_shade;
  bool m_pixelPerfect;
};

} // namespace app

#endif
