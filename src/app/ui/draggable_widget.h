// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DRAGGABLE_WIDGET_H_INCLUDED
#define APP_UI_DRAGGABLE_WIDGET_H_INCLUDED
#pragma once

#include "os/surface.h"
#include "os/system.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/overlay.h"
#include "ui/overlay_manager.h"
#include "ui/paint_event.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

template<typename Base>
class DraggableWidget : public Base {
public:
  template<typename...Args>
  DraggableWidget(Args...args) : Base(args...) { }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case ui::kSetCursorMessage:
        if (m_floatingOverlay) {
          const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
          const gfx::Point mousePos = mouseMsg->position();
          if (onCanDropItemsOutside() &&
              !getParentBounds().contains(mousePos)) {
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
          m_createFloatingOverlay = true;
        }
        return result;
      }

      case ui::kMouseMoveMessage: {
        const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point mousePos = mouseMsg->position();

        if (this->hasCapture() && m_createFloatingOverlay) {
          if (this->manager()->pick(mousePos) != this) {
            m_createFloatingOverlay = false;
            if (!m_floatingOverlay)
              createFloatingOverlay();
          }
        }

        if (m_floatingOverlay) {
          m_floatingOverlay->moveOverlay(mousePos - m_floatingOffset);

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

        m_wasDragged = (this->hasCapture() && m_floatingOverlay);
        const bool result = Base::onProcessMessage(msg);

        if (!this->hasCapture()) {
          if (m_floatingOverlay) {
            destroyFloatingOverlay();
            ASSERT(!m_createFloatingOverlay);
            onFinalDrop(getParentBounds().contains(mousePos));
          }
          else if (m_createFloatingOverlay)
            m_createFloatingOverlay = false;
        }

        m_wasDragged = false;
        return result;
      }

    }
    return Base::onProcessMessage(msg);
  }

  bool wasDragged() const {
    return m_wasDragged;
  }

  bool isDragging() const {
    return m_isDragging;
  }

private:

  void createFloatingOverlay() {
    ASSERT(!m_floatingOverlay);

    m_isDragging = true;

    gfx::Size sz = getFloatingOverlaySize();
    sz.w = std::max(1, sz.w);
    sz.h = std::max(1, sz.h);
    os::Surface* surface = os::instance()->createRgbaSurface(sz.w, sz.h);

    {
      os::SurfaceLock lock(surface);
      surface->fillRect(gfx::rgba(0, 0, 0, 0),
                        gfx::Rect(0, 0, surface->width(), surface->height()));
    }
    {
      ui::Graphics g(surface, 0, 0);
      g.setFont(this->font());
      drawFloatingOverlay(g);
    }

    m_floatingOverlay.reset(new ui::Overlay(surface, gfx::Point(),
                                            ui::Overlay::MouseZOrder-1));
    ui::OverlayManager::instance()->addOverlay(m_floatingOverlay.get());
  }

  void destroyFloatingOverlay() {
    ui::OverlayManager::instance()->removeOverlay(m_floatingOverlay.get());
    m_floatingOverlay.reset();
    m_isDragging = false;
  }

  gfx::Size getFloatingOverlaySize() {
    auto view = ui::View::getView(this->parent());
    if (view)
      return (view->viewportBounds().offset(view->viewScroll()) & this->bounds()).size();
    else
      return this->size();
  }

  gfx::Rect getParentBounds() {
    auto view = ui::View::getView(this->parent());
    if (view)
      return view->viewportBounds();
    else
      return this->parent()->bounds();
  }

  void layoutParent() {
    this->parent()->layout();
    auto view = ui::View::getView(this->parent());
    if (view)
      return view->updateView();
  }

  void drawFloatingOverlay(ui::Graphics& g) {
    ui::PaintEvent ev(this, &g);
    this->onPaint(ev);
  }

  virtual bool onCanDropItemsOutside() { return true; }
  virtual void onReorderWidgets(const gfx::Point& mousePos, bool inside) { }
  virtual void onFinalDrop(bool inside) { }

  // True if we should create the floating overlay after leaving the
  // widget bounds.
  bool m_createFloatingOverlay = false;

  bool m_isDragging = false;

  // True when the mouse button is released (drop operation) and we've
  // dragged the widget to other position. Can be used to avoid
  // triggering the default click operation by derived classes when
  // we've dragged the widget.
  bool m_wasDragged = false;

  // Initial mouse position when we start the dragging process.
  gfx::Point m_dragMousePos;

  // Overlay used to show the floating widget (this overlay floats
  // next to the mouse cursor).
  std::unique_ptr<ui::Overlay> m_floatingOverlay;

  // Relative mouse position between the widget and the overlay.
  gfx::Point m_floatingOffset;
};

} // namespace app

#endif
