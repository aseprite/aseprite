// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/drop_down_button.h"

#include "app/modules/gui.h"
#include "app/ui/skin/button_icon_impl.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/theme.h"

namespace app {

using namespace app::skin;
using namespace ui;

DropDownButton::DropDownButton(const char* text)
  : HBox()
  , m_button(new Button(text))
  , m_dropDown(new Button(""))
{
  SkinTheme* theme = SkinTheme::instance();

  setup_look(m_button, LeftButtonLook);
  setup_look(m_dropDown, RightButtonLook);

  m_button->setExpansive(true);
  m_button->setAlign(LEFT | MIDDLE);
  m_button->Click.connect(&DropDownButton::onButtonClick, this);
  m_dropDown->Click.connect(&DropDownButton::onDropDownButtonClick, this);

  addChild(m_button);
  addChild(m_dropDown);

  setChildSpacing(0);

  m_dropDown->setIconInterface
    (new ButtonIconImpl(theme->parts.comboboxArrowDown(),
                        theme->parts.comboboxArrowDownSelected(),
                        theme->parts.comboboxArrowDownDisabled(),
                        CENTER | MIDDLE));
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

} // namespace app
