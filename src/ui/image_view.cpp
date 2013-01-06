// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "ui/draw.h"
#include "ui/image_view.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/rect.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

ImageView::ImageView(BITMAP* bmp, int align)
 : Widget(JI_IMAGE_VIEW)
{
  setAlign(align);
}

void ImageView::onPreferredSize(PreferredSizeEvent& ev)
{
  struct jrect box, text, icon;
  jwidget_get_texticon_info(this, &box, &text, &icon,
                            getAlign(), m_bmp->w, m_bmp->h);

  ev.setPreferredSize(gfx::Size(border_width.l + jrect_w(&box) + border_width.r,
                                border_width.t + jrect_h(&box) + border_width.b));
}

void ImageView::onPaint(PaintEvent& ev)
{
  struct jrect box, text, icon;

  jwidget_get_texticon_info(this, &box, &text, &icon,
                            getAlign(), m_bmp->w, m_bmp->h);

  jdraw_rectexclude(rc, &icon, getBgColor());

  blit(m_bmp, ji_screen, 0, 0,
       icon.x1, icon.y1, jrect_w(&icon), jrect_h(&icon));
}

} // namespace ui
