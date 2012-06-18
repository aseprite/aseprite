// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_IMAGE_VIEW_H_INCLUDED
#define GUI_IMAGE_VIEW_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

struct BITMAP;

namespace ui {

  class ImageView : public Widget
  {
  public:
    ImageView(BITMAP* bmp, int align);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    BITMAP* m_bmp;
  };

} // namespace ui

#endif
