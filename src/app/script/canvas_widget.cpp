// Aseprite
// Copyright (c) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/script/canvas_widget.h"

#include "app/script/graphics_context.h"
#include "app/ui/skin/skin_theme.h"
#include "os/system.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"

#ifdef ENABLE_UI

namespace app {
namespace script {

// static
ui::WidgetType Canvas::Type()
{
  static ui::WidgetType type = ui::kGenericWidget;
  if (type == ui::kGenericWidget)
    type = ui::register_widget_type();
  return type;
}

Canvas::Canvas() : ui::Widget(Type())
{
}

void Canvas::callPaint()
{
  if (!m_surface)
    return;

  os::Paint p;
  p.color(bgColor());
  m_surface->drawRect(m_surface->bounds(), p);

  // Draw only on resize (onPaint we draw the cached m_surface)
  GraphicsContext gc(m_surface);
  Paint(gc);
}

void Canvas::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  gfx::Color bg;
  if (auto theme = skin::SkinTheme::get(this))
    bg = theme->colors.windowFace();
  else
    bg = gfx::rgba(0, 0, 0);
  setBgColor(bg);
}

bool Canvas::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case ui::kMouseMoveMessage: {
      auto mouseMsg = static_cast<ui::MouseMessage*>(msg);
      MouseMove(mouseMsg);
      break;
    }

    case ui::kMouseDownMessage: {
      auto mouseMsg = static_cast<ui::MouseMessage*>(msg);
      MouseDown(mouseMsg);
      break;
    }

    case ui::kMouseUpMessage: {
      auto mouseMsg = static_cast<ui::MouseMessage*>(msg);
      MouseUp(mouseMsg);
      break;
    }

  }
  return ui::Widget::onProcessMessage(msg);
}

void Canvas::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);
  if (os::instance() && !ev.bounds().isEmpty()) {
    const int w = ev.bounds().w;
    const int h = ev.bounds().h;

    if (!m_surface ||
        m_surface->width() != w ||
        m_surface->height() != h) {
      m_surface = os::instance()->makeSurface(w, h);

      callPaint();
    }
  }
  else
    m_surface.reset();
}

void Canvas::onPaint(ui::PaintEvent& ev)
{
  auto g = ev.graphics();
  const gfx::Rect rc = clientBounds();
  if (m_surface)
    g->drawSurface(m_surface.get(), rc.x, rc.y);
}

} // namespace script
} // namespace app

#endif // ENABLE_UI
