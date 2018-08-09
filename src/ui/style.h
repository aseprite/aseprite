// Aseprite UI Library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_STYLE_H_INCLUDED
#define UI_STYLE_H_INCLUDED
#pragma once

#include "gfx/border.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/base.h"

#include <string>
#include <vector>

namespace os {
  class Font;
  class Surface;
}

namespace ui {

  class Style {
  public:
    class Layer {
    public:
      enum class Type {
        kNone,
        kBackground,
        kBackgroundBorder,
        kBorder,
        kIcon,
        kText,
        kNewLayer,
      };

      // Flags (in which state the widget must be to show this layer)
      enum {
        kMouse = 1,
        kFocus = 2,
        kSelected = 4,
        kDisabled = 8,
      };

      Layer()
        : m_type(Type::kNone)
        , m_flags(0)
        , m_align(CENTER | MIDDLE)
        , m_color(gfx::ColorNone)
        , m_icon(nullptr)
        , m_spriteSheet(nullptr)
        , m_offset(0, 0) {
      }

      Type type() const { return m_type; }
      int flags() const { return m_flags; }
      int align() const { return m_align; }

      gfx::Color color() const { return m_color; }
      os::Surface* icon() const { return m_icon; }
      os::Surface* spriteSheet() const { return m_spriteSheet; }
      const gfx::Rect& spriteBounds() const { return m_spriteBounds; }
      const gfx::Rect& slicesBounds() const { return m_slicesBounds; }
      const gfx::Point& offset() const { return m_offset; }

      void setType(const Type type) { m_type = type; }
      void setFlags(const int flags) { m_flags = flags; }
      void setAlign(const int align) { m_align = align; }
      void setColor(gfx::Color color) { m_color = color; }
      void setIcon(os::Surface* icon) { m_icon = icon; }
      void setSpriteSheet(os::Surface* spriteSheet) { m_spriteSheet = spriteSheet; }
      void setSpriteBounds(const gfx::Rect& bounds) { m_spriteBounds = bounds; }
      void setSlicesBounds(const gfx::Rect& bounds) { m_slicesBounds = bounds; }
      void setOffset(const gfx::Point& offset) { m_offset = offset; }

    private:
      Type m_type;
      int m_flags;
      int m_align;
      gfx::Color m_color;
      os::Surface* m_icon;
      os::Surface* m_spriteSheet;
      gfx::Rect m_spriteBounds;
      gfx::Rect m_slicesBounds;
      gfx::Point m_offset;
    };

    typedef std::vector<Layer> Layers;

    static gfx::Border UndefinedBorder();

    Style(const Style* base);

    const std::string& id() const { return m_id; }
    const gfx::Border& margin() const { return m_margin; }
    const gfx::Border& border() const { return m_border; }
    const gfx::Border& padding() const { return m_padding; }
    os::Font* font() const { return m_font; }
    const Layers& layers() const { return m_layers; }
    Layers& layers() { return m_layers; }

    void setId(const std::string& id) { m_id = id; }
    void setMargin(const gfx::Border& value) { m_margin = value; }
    void setBorder(const gfx::Border& value) { m_border = value; }
    void setPadding(const gfx::Border& value) { m_padding = value; }
    void setFont(os::Font* font) { m_font = font; }
    void addLayer(const Layer& layer);

  private:
    std::string m_id;         // Just for debugging purposes
    Layers m_layers;
    int m_insertionPoint;
    gfx::Border m_margin;
    gfx::Border m_border;
    gfx::Border m_padding;
    os::Font* m_font;
  };

} // namespace ui

#endif
