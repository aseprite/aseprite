// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/button_set.h"

#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "gfx/color.h"
#include "os/surface.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <algorithm>
#include <cstdarg>

namespace app {

using namespace ui;
using namespace app::skin;

// Last selected item for ButtonSet activated on mouse up when the
// mouse capture is get.
static int g_itemBeforeCapture = -1;

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
  , m_hotColor(gfx::ColorNone)
{
  setup_mini_font(this);
  setAlign(CENTER | MIDDLE);
  setFocusStop(true);
}

void ButtonSet::Item::setHotColor(gfx::Color color)
{
  m_hotColor = color;
}

void ButtonSet::Item::setIcon(const SkinPartPtr& icon, bool mono)
{
  m_icon = icon;
  m_mono = mono;
  invalidate();
}

ButtonSet* ButtonSet::Item::buttonSet()
{
  return static_cast<ButtonSet*>(parent());
}

void ButtonSet::Item::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  Graphics* g = ev.graphics();
  gfx::Rect rc = clientBounds();
  gfx::Color fg;
  SkinPartPtr nw;
  gfx::Rect boxRc, textRc, iconRc;
  gfx::Size iconSize;
  if (m_icon)
    iconSize = m_icon->size();

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

  if (!gfx::is_transparent(bgColor()))
    g->fillRect(bgColor(), g->getClipBounds());

  if (isSelected() || hasMouseOver()) {
    if (hasCapture()) {
      nw = theme->parts.buttonsetItemPushed();
      fg = theme->colors.buttonSelectedText();
    }
    else {
      nw = (hasFocus() ? theme->parts.buttonsetItemHotFocused():
                         theme->parts.buttonsetItemHot());
      fg = theme->colors.buttonHotText();
    }
  }
  else {
    nw = (hasFocus() ? theme->parts.buttonsetItemFocused():
                       theme->parts.buttonsetItemNormal());
    fg = theme->colors.buttonNormalText();
  }

  if (!isLastCol)
    rc.w += 1*guiscale();

  if (!isLastRow) {
    if (nw == theme->parts.buttonsetItemHotFocused())
      rc.h += 2*guiscale();
    else
      rc.h += 3*guiscale();
  }

  theme->drawRect(g, rc, nw.get(),
                  gfx::is_transparent(m_hotColor));

  if (!gfx::is_transparent(m_hotColor)) {
    gfx::Rect rc2(rc);
    gfx::Rect sprite(nw->spriteBounds());
    gfx::Rect slices(nw->slicesBounds());
    rc2.shrink(
      gfx::Border(
        slices.x-1, // TODO this "-1" is an ugly hack for the pal edit
                    //      button, replace all this with styles
        slices.y-1,
        sprite.w-slices.w-slices.x-1,
        sprite.h-slices.h-slices.y));
    g->fillRect(m_hotColor, rc2);
  }

  if (m_icon) {
    os::Surface* bmp = m_icon->bitmap(0);

    if (isSelected() && hasCapture())
      g->drawColoredRgbaSurface(bmp, theme->colors.buttonSelectedText(),
                                iconRc.x, iconRc.y);
    else if (m_mono)
      g->drawColoredRgbaSurface(bmp, theme->colors.buttonNormalText(),
                                iconRc.x, iconRc.y);
    else
      g->drawRgbaSurface(bmp, iconRc.x, iconRc.y);
  }

  if (hasText()) {
    g->setFont(font());
    g->drawUIText(text(), fg, gfx::ColorNone, textRc.origin(), 0);
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
        bool mnemonicPressed = (msg->altPressed() &&
                                isMnemonicPressed(keymsg));

        if (mnemonicPressed ||
            (hasFocus() && keymsg->scancode() == kKeySpace)) {
          buttonSet()->onSelectItem(this, true, msg);
          onClick();
        }
      }
      break;

    case ui::kMouseDownMessage:
      // Only for single-item and trigerred on mouse up ButtonSets: We
      // save the current selected item to restore it just in case the
      // user leaves the ButtonSet without releasing the mouse button
      // and the mouse capture if offered to other ButtonSet.
      if (buttonSet()->m_triggerOnMouseUp) {
        ASSERT(g_itemBeforeCapture < 0);
        g_itemBeforeCapture = buttonSet()->selectedItem();
      }

      captureMouse();
      buttonSet()->onSelectItem(this, true, msg);
      invalidate();

      if (static_cast<MouseMessage*>(msg)->left() &&
          !buttonSet()->m_triggerOnMouseUp) {
        onClick();
      }
      break;

    case ui::kMouseUpMessage:
      if (hasCapture()) {
        if (g_itemBeforeCapture >= 0)
          g_itemBeforeCapture = -1;

        releaseMouse();
        invalidate();

        if (static_cast<MouseMessage*>(msg)->left()) {
          if (buttonSet()->m_triggerOnMouseUp)
            onClick();
        }
        else if (static_cast<MouseMessage*>(msg)->right()) {
          onRightClick();
        }
      }
      break;

    case ui::kMouseMoveMessage:
      if (hasCapture()) {
        if (buttonSet()->m_offerCapture) {
          if (offerCapture(static_cast<ui::MouseMessage*>(msg), buttonset_item_type())) {
            // Only for ButtonSets trigerred on mouse up.
            if (buttonSet()->m_triggerOnMouseUp &&
                g_itemBeforeCapture >= 0) {
              if (g_itemBeforeCapture < (int)children().size()) {
                Item* item = dynamic_cast<Item*>(at(g_itemBeforeCapture));
                ASSERT(item);

                // As we never received a kMouseUpMessage (so we never
                // called onClick()), we have to restore the selected
                // item at the point when we received the mouse capture.
                buttonSet()->onSelectItem(item, true, msg);
              }
              g_itemBeforeCapture = -1;
            }
          }
        }
      }
      break;

    case ui::kMouseLeaveMessage:
    case ui::kMouseEnterMessage:
      invalidate();
      break;
  }
  return Widget::onProcessMessage(msg);
}

void ButtonSet::Item::onSizeHint(ui::SizeHintEvent& ev)
{
  gfx::Size iconSize;
  if (m_icon) {
    iconSize = m_icon->size();
    iconSize.w = std::max(iconSize.w, 16*guiscale());
    iconSize.h = std::max(iconSize.h, 16*guiscale());
  }

  gfx::Rect boxRc;
  getTextIconInfo(
    &boxRc, NULL, NULL,
    CENTER | (hasText() ? BOTTOM: MIDDLE),
    iconSize.w, iconSize.h);

  gfx::Size sz = boxRc.size();
  if (hasText())
    sz += 8*guiscale();

  Grid::Info info = buttonSet()->getChildInfo(this);
  if (info.row == info.grid_rows-1)
    sz.h += 3*guiscale();

  ev.setSizeHint(sz);
}

void ButtonSet::Item::onClick()
{
  buttonSet()->onItemChange(this);
}

void ButtonSet::Item::onRightClick()
{
  buttonSet()->onRightClick(this);
}

ButtonSet::ButtonSet(int columns)
  : Grid(columns, false)
  , m_offerCapture(true)
  , m_triggerOnMouseUp(false)
  , m_multiMode(MultiMode::One)
{
  InitTheme.connect(
    [this]{
      noBorderNoChildSpacing();
    });
  initTheme();
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

int ButtonSet::getItemIndex(const Item* item) const
{
  int index = 0;
  for (Widget* child : children()) {
    if (child == item)
      return index;
    ++index;
  }
  return -1;
}

int ButtonSet::selectedItem() const
{
  int index = 0;
  for (Widget* child : children()) {
    if (child->isSelected())
      return index;
    ++index;
  }
  return -1;
}

int ButtonSet::countSelectedItems() const
{
  int count = 0;
  for (auto child : children())
    if (child->isSelected())
      ++count;
  return count;
}

void ButtonSet::setSelectedItem(int index, bool focusItem)
{
  if (index >= 0 && index < (int)children().size())
    setSelectedItem(static_cast<Item*>(at(index)), focusItem);
  else
    setSelectedItem(static_cast<Item*>(nullptr), focusItem);
}

void ButtonSet::setSelectedItem(Item* item, bool focusItem)
{
  onSelectItem(item, focusItem, nullptr);
}

void ButtonSet::onSelectItem(Item* item, bool focusItem, ui::Message* msg)
{
  const int count = countSelectedItems();

  if ((m_multiMode == MultiMode::One) ||
      (m_multiMode == MultiMode::OneOrMore &&
       msg &&
       !msg->shiftPressed() &&
       !msg->altPressed() &&
       !msg->ctrlPressed() &&
       !msg->cmdPressed())) {
    if (item && item->isSelected() &&
        ((m_multiMode == MultiMode::One) ||
         (m_multiMode == MultiMode::OneOrMore && count == 1)))
      return;

    if (m_multiMode == MultiMode::One) {
      if (auto sel = findSelectedItem())
        sel->setSelected(false);
    }
    else if (m_multiMode == MultiMode::OneOrMore) {
      for (auto child : children())
        child->setSelected(false);
    }
  }

  if (item) {
    if (m_multiMode == MultiMode::OneOrMore) {
      // Item already selected
      if (count == 1 && item == findSelectedItem())
        return;
    }

    // Toggle item
    item->setSelected(!item->isSelected());
    if (focusItem)
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

void ButtonSet::setMultiMode(MultiMode mode)
{
  m_multiMode = mode;
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
  for (auto child : children()) {
    if (child->isSelected())
      return static_cast<Item*>(child);
  }
  return nullptr;
}

} // namespace app
