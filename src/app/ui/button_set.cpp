// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/button_set.h"

#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "gfx/color.h"
#include "she/surface.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <cstdarg>

namespace app {

using namespace ui;
using namespace app::skin;

WidgetType buttonset_item_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

ButtonSet::Item::Item()
  : Widget(buttonset_item_type())
  , m_icon(NULL)
{
}

void ButtonSet::Item::setIcon(she::Surface* icon)
{
  m_icon = icon;
  invalidate();
}

ButtonSet* ButtonSet::Item::buttonSet()
{
  return static_cast<ButtonSet*>(getParent());
}

void ButtonSet::Item::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  Graphics* g = ev.getGraphics();
  gfx::Rect rc = getClientBounds();
  gfx::Color face;
  int nw;

  if (!gfx::is_transparent(getBgColor()))
    g->fillRect(getBgColor(), g->getClipBounds());

  if (isSelected() || hasMouseOver()) {
    nw = PART_TOOLBUTTON_HOT_NW;
    face = theme->colors.buttonHotFace();
  }
  else {
    nw = PART_TOOLBUTTON_LAST_NW;
    face = theme->colors.buttonNormalFace();
  }

  Grid::Info info = buttonSet()->getChildInfo(this);
  if (info.col < info.grid_cols-1) rc.w+=1*guiscale();
  if (info.row < info.grid_rows-1) rc.h+=3*guiscale();

  theme->draw_bounds_nw(g, rc, nw, face);

  if (m_icon) {
    g->drawRgbaSurface(m_icon,
      rc.x + rc.w/2 - m_icon->width()/2,
      rc.y + rc.h/2 - m_icon->height()/2);
  }
}

bool ButtonSet::Item::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case ui::kMouseDownMessage:
      captureMouse();
      buttonSet()->setSelectedItem(this);
      buttonSet()->onItemChange();
      break;

    case ui::kMouseUpMessage:
      if (hasCapture())
        releaseMouse();
      break;

    case ui::kMouseMoveMessage:
      if (hasCapture()) {
        if (buttonSet()->m_offerCapture)
          offerCapture(static_cast<ui::MouseMessage*>(msg), buttonset_item_type());
      }
      break;

    case ui::kMouseLeaveMessage:
    case ui::kMouseEnterMessage:
      invalidate();
      break;
  }
  return Widget::onProcessMessage(msg);
}

void ButtonSet::Item::onPreferredSize(ui::PreferredSizeEvent& ev)
{
  gfx::Size sz(16, 16); // TODO Calculate from icon

  Grid::Info info = buttonSet()->getChildInfo(this);
  if (info.row == info.grid_rows-1)
    sz.h += 3;

  ev.setPreferredSize(sz*guiscale());
}

ButtonSet::ButtonSet(int columns)
  : Grid(columns, false)
  , m_offerCapture(true)
{
  noBorderNoChildSpacing();
}

void ButtonSet::addItem(she::Surface* icon, int hspan, int vspan)
{
  Item* item = new Item();
  item->setIcon(icon);
  addChildInCell(item, hspan, vspan, JI_CENTER | JI_MIDDLE);
}

ButtonSet::Item* ButtonSet::getItem(int index)
{
  return dynamic_cast<Item*>(at(index));
}

int ButtonSet::selectedItem() const
{
  int index = 0;
  for (Widget* child : getChildren()) {
    if (child->isSelected())
      return index;
    ++index;
  }
  return -1;
}

void ButtonSet::setSelectedItem(int index)
{
  if (index >= 0 && index < (int)getChildren().size())
    setSelectedItem(static_cast<Item*>(at(index)));
  else
    setSelectedItem(static_cast<Item*>(NULL));
}

void ButtonSet::setSelectedItem(Item* item)
{
  if (item && item->isSelected())
    return;

  Item* sel = findSelectedItem();
  if (sel)
    sel->setSelected(false);

  if (item)
    item->setSelected(true);
}

void ButtonSet::deselectItems()
{
  Item* sel = findSelectedItem();
  if (sel)
    sel->setSelected(false);
}

void ButtonSet::setOfferCapture(bool state)
{
  m_offerCapture = state;
}

void ButtonSet::onItemChange()
{
  ItemChange();
}

ButtonSet::Item* ButtonSet::findSelectedItem() const
{
  for (Widget* child : getChildren()) {
    if (child->isSelected())
      return static_cast<Item*>(child);
  }
  return NULL;
}

} // namespace app
