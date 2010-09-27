// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JPOPUP_WINDOW_H_INCLUDED
#define GUI_JPOPUP_WINDOW_H_INCLUDED

#include "gui/jwindow.h"

class PopupWindow : public Frame
{
  bool m_close_on_buttonpressed;
  JRegion m_hot_region;
  bool m_filtering;

public:
  PopupWindow(const char* text, bool close_on_buttonpressed);
  ~PopupWindow();

  void setHotRegion(JRegion region);

protected:
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);
};

#endif
