// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_POPUP_WINDOW_H_INCLUDED
#define UI_POPUP_WINDOW_H_INCLUDED
#pragma once

#include "ui/window.h"

namespace ui {

  class PopupWindow : public Window {
  public:
    enum class ClickBehavior {
      DoNothingOnClick,
      CloseOnClickInOtherWindow,
      CloseOnClickOutsideHotRegion
    };

    enum class EnterBehavior {
      DoNothingOnEnter,
      CloseOnEnter,
    };

    PopupWindow(const std::string& text,
                ClickBehavior clickBehavior,
                EnterBehavior enterBehavior = EnterBehavior::CloseOnEnter);
    ~PopupWindow();

    void setHotRegion(const gfx::Region& region);

    void makeFloating();
    void makeFixed();

  protected:
    bool onProcessMessage(Message* msg) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onPaint(PaintEvent& ev) override;
    void onInitTheme(InitThemeEvent& ev) override;
    void onHitTest(HitTestEvent& ev) override;

  private:
    void startFilteringMessages();
    void stopFilteringMessages();

    ClickBehavior m_clickBehavior;
    EnterBehavior m_enterBehavior;
    gfx::Region m_hotRegion;
    bool m_filtering;
  };

} // namespace ui

#endif
