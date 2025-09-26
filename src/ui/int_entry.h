// Aseprite UI Library
// Copyright (C) 2022-2025  Igara Studio S.A.
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

  // If useSlider is false, then it won't show the slider popup to change its
  // value.
  void useSlider(bool useSlider) { m_useSlider = useSlider; }

protected:
  bool onProcessMessage(Message* msg) override;
  void onInitTheme(InitThemeEvent& ev) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onChange() override;
  virtual void onChangeSlider();

  // New events
  virtual void onValueChange();
  virtual bool onAcceptUnicodeChar(int unicodeChar);

  int m_min;
  int m_max;
  std::unique_ptr<PopupWindow> m_popupWindow;
  bool m_changeFromSlider;
  // If true a slider can be used to modify the value.
  bool m_useSlider = true;
  std::unique_ptr<Slider> m_slider;

private:
  void openPopup();
  void closePopup();
  void onPopupClose(CloseEvent& ev);
  void removeSlider();
};

} // namespace ui

#endif
