// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SLIDER_H_INCLUDED
#define UI_SLIDER_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/widget.h"

namespace ui {

  class SliderDelegate {
  public:
    virtual ~SliderDelegate() { }
    virtual std::string onGetTextFromValue(int value) = 0;
    virtual int onGetValueFromText(const std::string& text) = 0;
  };

  class Slider : public Widget {
  public:
    Slider(int min, int max, int value, SliderDelegate* delegate = nullptr);

    int getMinValue() const { return m_min; }
    int getMaxValue() const { return m_max; }
    int getValue() const    { return m_value; }

    void setRange(int min, int max);
    void setValue(int value);

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly) { m_readOnly = readOnly; }

    void getSliderThemeInfo(int* min, int* max, int* value) const;

    std::string convertValueToText(int value) const;
    int convertTextToValue(const std::string& text) const;

    // Signals
    obs::signal<void()> Change;
    obs::signal<void()> SliderReleased;

  protected:
    // Events
    bool onProcessMessage(Message* msg) override;
    void onPaint(PaintEvent& ev) override;

    // New events
    virtual void onChange();
    virtual void onSliderReleased();

  private:
    void setupSliderCursor();

    int m_min;
    int m_max;
    int m_value;
    bool m_readOnly;
    SliderDelegate* m_delegate;
  };

} // namespace ui

#endif
