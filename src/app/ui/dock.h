// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DOCK_H_INCLUDED
#define APP_UI_DOCK_H_INCLUDED
#pragma once

#include "app/ui/dockable.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/widget.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace app {

class Dockable;

class Dock : public ui::Widget {
public:
  static constexpr const int kSides = 5;

  Dock();

  bool isCustomizing() const { return m_customizing; }
  void setCustomizing(bool enable, bool doLayout = true);

  void resetDocks();

  // side = ui::LEFT, or ui::RIGHT, etc.
  void dock(int side, ui::Widget* widget, const gfx::Size& prefSize = gfx::Size());

  void dockRelativeTo(ui::Widget* relative,
                      int side,
                      ui::Widget* widget,
                      const gfx::Size& prefSize = gfx::Size());

  void undock(ui::Widget* widget);

  Dock* subdock(int side);

  Dock* top() { return subdock(ui::TOP); }
  Dock* bottom() { return subdock(ui::BOTTOM); }
  Dock* left() { return subdock(ui::LEFT); }
  Dock* right() { return subdock(ui::RIGHT); }
  Dock* center() { return subdock(ui::CENTER); }

  // Functions useful to query/save the dock layout.
  int whichSideChildIsDocked(const ui::Widget* widget) const;
  const gfx::Size getUserDefinedSizeAtSide(int side) const;

  obs::signal<void()> Resize;
  obs::signal<void()> UserResizedDock;

protected:
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onUserResizedDock();

private:
  struct Hit {
    ui::Widget* widget = nullptr;
    Dockable* dockable = nullptr;
    Dock* dock = nullptr;
    int sideIndex = -1;
    int targetSide = 0;

    gfx::Rect bounds() const;

    bool operator==(const Hit& other) const
    {
      return (widget == other.widget && dockable == other.dockable && dock == other.dock &&
              sideIndex == other.sideIndex && targetSide == other.targetSide);
    }
    bool operator!=(const Hit& other) const { return !operator==(other); }
  };

  class DraggingFeedback final {
  public:
    DraggingFeedback(ui::Display* display, const Hit& hit, const gfx::Point& mousePosition);
    ~DraggingFeedback();

    const Hit& hit() const { return m_hit; }

    void setHit(const Hit& hit);
    void setFloatingPosition(const gfx::Point& position) const;
    void paint(ui::Graphics* g, gfx::Rect bounds) const;

  private:
    Hit m_hit;
    ui::Display* m_display;
    ui::Widget* m_dragWidget;
    gfx::Point m_mouseOffset;
    ui::UILayerRef m_floatingUILayer;
    ui::UILayerRef m_dropUILayer;
  };

  void setSide(int i, ui::Widget* newWidget);
  int calcAlign(int i);
  void updateDockVisibility();
  void forEachSide(gfx::Rect bounds,
                   std::function<void(ui::Widget* widget,
                                      const gfx::Rect& widgetBounds,
                                      const gfx::Rect& separator,
                                      const int index)> f);

  bool hasVisibleSide(const int i) const { return (m_sides[i] && m_sides[i]->isVisible()); }
  void redockWidget(app::Dock* newDock, ui::Widget* dockableWidget, int side);
  Dock* rootDock();

  Hit calcHit(const gfx::Point& pos);

  std::array<Widget*, kSides> m_sides;
  std::array<int, kSides> m_aligns;
  std::array<gfx::Size, kSides> m_sizes;
  bool m_autoDelete = false;

  // Use to drag-and-drop stuff (splitters and dockable widgets)
  Hit m_hit;

  // Used to resize sizes splitters.
  gfx::Size m_startSize;
  gfx::Point m_startPos;

  // True when we paint/can drag-and-drop dockable widgets from handles.
  bool m_customizing = false;

  // True when we're dragging a widget to attempt to dock it somewhere else.
  bool m_dragging = false;
  std::unique_ptr<DraggingFeedback> m_draggingFeedback;
};

} // namespace app

#endif
