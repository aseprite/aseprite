// Aseprite
// Copyright (C) 2021-2024  Igara Studio S.A.
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

  class DropzonePlaceholder final : public Widget,
                                    public Dockable {
  public:
    explicit DropzonePlaceholder(Widget* dragWidget, const gfx::Point& mousePosition);
    ~DropzonePlaceholder() override;
    void setGhostPosition(const gfx::Point& position) const;

  private:
    void onPaint(ui::PaintEvent& ev) override;
    int dockHandleSide() const override { return 0; }

    gfx::Point m_mouseOffset;
    ui::UILayerRef m_floatingUILayer;
    bool m_hidePreview;
  };

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
  void setSide(int i, ui::Widget* newWidget);
  int calcAlign(int i);
  void updateDockVisibility();
  void forEachSide(gfx::Rect bounds,
                   std::function<void(ui::Widget* widget,
                                      const gfx::Rect& widgetBounds,
                                      const gfx::Rect& separator,
                                      const int index)> f);

  bool hasVisibleSide(const int i) const { return (m_sides[i] && m_sides[i]->isVisible()); }
  void redockWidget(app::Dock* widgetDock, ui::Widget* dockableWidget, const int side);

  struct Hit {
    ui::Widget* widget = nullptr;
    Dockable* dockable = nullptr;
    int sideIndex = -1;
    int targetSide = -1;
  };

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
  std::unique_ptr<DropzonePlaceholder> m_dropzonePlaceholder;
};

} // namespace app

#endif
