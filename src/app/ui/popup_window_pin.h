// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_POPUP_FRAME_PIN_H_INCLUDED
#define APP_UI_POPUP_FRAME_PIN_H_INCLUDED
#pragma once

#include "ui/button.h"
#include "ui/popup_window.h"

namespace app {

  class PopupWindowPin : public ui::PopupWindow {
  public:
    PopupWindowPin(const std::string& text, ClickBehavior clickBehavior);

    bool showPin(bool state);
    bool isPinned() const { return m_pin.isSelected(); }
    void setPinned(bool pinned);

  protected:
    virtual bool onProcessMessage(ui::Message* msg) override;
    virtual void onHitTest(ui::HitTestEvent& ev) override;

    // The pin. Your derived class must add this pin in some place of
    // the frame as a children, and you must to remove the pin from the
    // parent in your class's dtor.
    ui::CheckBox* getPin() { return &m_pin; }

  private:
    void onPinClick(ui::Event& ev);

    ui::CheckBox m_pin;
  };

} // namespace app

#endif
