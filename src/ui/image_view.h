// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_IMAGE_VIEW_H_INCLUDED
#define UI_IMAGE_VIEW_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

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
