// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/drop_down_button.h"

#include "app/modules/gui.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "ui/button.h"
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

  m_button->setStyle(theme->styles.dropDownButton());
  m_dropDown->setStyle(theme->styles.dropDownExpandButton());

  m_button->setExpansive(true);
  m_button->Click.connect(&DropDownButton::onButtonClick, this);
  m_dropDown->Click.connect(&DropDownButton::onDropDownButtonClick, this);

  addChild(m_button);
  addChild(m_dropDown);

  setChildSpacing(0);
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
