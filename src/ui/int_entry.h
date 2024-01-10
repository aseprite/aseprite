// Aseprite UI Library
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_INT_ENTRY_H_INCLUDED
#define UI_INT_ENTRY_H_INCLUDED
#pragma once

#include "ui/entry.h"
#include "ui/slider.h"

#include <memory>

namespace ui {

  class CloseEvent;
  class PopupWindow;

  class IntEntry : public Entry {
  public:
    IntEntry(int min, int max, SliderDelegate* sliderDelegate = nullptr);
    virtual ~IntEntry();

    virtual int getValue() const;
    virtual void setValue(int value);

  protected:
    bool onProcessMessage(Message* msg) override;
    void onInitTheme(InitThemeEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    void onChange() override;
    virtual void onChangeSlider();

    // New events
    virtual void onValueChange();

    int m_min;
    int m_max;
    std::unique_ptr<PopupWindow> m_popupWindow;
    bool m_changeFromSlider;
    std::unique_ptr<Slider> m_slider;

  private:
    void openPopup();
    void closePopup();
    void onPopupClose(CloseEvent& ev);
    void removeSlider();

  };

} // namespace ui

#endif
