// Aseprite UI Library
// Copyright (C) 2020-2024  Igara Studio S.A.
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
#include "text/font_mgr.h"
#include "text/fwd.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/scale.h"
#include "ui/style.h"

namespace gfx {
  class Region;
}

namespace os {
  class Font;
  class Surface;
}

namespace ui {

  class Cursor;
  class Graphics;
  class PaintEvent;
  class SizeHintEvent;
  class Widget;
  class Theme;

  void set_theme(Theme* theme, const int uiscale);
  Theme* get_theme();

  inline int CALC_FOR_CENTER(int p, int s1, int s2) {
    return (p/guiscale() + (s1/guiscale())/2 - (s2/guiscale())/2)*guiscale();
  }

  struct PaintWidgetPartInfo {
    gfx::Color bgColor;
    int styleFlags;           // ui::Style::Layer flags
    const std::string* text;
    text::TextBlobRef textBlob;
    int mnemonic;
    os::Surface* icon;

    PaintWidgetPartInfo();
    PaintWidgetPartInfo(const Widget* widget);

    static int getStyleFlagsForWidget(const Widget* widget);
  };

  class Theme {
    friend void set_theme(Theme* theme, const int uiscale);
  public:
    static constexpr int kDefaultFontHeight = 16;

    Theme();
    virtual ~Theme();

    virtual text::FontMgrRef fontMgr() const { return m_fontMgr; }
    virtual text::FontRef getDefaultFont() const;
    virtual text::FontRef getWidgetFont(const Widget* widget) const {
      return getDefaultFont();
    }

    virtual ui::Cursor* getStandardCursor(CursorType type) { return nullptr; }
    virtual void initWidget(Widget* widget);
    virtual void getWindowMask(Widget* widget, gfx::Region& region) { }
    virtual void setDecorativeWidgetBounds(Widget* widget);
    virtual int getScrollbarSize() { return kDefaultFontHeight; }
    virtual gfx::Size getEntryCaretSize(Widget* widget) {
      return gfx::Size(kDefaultFontHeight, 1);
    }

    virtual void paintEntry(PaintEvent& ev) { }
    virtual void paintListBox(PaintEvent& ev);
    virtual void paintMenu(PaintEvent& ev) { }
    virtual void paintMenuItem(PaintEvent& ev) { }
    virtual void paintSlider(PaintEvent& ev) { }
    virtual void paintComboBoxEntry(PaintEvent& ev) { }
    virtual void paintTextBox(PaintEvent& ev) { }
    virtual void paintViewViewport(PaintEvent& ev);

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

    void paintTextBoxWithStyle(Graphics* g,
                               const Widget* widget);

    virtual gfx::Size calcSizeHint(const Widget* widget,
                                   const Style* style);
    virtual gfx::Border calcBorder(const Widget* widget,
                                   const Style* style);
    virtual void calcSlices(const Widget* widget,
                            const Style* style,
                            gfx::Size& topLeft,
                            gfx::Size& center,
                            gfx::Size& bottomRight);
    virtual void calcTextInfo(const Widget* widget,
                              const Style* style,
                              const gfx::Rect& bounds,
                              gfx::Rect& textBounds, int& textAlign);
    virtual gfx::Color calcBgColor(const Widget* widget,
                                   const Style* style);
    virtual gfx::Size calcMinSize(const Widget* widget,
                                  const Style* style);
    virtual gfx::Size calcMaxSize(const Widget* widget,
                                  const Style* style);

    static void drawSlices(Graphics* g,
                           os::Surface* sheet,
                           const gfx::Rect& rc,
                           const gfx::Rect& sprite,
                           const gfx::Rect& slices,
                           const gfx::Color color,
                           const bool drawCenter = true);

    static void drawTextBox(Graphics* g, const Widget* textbox,
                            int* w, int* h, gfx::Color bg, gfx::Color fg);

    static ui::Style* EmptyStyle() { return &m_emptyStyle; }

  protected:
    virtual void onRegenerateTheme() { }

    text::FontMgrRef m_fontMgr;

  private:
    void regenerateTheme();
    void paintLayer(Graphics* g,
                    const Style* style,
                    const Style::Layer& layer,
                    const std::string& text,
                    const text::TextBlobRef& textBlob,
                    const int mnemonic,
                    os::Surface* icon,
                    gfx::Rect& rc,
                    gfx::Color& bgColor);
    void measureLayer(const Widget* widget,
                      const Style* style,
                      const Style::Layer& layer,
                      gfx::Border& borderHint,
                      gfx::Rect& textHint, int& textAlign,
                      gfx::Size& iconHint, int& iconAlign);
    void calcWidgetMetrics(const Widget* widget,
                           const Style* style,
                           gfx::Size& sizeHint,
                           gfx::Border& borderHint,
                           gfx::Rect& textHint, int& textAlign);

    static ui::Style m_emptyStyle;
    static ui::Style m_simpleStyle;
  };

} // namespace ui

#endif
