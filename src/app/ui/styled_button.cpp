/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/styled_button.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"

namespace app {

using namespace ui;

StyledButton::StyledButton(const std::string& styleName) : Button("") {
  m_style = skin::get_style(styleName);
}

bool StyledButton::onProcessMessage(Message* msg) {
  switch (msg->type()) {
    case kSetCursorMessage:
      ui::set_mouse_cursor(kHandCursor);
      return true;
  }
  return Button::onProcessMessage(msg);
}

void StyledButton::onPreferredSize(PreferredSizeEvent& ev) {
  ev.setPreferredSize(
    m_style->preferredSize(NULL, skin::Style::State()) + 4*jguiscale());
}

void StyledButton::onPaint(PaintEvent& ev) {
  Graphics* g = ev.getGraphics();
  skin::Style::State state;
  if (hasMouse()) state += skin::Style::hover();
  if (isSelected()) state += skin::Style::clicked();
  m_style->paint(g, getClientBounds(), NULL, state);
}

} // namespace app
