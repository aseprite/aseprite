// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/image_view.h"

#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"

namespace ui {

ImageView::ImageView(const os::SurfaceRef& sur, int align) : Widget(kImageViewWidget), m_sur(sur)
{
  setAlign(align);
}

void ImageView::onSizeHint(SizeHintEvent& ev)
{
  gfx::Rect box;
  getTextIconInfo(&box, nullptr, nullptr, align(), m_sur->width(), m_sur->height());

  ev.setSizeHint(gfx::Size(box.w + border().width(), box.h + border().height()));
}

void ImageView::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();
  gfx::Rect icon;
  getTextIconInfo(nullptr, nullptr, &icon, align(), m_sur->width(), m_sur->height());

  g->fillRect(bgColor(), bounds);
  g->drawRgbaSurface(m_sur.get(), icon.x, icon.y);
}

} // namespace ui
