// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/image_view.h"

#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

ImageView::ImageView(she::Surface* sur, int align)
 : Widget(kImageViewWidget)
 , m_sur(sur)
{
  setAlign(align);
}

void ImageView::onSizeHint(SizeHintEvent& ev)
{
  gfx::Rect box;
  getTextIconInfo(&box, NULL, NULL,
    getAlign(), m_sur->width(), m_sur->height());

  ev.setSizeHint(
    gfx::Size(
      box.w + border().width(),
      box.h + border().height()));
}

void ImageView::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  gfx::Rect bounds = getClientBounds();
  gfx::Rect icon;
  getTextIconInfo(NULL, NULL, &icon, getAlign(),
    m_sur->width(), m_sur->height());

  g->fillRect(getBgColor(), bounds);
  g->drawRgbaSurface(m_sur, icon.x, icon.y);
}

} // namespace ui
