// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_INT_ENTRY_H_INCLUDED
#define UI_INT_ENTRY_H_INCLUDED
#pragma once

#include "ui/entry.h"

namespace ui {

  class CloseEvent;
  class PopupWindow;
  class Slider;

  class IntEntry : public Entry {
  public:
    IntEntry(int min, int max);
    ~IntEntry();

    int getValue() const;
    void setValue(int value);

  protected:
    bool onProcessMessage(Message* msg) override;
    void onEntryChange() override;

    // New events
    virtual void onValueChange();

  private:
    void openPopup();
    void closePopup();
    void onChangeSlider();
    void onPopupClose(CloseEvent& ev);

    int m_min;
    int m_max;
    PopupWindow* m_popupWindow;
    Slider* m_slider;
    bool m_changeFromSlider;
  };

} // namespace ui

#endif
