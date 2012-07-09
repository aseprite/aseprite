/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "widgets/popup_window_pin.h"

#include "base/bind.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "skin/skin_theme.h"
#include "ui/gui.h"

#include <allegro.h>
#include <vector>

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
    JRect rc = jrect_new(this->rc->x1-8, this->rc->y1-8, this->rc->x2+8, this->rc->y2+8);
    JRegion rgn = jregion_new(rc, 1);
    jrect_free(rc);

    setHotRegion(rgn);
    makeFixed();
  }
}

bool PopupWindowPin::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_OPEN:
      m_pin.setSelected(false);
      makeFixed();
      break;

    case JM_CLOSE:
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
