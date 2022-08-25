// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DOCK_H_INCLUDED
#define APP_UI_DOCK_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/widget.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace app {

class DockTabs : public ui::Widget {
public:
protected:
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;
};

class Dock : public ui::Widget {
public:
  static constexpr const int kSides = 5;

  Dock();

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
  gfx::Size getUserDefinedSizeAtSide(int side) const;

  obs::signal<void()> Resize;

protected:
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;

private:
  void setSide(const int i, ui::Widget* newWidget);
  int calcAlign(const int i);
  void updateDockVisibility();
  void forEachSide(gfx::Rect bounds,
                   std::function<void(ui::Widget* widget,
                                      const gfx::Rect& widgetBounds,
                                      const gfx::Rect& separator,
                                      const int index)> f);

  bool hasVisibleSide(const int i) const { return (m_sides[i] && m_sides[i]->isVisible()); }

  std::array<Widget*, kSides> m_sides;
  std::array<int, kSides> m_aligns;
  std::array<gfx::Size, kSides> m_sizes;
  bool m_autoDelete = false;

  // Used to drag-and-drop sides.
  ui::Widget* m_capturedWidget = nullptr;
  int m_capturedSide;
  gfx::Size m_startSize;
  gfx::Point m_startPos;
};

} // namespace app

#endif
