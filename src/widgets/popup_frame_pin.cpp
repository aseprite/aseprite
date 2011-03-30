/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "widgets/popup_frame_pin.h"

#include "base/bind.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "gui/gui.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "skin/skin_theme.h"

#include <allegro.h>
#include <vector>

PopupFramePin::PopupFramePin(const char* text, bool close_on_buttonpressed)
  : PopupFrame(text, close_on_buttonpressed)
  , m_pin("")
{
  // Configure the micro check-box look without borders, only the "pin" icon is shown.
  setup_look(&m_pin, WithoutBordersLook);
  m_pin.child_spacing = 0;
  m_pin.setBorder(gfx::Border(0));

  m_pin.Click.connect(&PopupFramePin::onPinClick, this);

  set_gfxicon_to_button(&m_pin, PART_UNPINNED, PART_PINNED, PART_UNPINNED, JI_CENTER | JI_MIDDLE);
}

void PopupFramePin::onPinClick(Event& ev)
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

bool PopupFramePin::onProcessMessage(JMessage msg)
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

  return PopupFrame::onProcessMessage(msg);
}

void PopupFramePin::onHitTest(HitTestEvent& ev)
{
  PopupFrame::onHitTest(ev);

  if (m_pin.isSelected() && ev.getHit() == HitTestClient)
    ev.setHit(HitTestCaption);
}
