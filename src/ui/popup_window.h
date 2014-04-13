// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_POPUP_WINDOW_H_INCLUDED
#define UI_POPUP_WINDOW_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/window.h"

namespace ui {

  class PopupWindow : public Window
  {
  public:
    enum ClickBehavior {
      kDoNothingOnClick,
      kCloseOnClickInOtherWindow,
      kCloseOnClickOutsideHotRegion
    };

    PopupWindow(const base::string& text, ClickBehavior clickBehavior);
    ~PopupWindow();

    void setHotRegion(const gfx::Region& region);

    void makeFloating();
    void makeFixed();

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;
    void onInitTheme(InitThemeEvent& ev) OVERRIDE;

  private:
    void startFilteringMessages();
    void stopFilteringMessages();

    ClickBehavior m_clickBehavior;
    gfx::Region m_hotRegion;
    bool m_filtering;
  };

} // namespace ui

#endif
