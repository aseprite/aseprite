// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_SLIDER_H_INCLUDED
#define UI_SLIDER_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/widget.h"

namespace ui {

  class Slider : public Widget
  {
  public:
    Slider(int min, int max, int value);

    int getMinValue() const { return m_min; }
    int getMaxValue() const { return m_max; }
    int getValue() const    { return m_value; }

    void setRange(int min, int max);
    void setValue(int value);

    void getSliderThemeInfo(int* min, int* max, int* value);

    // Signals
    Signal0<void> Change;
    Signal0<void> SliderReleased;

  protected:
    // Events
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

    // New events
    virtual void onChange();
    virtual void onSliderReleased();

  private:
    void setupSliderCursor();

    int m_min;
    int m_max;
    int m_value;
  };

} // namespace ui

#endif
