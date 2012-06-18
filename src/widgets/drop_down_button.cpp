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

#include "widgets/drop_down_button.h"

#include "modules/gui.h"
#include "skin/button_icon_impl.h"
#include "skin/skin_property.h"
#include "skin/skin_theme.h"
#include "ui/theme.h"

using namespace ui;

DropDownButton::DropDownButton(const char* text)
  : HBox()
  , m_button(new Button(text))
  , m_dropDown(new Button(""))
{
  setup_look(m_button, LeftButtonLook);
  setup_look(m_dropDown, RightButtonLook);

  m_button->Click.connect(&DropDownButton::onButtonClick, this);
  m_dropDown->Click.connect(&DropDownButton::onDropDownButtonClick, this);

  addChild(m_button);
  addChild(m_dropDown);

  child_spacing = 0;

  m_dropDown->setIconInterface
    (new ButtonIconImpl(static_cast<SkinTheme*>(m_dropDown->getTheme()),
                        PART_COMBOBOX_ARROW_DOWN,
                        PART_COMBOBOX_ARROW_DOWN_SELECTED,
                        PART_COMBOBOX_ARROW_DOWN_DISABLED,
                        JI_CENTER | JI_MIDDLE));
}

void DropDownButton::onButtonClick(Event& ev)
{
  // Fire "Click" signal.
  Click();
}

void DropDownButton::onDropDownButtonClick(Event& ev)
{
  DropDownClick();
}
