// Aseprite UI Library
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_TOOLTIPS_H_INCLUDED
#define UI_TOOLTIPS_H_INCLUDED
#pragma once

#include "ui/base.h"
#include "ui/popup_window.h"
#include "ui/timer.h"
#include "ui/window.h"

#include <map>
#include <memory>

namespace ui {

  class TextBox;
  class TipWindow;

  class TooltipManager : public Widget {
  public:
    TooltipManager();
    ~TooltipManager();

    void addTooltipFor(Widget* widget, const std::string& text, int arrowAlign = 0);
    void removeTooltipFor(Widget* widget);

  protected:
    bool onProcessMessage(Message* msg) override;
    void onInitTheme(InitThemeEvent& ev) override;

  private:
    void onTick();

    struct TipInfo {
      std::string text;
      int arrowAlign;

      TipInfo() { }
      TipInfo(const std::string& text, int arrowAlign)
        : text(text), arrowAlign(arrowAlign) {
      }
    };

    typedef std::map<Widget*, TipInfo> Tips;
    Tips m_tips;                      // All tips.
    std::unique_ptr<TipWindow> m_tipWindow; // Frame to show tooltips.
    std::unique_ptr<Timer> m_timer;         // Timer to control the tooltip delay.
    struct {
      Widget* widget;
      TipInfo tipInfo;
    } m_target;
  };

  class TipWindow : public PopupWindow {
  public:
    TipWindow(const std::string& text = "");

    Style* arrowStyle() { return m_arrowStyle; }
    void setArrowStyle(Style* style) { m_arrowStyle = style; }

    int arrowAlign() const { return m_arrowAlign; }
    const gfx::Rect& target() const { return m_target; }

    void setCloseOnKeyDown(bool state);

    // Returns false there is no enough screen space to show the
    // window.
    bool pointAt(int arrowAlign, const gfx::Rect& target);

    TextBox* textBox() const { return m_textBox; }

  protected:
    bool onProcessMessage(Message* msg) override;
    void onPaint(PaintEvent& ev) override;
    void onBuildTitleLabel() override;

  private:
    Style* m_arrowStyle;
    int m_arrowAlign;
    gfx::Rect m_target;
    bool m_closeOnKeyDown;
    TextBox* m_textBox;
  };

} // namespace ui

#endif
