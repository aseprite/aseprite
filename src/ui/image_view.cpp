// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "ui/draw.h"
#include "ui/image_view.h"
#include "ui/message.h"
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
  gfx::Rect icon;
  jwidget_get_texticon_info(this, NULL, NULL, &icon,
                            getAlign(), m_bmp->w, m_bmp->h);

  jdraw_rectexclude(getBounds(), icon, getBgColor());

  blit(m_bmp, ji_screen, 0, 0, icon.x, icon.y, icon.w, icon.h);
}

} // namespace ui
