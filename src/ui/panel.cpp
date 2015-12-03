// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/panel.h"

#include "ui/resize_event.h"
#include "ui/preferred_size_event.h"

namespace ui {

Panel::Panel()
  : Widget(kPanelWidget)
{
}

void Panel::showChild(Widget* widget)
{
  for (auto child : children()) {
    if (!child->isDecorative())
      child->setVisible(child == widget);
  }
  layout();
}

void Panel::onResize(ResizeEvent& ev)
{
  // Copy the new position rectangle
  setBoundsQuietly(ev.getBounds());

  // Set all the children to the same "cpos"
  gfx::Rect cpos = getChildrenBounds();
  for (auto child : children()) {
    if (!child->isDecorative())
      child->setBounds(cpos);
  }
}

void Panel::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size maxSize(0, 0);
  gfx::Size reqSize;

  for (auto child : children()) {
    if (!child->isDecorative()) {
      reqSize = child->getPreferredSize();

      maxSize.w = MAX(maxSize.w, reqSize.w);
      maxSize.h = MAX(maxSize.h, reqSize.h);
    }
  }

  if (hasText())
    maxSize.w = MAX(maxSize.w, getTextWidth());

  ev.setPreferredSize(
    maxSize.w + border().width(),
    maxSize.h + border().height());
}

} // namespace ui
