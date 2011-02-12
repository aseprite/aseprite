// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_IMAGE_VIEW_H_INCLUDED
#define GUI_IMAGE_VIEW_H_INCLUDED

#include "gui/widget.h"

struct BITMAP;

class ImageView : public Widget
{
public:
   ImageView(BITMAP* bmp, int align);

protected:
  bool onProcessMessage(JMessage msg);
  void onPaint(PaintEvent& ev);

private:
  BITMAP* m_bmp;
};

#endif
