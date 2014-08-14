// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SCROLL_BAR_H_INCLUDED
#define UI_SCROLL_BAR_H_INCLUDED
#pragma once

#include "base/override.h"
#include "ui/widget.h"

namespace ui {

  class ScrollBar : public Widget {
  public:
    ScrollBar(int align);

    int getBarWidth() const { return m_barWidth; }
    void setBarWidth(int barWidth) { m_barWidth = barWidth; }

    int getPos() const { return m_pos; }
    void setPos(int pos);

    int getSize() const { return m_size; }
    void setSize(int size);

    // For themes
    void getScrollBarThemeInfo(int* pos, int* len);

  protected:
    // Events
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    void getScrollBarInfo(int* _pos, int* _len, int* _bar_size, int* _viewport_size);

    int m_barWidth;
    int m_pos;
    int m_size;

    static int m_wherepos;
    static int m_whereclick;
  };

} // namespace ui

#endif
