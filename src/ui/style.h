// Aseprite UI Library
// Copyright (C) 2020-2025  Igara Studio S.A.
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
#include "os/surface.h"
#include "text/font.h"
#include "ui/base.h"

#include <string>
#include <vector>

namespace os {
class Font;
class Surface;
} // namespace os

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
    enum { kMouse = 1, kFocus = 2, kSelected = 4, kDisabled = 8, kCapture = 16 };

    class IconSurfaceProvider {
    public:
      virtual os::Surface* iconSurface() const = 0;
    };

    Layer()
      : m_type(Type::kNone)
      , m_flags(0)
      , m_align(CENTER | MIDDLE)
      , m_color(gfx::ColorNone)
      , m_icon(nullptr)
      , m_spriteSheet(nullptr)
      , m_offset(0, 0)
    {
    }

    Type type() const { return m_type; }
    int flags() const { return m_flags; }
    int align() const { return m_align; }

    gfx::Color color() const { return m_color; }
    os::Surface* icon() const { return m_icon.get(); }
    os::Surface* spriteSheet() const { return m_spriteSheet.get(); }
    const gfx::Rect& spriteBounds() const { return m_spriteBounds; }
    const gfx::Rect& slicesBounds() const { return m_slicesBounds; }
    const gfx::Point& offset() const { return m_offset; }

    void setType(const Type type) { m_type = type; }
    void setFlags(const int flags) { m_flags = flags; }
    void setAlign(const int align) { m_align = align; }
    void setColor(gfx::Color color) { m_color = color; }
    void setIcon(const os::SurfaceRef& icon) { m_icon = icon; }
    void setSpriteSheet(const os::SurfaceRef& spriteSheet) { m_spriteSheet = spriteSheet; }
    void setSpriteBounds(const gfx::Rect& bounds) { m_spriteBounds = bounds; }
    void setSlicesBounds(const gfx::Rect& bounds) { m_slicesBounds = bounds; }
    void setOffset(const gfx::Point& offset) { m_offset = offset; }

  private:
    Type m_type;
    int m_flags;
    int m_align;
    gfx::Color m_color;
    os::SurfaceRef m_icon;
    os::SurfaceRef m_spriteSheet;
    gfx::Rect m_spriteBounds;
    gfx::Rect m_slicesBounds;
    gfx::Point m_offset;
  };

  typedef std::vector<Layer> Layers;

  static constexpr const int kUndefinedSide = -1;
  static gfx::Border UndefinedBorder();

  static gfx::Size MinSize();
  static gfx::Size MaxSize();

  Style(const Style* base);

  const std::string& id() const { return m_id; }

  // Raw margin/border/padding values which might contain
  // kUndefinedSide values.
  const gfx::Border& rawMargin() const { return m_margin; }
  const gfx::Border& rawBorder() const { return m_border; }
  const gfx::Border& rawPadding() const { return m_padding; }

  gfx::Border margin() const { return normalizeBorder(m_margin); }
  gfx::Border border() const { return normalizeBorder(m_border); }
  gfx::Border padding() const { return normalizeBorder(m_padding); }

  const gfx::Size& minSize() const { return m_minSize; }
  const gfx::Size& maxSize() const { return m_maxSize; }
  const gfx::Size& gap() const { return m_gap; }
  const text::FontRef& font() const { return m_font; }
  const bool mnemonics() const { return m_mnemonics; }
  const Layers& layers() const { return m_layers; }
  Layers& layers() { return m_layers; }

  void setId(const std::string& id) { m_id = id; }
  void setMargin(const gfx::Border& value) { m_margin = value; }
  void setBorder(const gfx::Border& value) { m_border = value; }
  void setPadding(const gfx::Border& value) { m_padding = value; }
  void setMinSize(const gfx::Size& sz);
  void setMaxSize(const gfx::Size& sz);
  void setGap(const gfx::Size& value) { m_gap = value; }
  void setFont(const text::FontRef& font);
  void setMnemonics(const bool enabled) { m_mnemonics = enabled; }
  void addLayer(const Layer& layer);

  static inline void applyOnlyDefinedBorders(gfx::Border& border, const gfx::Border& defBorder)
  {
    if (defBorder.left() != kUndefinedSide)
      border.left(defBorder.left());
    if (defBorder.top() != kUndefinedSide)
      border.top(defBorder.top());
    if (defBorder.right() != kUndefinedSide)
      border.right(defBorder.right());
    if (defBorder.bottom() != kUndefinedSide)
      border.bottom(defBorder.bottom());
  }

private:
  static inline gfx::Border normalizeBorder(const gfx::Border& b)
  {
    return gfx::Border(b.left() == kUndefinedSide ? 0 : b.left(),
                       b.top() == kUndefinedSide ? 0 : b.top(),
                       b.right() == kUndefinedSide ? 0 : b.right(),
                       b.bottom() == kUndefinedSide ? 0 : b.bottom());
  }

  std::string m_id; // Just for debugging purposes
  Layers m_layers;
  int m_insertionPoint;
  gfx::Border m_margin;
  gfx::Border m_border;
  gfx::Border m_padding;
  // Min width and height for the widget.
  gfx::Size m_minSize;
  // Max width and height for the widget.
  gfx::Size m_maxSize;
  // Grid's columns and rows separation in pixels.
  gfx::Size m_gap;
  text::FontRef m_font;
  bool m_mnemonics;
};

} // namespace ui

#endif
