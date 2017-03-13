// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_THEME_H_INCLUDED
#define UI_THEME_H_INCLUDED
#pragma once

#include "gfx/border.h"
#include "gfx/color.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/style.h"

namespace gfx {
  class Region;
}

namespace she {
  class Font;
  class Surface;
}

namespace ui {

  class Cursor;
  class Graphics;
  class PaintEvent;
  class SizeHintEvent;
  class Widget;

  struct PaintWidgetPartInfo {
    gfx::Color bgColor;
    int styleFlags;           // ui::Style::Layer flags
    const std::string* text;
    int mnemonic;

    PaintWidgetPartInfo();
    PaintWidgetPartInfo(const Widget* widget);

    static int getStyleFlagsForWidget(const Widget* widget);
  };

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
    virtual void setDecorativeWidgetBounds(Widget* widget);
    virtual int getScrollbarSize() = 0;
    virtual gfx::Size getEntryCaretSize(Widget* widget) = 0;

    virtual void paintCheckBox(PaintEvent& ev) = 0;
    virtual void paintEntry(PaintEvent& ev) = 0;
    virtual void paintListBox(PaintEvent& ev) = 0;
    virtual void paintListItem(PaintEvent& ev) = 0;
    virtual void paintMenu(PaintEvent& ev) = 0;
    virtual void paintMenuItem(PaintEvent& ev) = 0;
    virtual void paintRadioButton(PaintEvent& ev) = 0;
    virtual void paintSlider(PaintEvent& ev) = 0;
    virtual void paintComboBoxEntry(PaintEvent& ev) = 0;
    virtual void paintTextBox(PaintEvent& ev) = 0;
    virtual void paintViewViewport(PaintEvent& ev) = 0;

    virtual void paintWidgetPart(Graphics* g,
                                 const Style* style,
                                 const gfx::Rect& bounds,
                                 const PaintWidgetPartInfo& info);

    // Default implementation to draw widgets with new ui::Styles
    virtual void paintWidget(Graphics* g,
                             const Widget* widget,
                             const Style* style,
                             const gfx::Rect& bounds);

    virtual void paintScrollBar(Graphics* g,
                                const Widget* widget,
                                const Style* style,
                                const Style* thumbStyle,
                                const gfx::Rect& bounds,
                                const gfx::Rect& thumbBounds);

    virtual void paintTooltip(Graphics* g,
                              const Widget* widget,
                              const Style* style,
                              const Style* arrowStyle,
                              const gfx::Rect& bounds,
                              const int arrowAlign,
                              const gfx::Rect& target);

    virtual gfx::Size calcSizeHint(const Widget* widget,
                                   const Style* style);
    virtual gfx::Border calcBorder(const Widget* widget,
                                   const Style* style);
    virtual void calcSlices(const Widget* widget,
                            const Style* style,
                            gfx::Size& topLeft,
                            gfx::Size& center,
                            gfx::Size& bottomRight);
    virtual gfx::Color calcBgColor(const Widget* widget,
                                   const Style* style);

    static void drawSlices(Graphics* g,
                           she::Surface* sheet,
                           const gfx::Rect& rc,
                           const gfx::Rect& sprite,
                           const gfx::Rect& slices,
                           const bool drawCenter = true);

    static  void drawTextBox(Graphics* g, Widget* textbox,
                             int* w, int* h, gfx::Color bg, gfx::Color fg);

  protected:
    virtual void onRegenerate() = 0;

  private:
    void paintLayer(Graphics* g,
                    const Style::Layer& layer,
                    const std::string& text,
                    const int mnemonic,
                    gfx::Rect& rc,
                    gfx::Color& bgColor);
    void measureLayer(const Widget* widget,
                      const Style::Layer& layer,
                      gfx::Border& borderHint,
                      gfx::Size& textHint, int& textAlign,
                      gfx::Size& iconHint, int& iconAlign);
    void calcWidgetMetrics(const Widget* widget,
                           const Style* style,
                           gfx::Size& sizeHint,
                           gfx::Border& borderHint);

    int m_guiscale;
  };

  void set_theme(Theme* theme);
  Theme* get_theme();

  // This value is a factor to multiply every screen size/coordinate.
  // Every icon/graphics/font should be scaled to this factor.
  inline int guiscale() {
    return (get_theme() ? get_theme()->guiscale(): 1);
  }

} // namespace ui

#endif
