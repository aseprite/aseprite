// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_TOOLTIPS_H_INCLUDED
#define UI_TOOLTIPS_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/base.h"
#include "ui/window.h"

#include <map>

namespace ui {

  class TipWindow;

  class TooltipManager : public Widget
  {
  public:
    TooltipManager();
    ~TooltipManager();

    void addTooltipFor(Widget* widget, const base::string& text, int arrowAlign = 0);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;

  private:
    void onTick();

    struct TipInfo {
      std::string text;
      int arrowAlign;

      TipInfo() { }
      TipInfo(const base::string& text, int arrowAlign)
        : text(text), arrowAlign(arrowAlign) {
      }
    };

    typedef std::map<Widget*, TipInfo> Tips;
    Tips m_tips;                      // All tips.
    base::UniquePtr<TipWindow> m_tipWindow; // Frame to show tooltips.
    base::UniquePtr<Timer> m_timer;         // Timer to control the tooltip delay.
    struct {
      Widget* widget;
      TipInfo tipInfo;
    } m_target;
  };

  class TipWindow : public Window
  {
  public:
    TipWindow(const char *text, bool close_on_buttonpressed = false);
    ~TipWindow();

    void setHotRegion(const gfx::Region& region);

    int getArrowAlign() const;
    void setArrowAlign(int arrowAlign);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onInitTheme(InitThemeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    bool m_close_on_buttonpressed;
    gfx::Region m_hotRegion;
    bool m_filtering;
    int m_arrowAlign;
  };

} // namespace ui

#endif
