// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SLIDER2_H_INCLUDED
#define APP_UI_SLIDER2_H_INCLUDED
#pragma once

#include "ui/box.h"
#include "ui/entry.h"
#include "ui/slider.h"

namespace app {

// TODO merge this with IntEntry?
// It looks like there are 3 different modes to input values:
//
//   1. An entry field (Entry)
//   2. An entry field with a popup slider (IntEntry)
//   3. A slider + entry field (Slider2, ColorSliders+ColorEntry)
//
// We might merge all these modes in just on responsive widget, but
// I'm not sure.
class Slider2 : public ui::HBox {
public:
  Slider2(int min, int max, int value);

  int getValue() const { return m_slider.getValue(); }

  // Signals
  obs::signal<void()> Change;

private:
  virtual void onChange();
  void onSliderChange();
  void onEntryChange();

  class Slider2Entry : public ui::Entry {
  public:
    Slider2Entry(ui::Slider* slider);

  private:
    int minValue() const { return m_slider->getMinValue(); }
    int maxValue() const { return m_slider->getMaxValue(); }
    bool onProcessMessage(ui::Message* msg) override;
    ui::Slider* m_slider;
    // TODO remove this calling setFocus() in
    //      Widget::onProcessMessage() instead of
    //      Manager::handleWindowZOrder()
    bool m_recentFocus;
  };
  ui::Slider m_slider;
  Slider2Entry m_entry;
};

} // namespace app

#endif
