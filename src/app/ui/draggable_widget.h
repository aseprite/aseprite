// Aseprite
// Copyright (C) 2018-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DRAGGABLE_WIDGET_H_INCLUDED
#define APP_UI_DRAGGABLE_WIDGET_H_INCLUDED
#pragma once

#include "app/ui/drop_target_widget.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/cursor_type.h"
#include "ui/display.h"
#include "ui/graphics.h"
#include "ui/keys.h"
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

          if (onCanDropWidgetOutside()) {
            ui::set_mouse_cursor(ui::kMoveCursor);
          }
          else if (m_lastTarget) {
            DropEffect effect = DropEffect::None;
            bool over = m_lastTarget->onDragWidgetOver(mousePos, this, effect);
            if (over || !(effectsAllowed() & effect)) {
              ui::set_mouse_cursor(ui::CursorType::kForbiddenCursor);
              return true;
            }

            if (effect == DropEffect::Copy)
              ui::set_mouse_cursor(ui::kArrowPlusCursor);
            else if (effect == DropEffect::Move) {
              ui::set_mouse_cursor(ui::kMoveCursor);
            }
            else {
              ui::set_mouse_cursor(ui::kForbiddenCursor);
            }
          }
          else {
            ui::set_mouse_cursor(ui::kForbiddenCursor);
          }
          return true;
        }
        break;

      case ui::kKeyDownMessage: {
        ui::KeyMessage* keymsg = static_cast<ui::KeyMessage*>(msg);
        if (keymsg->scancode() == ui::kKeyEsc && onCanCancelDrag()) {
          dragEndCleanup();
          auto mousePos = ui::get_mouse_position();
          onDragWidgetEnd(mousePos, getParentBounds().contains(mousePos), true);
        }
        break;
      }

      case ui::kMouseDownMessage: {
        const bool wasCaptured = this->hasCapture();
        const bool result = Base::onProcessMessage(msg);

        if (!wasCaptured && this->hasCapture() && onCanStartDrag()) {
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
          m_createFloatingUILayer = false;
          if (!m_floatingUILayer)
            createFloatingUILayer();
        }

        if (m_floatingUILayer) {
          ui::Display* display = this->Base::display();

          display->dirtyRect(m_floatingUILayer->bounds());
          m_floatingUILayer->setPosition(mousePos - m_floatingOffset);
          display->dirtyRect(m_floatingUILayer->bounds());

          bool inside = true;
          if (onCanDropWidgetOutside()) {
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

          bool consumed = onDragWidget(mousePos, inside);

          auto* pick = this->manager()->pick(mousePos);
          auto* target = dynamic_cast<DropTargetWidget*>(pick);
          if (m_lastTarget != target) {
            if (m_lastTarget) {
              m_lastTarget->onDragWidgetLeave(mousePos);
            }
            if (target) {
              target->onDragWidgetEnter(mousePos);
            }
          }
          m_lastTarget = target;

          // If the drag is consumed, then avoid Base processing.
          if (consumed)
            return true;
        }
        break;
      }

      case ui::kMouseUpMessage: {
        // We must set this to false here too, because when just clicking a widget
        // there is no MouseMoveMessage and no dragging takes place.
        m_createFloatingUILayer = false;
        if (!m_isDragging)
          break;

        const ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point mousePos = mouseMsg->position();

        auto* pick = this->manager()->pick(mousePos);
        auto* target = dynamic_cast<DropTargetWidget*>(pick);
        DropEffect dropEffect = DropEffect::None;
        if (target && Base::hasCapture() && !target->onDragWidgetOver(mousePos, this, dropEffect)) {
          target->onDropWidget(mousePos, this, getParentBounds().contains(mousePos));
        }

        dragEndCleanup();
        bool consumed = onDragWidgetEnd(mousePos, getParentBounds().contains(mousePos), false);
        if (consumed)
          return true;
      }
    }
    return Base::onProcessMessage(msg);
  }

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

  void dragEndCleanup()
  {
    if (Base::hasCapture()) {
      Base::releaseMouse();
    }
    if (m_floatingUILayer) {
      destroyFloatingUILayer();
      ASSERT(!m_createFloatingUILayer);
    }
    ui::set_mouse_cursor(ui::kArrowCursor);
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

  // Returns the effects supported by the dragged widget once it is dropped. Can
  // return a bitwise combination of values.
  virtual DropEffect effectsAllowed() { return DropEffect::None; }
  virtual bool onCanStartDrag() { return true; }
  virtual bool onCanCancelDrag() { return true; }
  virtual bool onCanDropWidgetOutside() { return true; }
  virtual bool onDragWidget(const gfx::Point& mousePos, bool inside) { return true; }
  virtual bool onDragWidgetEnd(const gfx::Point& mousePos, bool inside, bool cancelled)
  {
    return true;
  }

  // True if we should create the floating UILayer after leaving the
  // widget bounds.
  bool m_createFloatingUILayer = false;

  bool m_isDragging = false;

  // Initial mouse position when we start the dragging process.
  gfx::Point m_dragMousePos;

  // UILayer used to show the floating widget (this layer floats next
  // to the mouse cursor).
  ui::UILayerRef m_floatingUILayer;

  // Relative mouse position between the widget and the overlay.
  gfx::Point m_floatingOffset;

  // Last DropTargetWidget that was hovered by the DraggableWidget, used to
  // calculate when a DropTargetWidget is entered or left.
  DropTargetWidget* m_lastTarget = nullptr;
};

} // namespace app

#endif
