// Aseprite
// Copyright (C) 2018-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DRAGGABLE_WIDGET_H_INCLUDED
#define APP_UI_DRAGGABLE_WIDGET_H_INCLUDED
#pragma once

#include "os/surface.h"
#include "os/system.h"
#include "ui/display.h"
#include "ui/graphics.h"
#include "ui/layer.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

template<typename Base>
class DraggableWidget : public Base {
public:
  template<typename... Args>
  DraggableWidget(Args... args) : Base(args...)
  {
  }

  ~DraggableWidget()
  {
    if (m_floatingUILayer)
      destroyFloatingUILayer();
  }

  bool onProcessMessage(ui::Message* msg) override
  {
    switch (msg->type()) {
      case ui::kSetCursorMessage:
        if (m_floatingUILayer) {
          const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
          const gfx::Point mousePos = mouseMsg->position();
          if (onCanDropItemsOutside() && !getParentBounds().contains(mousePos)) {
            ui::set_mouse_cursor(ui::kForbiddenCursor);
          }
          else {
            ui::set_mouse_cursor(ui::kMoveCursor);
          }
          return true;
        }
        break;

      case ui::kMouseDownMessage: {
        const bool wasCaptured = this->hasCapture();
        const bool result = Base::onProcessMessage(msg);

        if (!wasCaptured && this->hasCapture()) {
          const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
          const gfx::Point mousePos = mouseMsg->position();
          m_dragMousePos = mousePos;
          m_floatingOffset = mouseMsg->position() - this->bounds().origin();
          m_createFloatingUILayer = true;
        }
        return result;
      }

      case ui::kMouseMoveMessage: {
        const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point mousePos = mouseMsg->position();

        if (this->hasCapture() && m_createFloatingUILayer) {
          if (this->manager()->pick(mousePos) != this) {
            m_createFloatingUILayer = false;
            if (!m_floatingUILayer)
              createFloatingUILayer();
          }
        }

        if (m_floatingUILayer) {
          ui::Display* display = this->Base::display();

          display->dirtyRect(m_floatingUILayer->bounds());
          m_floatingUILayer->setPosition(mousePos - m_floatingOffset);
          display->dirtyRect(m_floatingUILayer->bounds());

          bool inside = true;
          if (onCanDropItemsOutside()) {
            inside = getParentBounds().contains(mousePos);
            if (inside) {
              if (this->hasFlags(ui::HIDDEN)) {
                this->disableFlags(ui::HIDDEN);
                layoutParent();
              }
            }
            else {
              if (!this->hasFlags(ui::HIDDEN)) {
                this->enableFlags(ui::HIDDEN);
                layoutParent();
              }
            }
          }

          onReorderWidgets(mousePos, inside);
        }
        break;
      }

      case ui::kMouseUpMessage: {
        const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point mousePos = mouseMsg->position();

        m_wasDragged = (this->hasCapture() && m_floatingUILayer);
        const bool result = Base::onProcessMessage(msg);

        if (!this->hasCapture()) {
          if (m_floatingUILayer) {
            destroyFloatingUILayer();
            ASSERT(!m_createFloatingUILayer);
            onFinalDrop(getParentBounds().contains(mousePos));
          }
          else if (m_createFloatingUILayer)
            m_createFloatingUILayer = false;
        }

        m_wasDragged = false;
        return result;
      }
    }
    return Base::onProcessMessage(msg);
  }

  bool wasDragged() const { return m_wasDragged; }

  bool isDragging() const { return m_isDragging; }

private:
  void createFloatingUILayer()
  {
    ASSERT(!m_floatingUILayer);

    m_isDragging = true;

    gfx::Size sz = floatingUILayerSizeHint();
    sz.w = std::max(1, sz.w);
    sz.h = std::max(1, sz.h);
    os::SurfaceRef surface = os::System::instance()->makeRgbaSurface(sz.w, sz.h);

    {
      os::SurfaceLock lock(surface.get());
      os::Paint paint;
      paint.color(gfx::rgba(0, 0, 0, 0));
      paint.style(os::Paint::Fill);
      surface->drawRect(surface->bounds(), paint);
    }

    ui::Display* display = this->Base::display();
    {
      ui::Graphics g(surface);
      g.setFont(this->font());

      // Draw this widget on the UILayer surface
      ui::PaintEvent ev(this, &g);
      this->onPaint(ev);
    }

    m_floatingUILayer = ui::UILayer::Make();
    m_floatingUILayer->setSurface(surface);
    display->addLayer(m_floatingUILayer);
  }

  void destroyFloatingUILayer()
  {
    this->Base::display()->removeLayer(m_floatingUILayer);
    m_floatingUILayer.reset();
    m_isDragging = false;
  }

  gfx::Size floatingUILayerSizeHint() const
  {
    auto view = ui::View::getView(this->parent());
    if (view)
      return (view->viewportBounds().offset(view->viewScroll()) & this->bounds()).size();
    else
      return this->size();
  }

  gfx::Rect getParentBounds()
  {
    auto view = ui::View::getView(this->parent());
    if (view)
      return view->viewportBounds();
    else
      return this->parent()->bounds();
  }

  void layoutParent()
  {
    this->parent()->layout();
    auto view = ui::View::getView(this->parent());
    if (view)
      return view->updateView();
  }

  virtual bool onCanDropItemsOutside() { return true; }
  virtual void onReorderWidgets(const gfx::Point& mousePos, bool inside) {}
  virtual void onFinalDrop(bool inside) {}

  // True if we should create the floating UILayer after leaving the
  // widget bounds.
  bool m_createFloatingUILayer = false;

  bool m_isDragging = false;

  // True when the mouse button is released (drop operation) and we've
  // dragged the widget to other position. Can be used to avoid
  // triggering the default click operation by derived classes when
  // we've dragged the widget.
  bool m_wasDragged = false;

  // Initial mouse position when we start the dragging process.
  gfx::Point m_dragMousePos;

  // UILayer used to show the floating widget (this layer floats next
  // to the mouse cursor).
  ui::UILayerRef m_floatingUILayer;

  // Relative mouse position between the widget and the overlay.
  gfx::Point m_floatingOffset;
};

} // namespace app

#endif
