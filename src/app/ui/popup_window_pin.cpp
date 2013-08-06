/* ASEPRITE
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

#include "app/ui/popup_window_pin.h"

#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "ui/ui.h"

#include <allegro.h>
#include <vector>

namespace app {

using namespace app::skin;
using namespace ui;

PopupWindowPin::PopupWindowPin(const char* text, bool close_on_buttonpressed)
  : PopupWindow(text, close_on_buttonpressed)
  , m_pin("")
{
  // Configure the micro check-box look without borders, only the "pin" icon is shown.
  setup_look(&m_pin, WithoutBordersLook);
  m_pin.child_spacing = 0;
  m_pin.setBorder(gfx::Border(0));

  m_pin.Click.connect(&PopupWindowPin::onPinClick, this);

  set_gfxicon_to_button(&m_pin, PART_UNPINNED, PART_PINNED, PART_UNPINNED, JI_CENTER | JI_MIDDLE);
}

void PopupWindowPin::onPinClick(Event& ev)
{
  if (m_pin.isSelected()) {
    makeFloating();
  }
  else {
    gfx::Rect rc = getBounds();
    rc.enlarge(8);
    setHotRegion(gfx::Region(rc));
    makeFixed();
  }
}

bool PopupWindowPin::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      m_pin.setSelected(false);
      makeFixed();
      break;

    case kCloseMessage:
      m_pin.setSelected(false);
      break;

  }

  return PopupWindow::onProcessMessage(msg);
}

void PopupWindowPin::onHitTest(HitTestEvent& ev)
{
  PopupWindow::onHitTest(ev);

  if (m_pin.isSelected() &&
      ev.getHit() == HitTestClient) {
    if (ev.getPoint().x <= rc->x1+2)
      ev.setHit(HitTestBorderW);
    else if (ev.getPoint().x >= rc->x2-3)
      ev.setHit(HitTestBorderE);
    else
      ev.setHit(HitTestCaption);
  }
}

} // namespace app
