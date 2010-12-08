// ASE gui library
// Copyright (C) 2001-2010  David Capello
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

  void setRange(int min, int max);

  void setValue(int value);
  int getValue() const;

  void getSliderThemeInfo(int* min, int* max, int* value);
  
  // Signals
  Signal0<void> Change;
  Signal0<void> ButtonReleased;

protected:
  // Events
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);

  // New events
  virtual void onChange();
  virtual void onButtonReleased();

private:
  void setupSliderCursor();

  int m_min;
  int m_max;
  int m_value;
};

#endif
