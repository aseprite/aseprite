// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/panel.h"

#include "ui/resize_event.h"
#include "ui/size_hint_event.h"

#include <algorithm>

namespace ui {

Panel::Panel()
  : m_multiple(false)
{
}

void Panel::showChild(Widget* widget)
{
  m_multiple = false;
  for (auto child : children()) {
    if (!child->isDecorative())
      child->setVisible(child == widget);
  }
  layout();
}

void Panel::showAllChildren()
{
  m_multiple = true;
  for (auto child : children()) {
    if (!child->isDecorative())
      child->setVisible(true);
  }
  layout();
}

void Panel::onResize(ResizeEvent& ev)
{
  if (m_multiple) {
    VBox::onResize(ev);
    return;
  }

  // Copy the new position rectangle
  setBoundsQuietly(ev.bounds());

  // Set all the children to the same "cpos"
  gfx::Rect cpos = childrenBounds();
  for (auto child : children()) {
    if (!child->isDecorative())
      child->setBounds(cpos);
  }
}

void Panel::onSizeHint(SizeHintEvent& ev)
{
  if (m_multiple) {
    VBox::onSizeHint(ev);
    return;
  }

  gfx::Size maxSize(0, 0);
  gfx::Size reqSize;

  for (auto child : children()) {
    if (!child->isDecorative()) {
      reqSize = child->sizeHint();

      maxSize.w = std::max(maxSize.w, reqSize.w);
      maxSize.h = std::max(maxSize.h, reqSize.h);
    }
  }

  if (hasText())
    maxSize.w = std::max(maxSize.w, textWidth());

  ev.setSizeHint(
    maxSize.w + border().width(),
    maxSize.h + border().height());
}

} // namespace ui
