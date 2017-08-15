// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
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

    PopupWindow(const std::string& text = "",
                const ClickBehavior clickBehavior = ClickBehavior::CloseOnClickOutsideHotRegion,
                const EnterBehavior enterBehavior = EnterBehavior::CloseOnEnter,
                const bool withCloseButton = false);
    ~PopupWindow();

    // Sets the hot region. This region indicates the area where the
    // mouse can be located and the window will be kept open.
    void setHotRegion(const gfx::Region& region);
    void setClickBehavior(ClickBehavior behavior);
    void setEnterBehavior(EnterBehavior behavior);

    void makeFloating();
    void makeFixed();

  protected:
    bool onProcessMessage(Message* msg) override;
    void onHitTest(HitTestEvent& ev) override;

    virtual void onMakeFloating();
    virtual void onMakeFixed();

  private:
    void startFilteringMessages();
    void stopFilteringMessages();

    ClickBehavior m_clickBehavior;
    EnterBehavior m_enterBehavior;
    gfx::Region m_hotRegion;
    bool m_filtering;
    bool m_fixed;
  };

  class TransparentPopupWindow : public PopupWindow {
  public:
    TransparentPopupWindow(ClickBehavior clickBehavior);
  protected:
    void onInitTheme(InitThemeEvent& ev) override;
  };

} // namespace ui

#endif
