// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_THEME_H_INCLUDED
#define UI_THEME_H_INCLUDED
#pragma once

#include "gfx/size.h"
#include "ui/base.h"
#include "ui/cursor_type.h"

namespace gfx {
  class Region;
}

namespace she {
  class Font;
}

namespace ui {

  class Cursor;
  class PaintEvent;
  class Widget;

  class Theme {
  public:
    Theme();
    virtual ~Theme();

    void regenerate();

    int guiscale() const { return m_guiscale; }
    void setScale(int value) { m_guiscale = value; }

    virtual she::Font* getDefaultFont() const = 0;
    virtual she::Font* getWidgetFont(const Widget* widget) const = 0;

    virtual Cursor* getCursor(CursorType type) = 0;
    virtual void initWidget(Widget* widget) = 0;
    virtual void getWindowMask(Widget* widget, gfx::Region& region) = 0;
    virtual void setDecorativeWidgetBounds(Widget* widget) = 0;
    virtual int getScrollbarSize() = 0;
    virtual gfx::Size getEntryCaretSize(Widget* widget) = 0;

    virtual void paintDesktop(PaintEvent& ev) = 0;
    virtual void paintBox(PaintEvent& ev) = 0;
    virtual void paintButton(PaintEvent& ev) = 0;
    virtual void paintCheckBox(PaintEvent& ev) = 0;
    virtual void paintEntry(PaintEvent& ev) = 0;
    virtual void paintGrid(PaintEvent& ev) = 0;
    virtual void paintLabel(PaintEvent& ev) = 0;
    virtual void paintLinkLabel(PaintEvent& ev) = 0;
    virtual void paintListBox(PaintEvent& ev) = 0;
    virtual void paintListItem(PaintEvent& ev) = 0;
    virtual void paintMenu(PaintEvent& ev) = 0;
    virtual void paintMenuItem(PaintEvent& ev) = 0;
    virtual void paintSplitter(PaintEvent& ev) = 0;
    virtual void paintRadioButton(PaintEvent& ev) = 0;
    virtual void paintSeparator(PaintEvent& ev) = 0;
    virtual void paintSlider(PaintEvent& ev) = 0;
    virtual void paintComboBoxEntry(PaintEvent& ev) = 0;
    virtual void paintComboBoxButton(PaintEvent& ev) = 0;
    virtual void paintTextBox(PaintEvent& ev) = 0;
    virtual void paintView(PaintEvent& ev) = 0;
    virtual void paintViewScrollbar(PaintEvent& ev) = 0;
    virtual void paintViewViewport(PaintEvent& ev) = 0;
    virtual void paintWindow(PaintEvent& ev) = 0;
    virtual void paintPopupWindow(PaintEvent& ev) = 0;
    virtual void paintTooltip(PaintEvent& ev) = 0;

  protected:
    virtual void onRegenerate() = 0;

  private:
    int m_guiscale;
  };

  namespace CurrentTheme
  {
    void set(Theme* theme);
    Theme* get();
  }

  // This value is a factor to multiply every screen size/coordinate.
  // Every icon/graphics/font should be scaled to this factor.
  inline int guiscale()
  {
    return CurrentTheme::get() ? CurrentTheme::get()->guiscale(): 1;
  }

} // namespace ui

#endif
