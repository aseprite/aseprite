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
  setup_mini_font(this);
  setAlign(CENTER | MIDDLE);
  setFocusStop(true);
}

void ButtonSet::Item::setIcon(const SkinPartPtr& icon)
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
  gfx::Color fg, bg;
  SkinPartPtr nw;
  gfx::Rect boxRc, textRc, iconRc;
  gfx::Size iconSize;
  if (m_icon)
    iconSize = m_icon->getSize();

  getTextIconInfo(
    &boxRc, &textRc, &iconRc,
    CENTER | (hasText() ? BOTTOM: MIDDLE),
    iconSize.w, iconSize.h);

  Grid::Info info = buttonSet()->getChildInfo(this);
  bool isLastCol = (info.col+info.hspan >= info.grid_cols);
  bool isLastRow = (info.row+info.vspan >= info.grid_rows);

  if (m_icon || isLastRow) {
    textRc.y -= 1*guiscale();
    iconRc.y -= 1*guiscale();
  }

  if (!gfx::is_transparent(getBgColor()))
    g->fillRect(getBgColor(), g->getClipBounds());

  if (isSelected() || hasMouseOver()) {
    if (hasCapture()) {
      nw = theme->parts.toolbuttonPushed();
      fg = theme->colors.buttonSelectedText();
      bg = theme->colors.buttonSelectedFace();
    }
    else {
      nw = (hasFocus() ? theme->parts.toolbuttonHotFocused():
                         theme->parts.toolbuttonHot());
      fg = theme->colors.buttonHotText();
      bg = theme->colors.buttonHotFace();
    }
  }
  else {
    nw = (hasFocus() ? theme->parts.toolbuttonFocused():
                       theme->parts.toolbuttonLast());
    fg = theme->colors.buttonNormalText();
    bg = theme->colors.buttonNormalFace();
  }

  if (!isLastCol)
    rc.w += 1*guiscale();

  if (!isLastRow) {
    if (nw == theme->parts.toolbuttonHotFocused())
      rc.h += 2*guiscale();
    else
      rc.h += 3*guiscale();
  }

  theme->drawRect(g, rc, nw.get(), bg);

  if (m_icon) {
    if (isSelected() && hasCapture())
      g->drawColoredRgbaSurface(m_icon->getBitmap(0), theme->colors.buttonSelectedText(),
                                iconRc.x, iconRc.y);
    else
      g->drawRgbaSurface(m_icon->getBitmap(0), iconRc.x, iconRc.y);
  }

  if (hasText()) {
    g->setFont(getFont());
    g->drawUIString(getText(), fg, gfx::ColorNone, textRc.getOrigin(),
                    false);
  }
}

bool ButtonSet::Item::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
    case kFocusLeaveMessage:
      if (isEnabled()) {
        // TODO theme specific stuff
        invalidate();
      }
      break;

    case ui::kKeyDownMessage:
      if (isEnabled() && hasText()) {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        bool mnemonicPressed =
          (msg->altPressed() &&
           getMnemonicChar() &&
           getMnemonicChar() == tolower(keymsg->unicodeChar()));

        if (mnemonicPressed ||
            (hasFocus() && keymsg->scancode() == kKeySpace)) {
          buttonSet()->setSelectedItem(this);
          buttonSet()->onItemChange(this);
        }
      }
      break;

    case ui::kMouseDownMessage:
      captureMouse();
      buttonSet()->setSelectedItem(this);
      invalidate();

      if (static_cast<MouseMessage*>(msg)->left() &&
          !buttonSet()->m_triggerOnMouseUp) {
        buttonSet()->onItemChange(this);
      }
      break;

    case ui::kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
        invalidate();

        if (static_cast<MouseMessage*>(msg)->left()) {
          if (buttonSet()->m_triggerOnMouseUp)
            buttonSet()->onItemChange(this);
        }
        else if (static_cast<MouseMessage*>(msg)->right()) {
          buttonSet()->onRightClick(this);
        }
      }
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
  gfx::Size iconSize;
  if (m_icon) {
    iconSize = m_icon->getSize();
    iconSize.w = MAX(iconSize.w, 16*guiscale());
    iconSize.h = MAX(iconSize.h, 16*guiscale());
  }

  gfx::Rect boxRc;
  getTextIconInfo(
    &boxRc, NULL, NULL,
    CENTER | (hasText() ? BOTTOM: MIDDLE),
    iconSize.w, iconSize.h);

  gfx::Size sz = boxRc.getSize();
  if (hasText())
    sz += 8*guiscale();

  Grid::Info info = buttonSet()->getChildInfo(this);
  if (info.row == info.grid_rows-1)
    sz.h += 3*guiscale();

  ev.setPreferredSize(sz);
}

ButtonSet::ButtonSet(int columns)
  : Grid(columns, false)
  , m_offerCapture(true)
  , m_triggerOnMouseUp(false)
  , m_multipleSelection(false)
{
  noBorderNoChildSpacing();
}

ButtonSet::Item* ButtonSet::addItem(const std::string& text, int hspan, int vspan)
{
  Item* item = new Item();
  item->setText(text);
  addItem(item, hspan, vspan);
  return item;
}

ButtonSet::Item* ButtonSet::addItem(const skin::SkinPartPtr& icon, int hspan, int vspan)
{
  Item* item = new Item();
  item->setIcon(icon);
  addItem(item, hspan, vspan);
  return item;
}

ButtonSet::Item* ButtonSet::addItem(Item* item, int hspan, int vspan)
{
  addChildInCell(item, hspan, vspan, HORIZONTAL | VERTICAL);
  return item;
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
  if (!m_multipleSelection) {
    if (item && item->isSelected())
      return;

    Item* sel = findSelectedItem();
    if (sel)
      sel->setSelected(false);
  }

  if (item) {
    item->setSelected(!item->isSelected());
    item->requestFocus();
  }
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

void ButtonSet::setTriggerOnMouseUp(bool state)
{
  m_triggerOnMouseUp = state;
}

void ButtonSet::setMultipleSelection(bool state)
{
  m_multipleSelection = state;
}

void ButtonSet::onItemChange(Item* item)
{
  ItemChange(item);
}

void ButtonSet::onRightClick(Item* item)
{
  RightClick(item);
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
