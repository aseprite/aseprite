// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SCROLL_BAR_H_INCLUDED
#define UI_SCROLL_BAR_H_INCLUDED
#pragma once

#include "ui/widget.h"

namespace ui {

class ScrollableViewDelegate {
public:
  virtual ~ScrollableViewDelegate() {}
  virtual gfx::Size visibleSize() const = 0;
  virtual gfx::Point viewScroll() const = 0;
  virtual void setViewScroll(const gfx::Point& pt) = 0;
};

class ScrollBar : public Widget {
public:
  ScrollBar(int align, ScrollableViewDelegate* delegate);

  Style* thumbStyle() { return m_thumbStyle; }
  void setThumbStyle(Style* style) { m_thumbStyle = style; }

  int getBarWidth() const { return m_barWidth; }
  void setBarWidth(int barWidth) { m_barWidth = barWidth; }

  int getPos() const { return m_pos; }
  void setPos(int pos);

  int size() const { return m_size; }
  void setSize(int size);

  // For themes
  void getScrollBarThemeInfo(int* pos, int* len);

protected:
  // Events
  bool onProcessMessage(Message* msg) override;
  void onInitTheme(InitThemeEvent& ev) override;
  void onPaint(PaintEvent& ev) override;

private:
  void getScrollBarInfo(int* _pos, int* _len, int* _bar_size, int* _viewport_size);

  ScrollableViewDelegate* m_delegate;
  Style* m_thumbStyle;
  int m_barWidth;
  int m_pos;
  int m_size;

  static int m_wherepos;
  static int m_whereclick;
};

} // namespace ui

#endif
