// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_INT_ENTRY_H_INCLUDED
#define UI_INT_ENTRY_H_INCLUDED

#include "ui/entry.h"

namespace ui {

  class PopupWindow;
  class Slider;

  class IntEntry : public Entry
  {
  public:
    IntEntry(int min, int max);
    ~IntEntry();

    int getValue() const;
    void setValue(int value);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onEntryChange() OVERRIDE;

    // New events
    virtual void onValueChange();

  private:
    void openPopup();
    void closePopup();
    void onChangeSlider();

    int m_min;
    int m_max;
    PopupWindow* m_popupWindow;
    Slider* m_slider;
  };

} // namespace ui

#endif
