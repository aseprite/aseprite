// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/styled_button.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

namespace app {

using namespace ui;

StyledButton::StyledButton(skin::Style* style)
  : Button("")
  , m_style(style)
{
}

bool StyledButton::onProcessMessage(Message* msg) {
  switch (msg->type()) {
    case kSetCursorMessage:
      if (isEnabled()) {
        ui::set_mouse_cursor(kHandCursor);
        return true;
      }
      break;
  }
  return Button::onProcessMessage(msg);
}

void StyledButton::onSizeHint(SizeHintEvent& ev) {
  ev.setSizeHint(
    m_style->sizeHint(NULL, skin::Style::State()) + 4*guiscale());
}

void StyledButton::onPaint(PaintEvent& ev) {
  Graphics* g = ev.getGraphics();
  skin::Style::State state;
  if (hasMouse()) state += skin::Style::hover();
  if (isSelected()) state += skin::Style::clicked();
  m_style->paint(g, getClientBounds(), NULL, state);
}

} // namespace app
