// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/brush_popup.h"

#include "gfx/region.h"
#include "gfx/border.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/button_set.h"
#include "doc/brush.h"

namespace app {

using namespace app::skin;
using namespace ui;

BrushPopup::BrushPopup()
  : PopupWindow("", kCloseOnClickInOtherWindow)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

  setAutoRemap(false);
  setBorder(gfx::Border(0));
  child_spacing = 0;

  m_brushTypeButton = new ButtonSet(3);
  m_brushTypeButton->addItem(theme->get_part(PART_BRUSH_CIRCLE));
  m_brushTypeButton->addItem(theme->get_part(PART_BRUSH_SQUARE));
  m_brushTypeButton->addItem(theme->get_part(PART_BRUSH_LINE));
  m_brushTypeButton->ItemChange.connect(&BrushPopup::onBrushTypeChange, this);
  m_brushTypeButton->setTransparent(true);
  m_brushTypeButton->setBgColor(gfx::ColorNone);
  addChild(m_brushTypeButton);
}

void BrushPopup::setBrush(doc::Brush* brush)
{
  m_brushTypeButton->setSelectedItem(brush->type());
}

void BrushPopup::onBrushTypeChange()
{
  doc::BrushType brushType = (doc::BrushType)m_brushTypeButton->selectedItem();
  doc::Brush brush;
  brush.setType(brushType);

  BrushChange(&brush);
}

} // namespace app
