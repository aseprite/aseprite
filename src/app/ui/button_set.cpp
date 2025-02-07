// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
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

ButtonSet::Item::Item() : Widget(buttonset_item_type()), m_icon(NULL)
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
  return static_cast<ButtonSet*>(parent());
}

void ButtonSet::Item::onPaint(ui::PaintEvent& ev)
{
  if (style()) {
    gfx::Rect rc = clientBounds();
    Grid::Info info = buttonSet()->getChildInfo(this);
    bool isLastCol = (info.col + info.hspan >= info.grid_cols);
    bool isLastRow = (info.row + info.vspan >= info.grid_rows);
    // When gaps are negative we need to compensate client bounds size so the painting is based on a
    // complete button, and not just the part not overlapped.
    if (buttonSet()->m_colgap < 0 && !isLastCol)
      rc.w -= buttonSet()->m_colgap;
    if (buttonSet()->m_rowgap < 0 && !isLastRow)
      rc.h -= buttonSet()->m_rowgap;

    theme()->paintWidget(ev.graphics(), this, style(), rc);
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
        bool mnemonicPressed = (msg->altPressed() && isMnemonicPressed(keymsg));

        if (mnemonicPressed || (hasFocus() && keymsg->scancode() == kKeySpace)) {
          buttonSet()->onSelectItem(this, true, msg);
          onClick();
        }
      }
      break;

    case ui::kMouseDownMessage:
      if (!isEnabled())
        return true;
      // Only for single-item and trigerred on mouse up ButtonSets: We
      // save the current selected item to restore it just in case the
      // user leaves the ButtonSet without releasing the mouse button
      // and the mouse capture if offered to other ButtonSet.
      if (buttonSet()->m_triggerOnMouseUp) {
        // g_itemBeforeCapture can be >= 0 if we clicked other button
        // without releasing the first button.
        // ASSERT(g_itemBeforeCapture < 0);
        g_itemBeforeCapture = buttonSet()->selectedItem();
      }

      captureMouse();
      buttonSet()->onSelectItem(this, true, msg);
      invalidate();

      if (static_cast<MouseMessage*>(msg)->left() && !buttonSet()->m_triggerOnMouseUp) {
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
            if (buttonSet()->m_triggerOnMouseUp && g_itemBeforeCapture >= 0) {
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
      if (!isEnabled())
        return true;
      invalidate();
      break;
  }
  return Widget::onProcessMessage(msg);
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
  InitTheme.connect([this] {
    noBorderNoChildSpacing();
    // Set default buttonset style if it wasn't already set.
    if (style() == SkinTheme::instance()->styles.grid()) {
      setStyle(SkinTheme::instance()->styles.buttonset());
    }
  });
  initTheme();
}

ButtonSet::Item* ButtonSet::addItem(const std::string& text, ui::Style* style)
{
  return addItem(text, 1, 1, style);
}

ButtonSet::Item* ButtonSet::addItem(const std::string& text, int hspan, int vspan, ui::Style* style)
{
  Item* item = new Item();
  item->setText(text);
  addItem(item, hspan, vspan, style);
  return item;
}

ButtonSet::Item* ButtonSet::addItem(const skin::SkinPartPtr& icon, ui::Style* style)
{
  return addItem(icon, 1, 1, style);
}

ButtonSet::Item* ButtonSet::addItem(const skin::SkinPartPtr& icon,
                                    int hspan,
                                    int vspan,
                                    ui::Style* style)
{
  Item* item = new Item();
  item->setIcon(icon);
  addItem(item, hspan, vspan, style);
  return item;
}
ButtonSet::Item* ButtonSet::addItem(Item* item, ui::Style* style)
{
  return addItem(item, 1, 1, style);
}

ButtonSet::Item* ButtonSet::addItem(Item* item, int hspan, int vspan, ui::Style* style)
{
  item->InitTheme.connect([item, style] {
    ui::Style* s = style;
    if (!s) {
      auto* theme = SkinTheme::get(item);
      s = theme->styles.buttonsetItemIcon();
      if (!item->text().empty()) {
        s = (item->icon() ? theme->styles.buttonsetItemTextTopIconBottom() :
                            theme->styles.buttonsetItemText());
      }
    }
    item->setStyle(s);
  });
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
      (m_multiMode == MultiMode::OneOrMore && msg && !msg->shiftPressed() && !msg->altPressed() &&
       !msg->ctrlPressed() && !msg->cmdPressed())) {
    if (item && item->isSelected() &&
        ((m_multiMode == MultiMode::One) || (m_multiMode == MultiMode::OneOrMore && count == 1)))
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
