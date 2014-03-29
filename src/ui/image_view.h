// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_IMAGE_VIEW_H_INCLUDED
#define UI_IMAGE_VIEW_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "ui/widget.h"

struct BITMAP;

namespace ui {

  class ImageView : public Widget
  {
  public:
    ImageView(BITMAP* bmp, int align);

  protected:
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    BITMAP* m_bmp;
  };

} // namespace ui

#endif
