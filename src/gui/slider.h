// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_SLIDER_H_INCLUDED
#define GUI_SLIDER_H_INCLUDED

#include "base/signal.h"
#include "gui/widget.h"

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
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);
  void onPaint(PaintEvent& ev);

  // New events
  virtual void onChange();
  virtual void onSliderReleased();

private:
  void setupSliderCursor();

  int m_min;
  int m_max;
  int m_value;
};

#endif
