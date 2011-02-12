// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_TOOLTIPS_H_INCLUDED
#define GUI_TOOLTIPS_H_INCLUDED

#include "gui/base.h"
#include "gui/frame.h"

class TipWindow : public Frame
{
  bool m_close_on_buttonpressed;
  JRegion m_hot_region;
  bool m_filtering;

public:
  TipWindow(const char *text, bool close_on_buttonpressed = false);
  ~TipWindow();

  void set_hotregion(JRegion region);

protected:
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);
  void onPaint(PaintEvent& ev);
};

void jwidget_add_tooltip_text(Widget* widget, const char *text);

#endif
