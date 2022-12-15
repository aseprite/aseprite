// Aseprite
// Copyright (c) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_GRAPHICS_CONTEXT_H_INCLUDED
#define APP_SCRIPT_GRAPHICS_CONTEXT_H_INCLUDED
#pragma once

#ifdef ENABLE_UI

#include "os/font.h"
#include "os/paint.h"
#include "os/surface.h"

#include <stack>

namespace doc {
  class Image;
}

namespace app {
namespace script {

class GraphicsContext {
public:
  GraphicsContext(const os::SurfaceRef& surface) : m_surface(surface) { }
  GraphicsContext(GraphicsContext&& gc) {
    std::swap(m_surface, gc.m_surface);
    std::swap(m_paint, gc.m_paint);
    std::swap(m_font, gc.m_font);
  }

  os::FontRef font() const { return m_font; }
  void font(const os::FontRef& font) { m_font = font; }

  int width() const { return m_surface->width(); }
  int height() const { return m_surface->height(); }

  void save() {
    m_saved.push(m_paint);
    m_surface->save();
  }

  void restore() {
    if (!m_saved.empty()) {
      m_paint = m_saved.top();
      m_saved.pop();
      m_surface->restore();
    }
  }

  bool antialias() const { return m_paint.antialias(); }
  void antialias(bool value) { m_paint.antialias(value); }

  gfx::Color color() const { return m_paint.color(); }
  void color(gfx::Color color) { m_paint.color(color); }

  float strokeWidth() const { return m_paint.strokeWidth(); }
  void strokeWidth(float value) { m_paint.strokeWidth(value); }

  void strokeRect(const gfx::Rect& rc) {
    m_paint.style(os::Paint::Stroke);
    m_surface->drawRect(rc, m_paint);
  }

  void fillRect(const gfx::Rect& rc) {
    m_paint.style(os::Paint::Fill);
    m_surface->drawRect(rc, m_paint);
  }

  void fillText(const std::string& text, int x, int y);
  gfx::Size measureText(const std::string& text) const;

  void drawImage(const doc::Image* img, int x, int y);

private:
  os::SurfaceRef m_surface = nullptr;
  os::Paint m_paint;
  os::FontRef m_font;
  std::stack<os::Paint> m_saved;
};

} // namespace script
} // namespace app

#endif

#endif
