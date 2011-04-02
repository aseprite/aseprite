// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_POPUP_FRAME_H_INCLUDED
#define GUI_POPUP_FRAME_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/frame.h"

class PopupFrame : public Frame
{
public:
  PopupFrame(const char* text, bool close_on_buttonpressed);
  ~PopupFrame();

  void setHotRegion(JRegion region);

  void makeFloating();
  void makeFixed();

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

private:
  void startFilteringMessages();
  void stopFilteringMessages();

  bool m_close_on_buttonpressed;
  JRegion m_hot_region;
  bool m_filtering;
};

#endif
