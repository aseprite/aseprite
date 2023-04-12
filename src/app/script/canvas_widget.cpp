// Aseprite
// Copyright (c) 2022-2023  Igara Studio S.A.
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
#include "ui/system.h"

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

// static
bool Canvas::s_stopKeyEventPropagation = false;

// static
void Canvas::stopKeyEventPropagation()
{
  s_stopKeyEventPropagation = true;
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
  GraphicsContext gc(m_surface, m_autoScaling ? ui::guiscale() : 1);
  if (m_autoScaling) {
    auto theme = skin::SkinTheme::get(this);
    gc.font(AddRef(theme->getUnscaledFont(font())));
  }
  else
    gc.font(AddRef(font()));

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

template <typename T>
static T makeScaled(T* msg, const gfx::Point& offset)
{
  static_assert(std::is_base_of<ui::Message, T>::value, "type parameter must derive from ui::Message");
  auto result = T(msg->type(), *msg, ((msg->position() - offset) / ui::guiscale()) + offset);
  result.setRecipient(static_cast<ui::Message*>(msg)->recipient());
  result.setDisplay(static_cast<ui::Message*>(msg)->display());
  return result;
}

bool Canvas::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case ui::kKeyDownMessage:
      if (hasFocus()) {
        s_stopKeyEventPropagation = false;
        auto keyMsg = static_cast<ui::KeyMessage*>(msg);
        KeyDown(keyMsg);
        if (s_stopKeyEventPropagation)
          return true;
      }
      break;

    case ui::kKeyUpMessage:
      if (hasFocus()) {
        s_stopKeyEventPropagation = false;
        auto keyMsg = static_cast<ui::KeyMessage*>(msg);
        KeyUp(keyMsg);
        if (s_stopKeyEventPropagation)
          return true;
      }
      break;

    case ui::kSetCursorMessage:
      ui::set_mouse_cursor(m_cursorType);
      return true;

    case ui::kMouseMoveMessage: {
      auto mouseMsg = *static_cast<ui::MouseMessage*>(msg);
      if (m_autoScaling) {
        mouseMsg = makeScaled(&mouseMsg, bounds().origin());
      }
      MouseMove(&mouseMsg);
      break;
    }

    case ui::kMouseDownMessage: {
      if (!hasCapture())
        captureMouse();

      if (isFocusStop() && !hasFocus())
        requestFocus();

      auto mouseMsg = *static_cast<ui::MouseMessage*>(msg);
      if (m_autoScaling) {
        mouseMsg = makeScaled(&mouseMsg, bounds().origin());
      }
      MouseDown(&mouseMsg);
      break;
    }

    case ui::kMouseUpMessage: {
      auto mouseMsg = *static_cast<ui::MouseMessage*>(msg);
      if (m_autoScaling) {
        mouseMsg = makeScaled(&mouseMsg, bounds().origin());
      }
      MouseUp(&mouseMsg);

      if (hasCapture())
        releaseMouse();
      break;
    }

    case ui::kDoubleClickMessage: {
      auto mouseMsg = *static_cast<ui::MouseMessage*>(msg);
      if (m_autoScaling) {
        mouseMsg = makeScaled(&mouseMsg, bounds().origin());
      }
      DoubleClick(&mouseMsg);
      break;
    }

    case ui::kMouseWheelMessage: {
      auto mouseMsg = *static_cast<ui::MouseMessage*>(msg);
      if (m_autoScaling) {
        mouseMsg = makeScaled(&mouseMsg, bounds().origin());
      }
      Wheel(&mouseMsg);
      break;
    }

    case ui::kTouchMagnifyMessage: {
      auto touchMsg = *static_cast<ui::TouchMessage*>(msg);
      if (m_autoScaling) {
        touchMsg = makeScaled(&touchMsg, bounds().origin());
      }
      TouchMagnify(&touchMsg);
      break;
    }

  }
  return ui::Widget::onProcessMessage(msg);
}

void Canvas::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);
  if (os::instance() && !ev.bounds().isEmpty()) {
    int w = ev.bounds().w;
    int h = ev.bounds().h;

    if (m_autoScaling) {
      w = w/ui::guiscale() + (w % ui::guiscale());
      h = h/ui::guiscale() + (h % ui::guiscale());
    }

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
  gfx::Rect rc = clientBounds();
  if (m_surface) {
    if (m_autoScaling) {
      rc.w += (rc.w % ui::guiscale());
      rc.h += (rc.h % ui::guiscale());
    }
    g->drawSurface(m_surface.get(), m_surface->bounds(), rc, os::Sampling(), nullptr);
  }
}

} // namespace script
} // namespace app

#endif // ENABLE_UI
