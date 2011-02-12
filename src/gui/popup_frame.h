// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_POPUP_FRAME_H_INCLUDED
#define GUI_POPUP_FRAME_H_INCLUDED

#include "gui/frame.h"

class PopupFrame : public Frame
{
public:
  PopupFrame(const char* text, bool close_on_buttonpressed);
  ~PopupFrame();

  void setHotRegion(JRegion region);

protected:
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);
  void onPaint(PaintEvent& ev);

private:
  bool m_close_on_buttonpressed;
  JRegion m_hot_region;
  bool m_filtering;
};

#endif
