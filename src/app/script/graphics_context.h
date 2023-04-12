// Aseprite
// Copyright (c) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_GRAPHICS_CONTEXT_H_INCLUDED
#define APP_SCRIPT_GRAPHICS_CONTEXT_H_INCLUDED
#pragma once

#ifdef ENABLE_UI

#include "gfx/path.h"
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
  GraphicsContext(const os::SurfaceRef& surface, int uiscale) : m_surface(surface), m_uiscale(uiscale) { }
  GraphicsContext(GraphicsContext&& gc) {
    std::swap(m_surface, gc.m_surface);
    std::swap(m_paint, gc.m_paint);
    std::swap(m_font, gc.m_font);
    std::swap(m_path, gc.m_path);
    m_uiscale = gc.m_uiscale;
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

#if LAF_SKIA
  int opacity() const { return m_paint.skPaint().getAlpha(); }
  void opacity(int value) { m_paint.skPaint().setAlpha(value); }
#else
  int opacity() const { return 255; }
  void opacity(int) { }
#endif

  os::BlendMode blendMode() const { return m_paint.blendMode(); }
  void blendMode(const os::BlendMode bm) { m_paint.blendMode(bm); }

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
  void drawImage(const doc::Image* img,
                 const gfx::Rect& srcRc,
                 const gfx::Rect& dstRc);

  void drawThemeImage(const std::string& partId, const gfx::Point& pt);
  void drawThemeRect(const std::string& partId, const gfx::Rect& rc);

  // Path
  void beginPath() { m_path.reset(); }
  void closePath() { m_path.close(); }
  void moveTo(float x, float y) { m_path.moveTo(x, y); }
  void lineTo(float x, float y) { m_path.lineTo(x, y); }
  void cubicTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) {
    m_path.cubicTo(cp1x, cp1y, cp2x, cp2y, x, y);
  }
  void oval(const gfx::Rect& rc) {
    m_path.oval(rc);
  }
  void rect(const gfx::Rect& rc) {
    m_path.rect(rc);
  }
  void roundedRect(const gfx::Rect& rc, float rx, float ry) {
    m_path.roundedRect(rc, rx, ry);
  }
  void stroke();
  void fill();

  void clip() {
    m_surface->clipPath(m_path);
  }

  int uiscale() const {
    return m_uiscale;
  }

private:
  os::SurfaceRef m_surface = nullptr;
  // Keeps the UI Scale currently in use when canvas autoScaling is enabled.
  int m_uiscale;
  os::Paint m_paint;
  os::FontRef m_font;
  gfx::Path m_path;
  std::stack<os::Paint> m_saved;
};

} // namespace script
} // namespace app

#endif

#endif
