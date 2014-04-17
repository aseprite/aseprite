// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "ui/draw.h"
#include "ui/graphics.h"
#include "ui/image_view.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

ImageView::ImageView(BITMAP* bmp, int align)
 : Widget(kImageViewWidget)
{
  setAlign(align);
}

void ImageView::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Rect box;
  jwidget_get_texticon_info(this, &box, NULL, NULL,
                            getAlign(), m_bmp->w, m_bmp->h);

  ev.setPreferredSize(gfx::Size(border_width.l + box.w + border_width.r,
                                border_width.t + box.h + border_width.b));
}

void ImageView::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  gfx::Rect bounds = getClientBounds();
  gfx::Rect icon;
  jwidget_get_texticon_info(this, NULL, NULL, &icon,
                            getAlign(), m_bmp->w, m_bmp->h);

  g->fillRect(getBgColor(), bounds);

  icon.x -= getBounds().x;
  icon.y -= getBounds().y;

  g->blit(m_bmp, 0, 0, icon.x, icon.y, icon.w, icon.h);
}

} // namespace ui
