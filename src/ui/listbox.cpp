// Aseprite UI Library
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/listbox.h"

#include "base/fs.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/resize_event.h"
#include "ui/separator.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

#include <algorithm>

namespace ui {

using namespace gfx;

ListBox::ListBox()
  : Widget(kListBoxWidget)
  , m_multiselect(false)
  , m_firstSelectedIndex(-1)
  , m_lastSelectedIndex(-1)
{
  setFocusStop(true);
  initTheme();
}

void ListBox::setMultiselect(const bool multiselect)
{
  m_multiselect = multiselect;
}

Widget* ListBox::getSelectedChild()
{
  for (auto child : children())
    if (child->isSelected())
      return child;

  return nullptr;
}

int ListBox::getSelectedIndex()
{
  int i = 0;

  for (auto child : children()) {
    if (child->isSelected())
      return i;

    i++;
  }

  return -1;
}

int ListBox::getChildIndex(Widget* item)
{
  const WidgetsList& children = this->children();
  auto it = std::find(children.begin(), children.end(), item);
  if (it != children.end())
    return it - children.begin();
  else
    return -1;
}

Widget* ListBox::getChildByIndex(int index)
{
  const WidgetsList& children = this->children();
  if (index >= 0 && index < int(children.size()))
    return children[index];
  else
    return nullptr;
}

void ListBox::selectChild(Widget* item, Message* msg)
{
  int itemIndex = getChildIndex(item);
  m_lastSelectedIndex = itemIndex;

  if (m_multiselect) {
    // Save current state of all children when we start selecting
    if (msg == nullptr ||
        msg->type() == kMouseDownMessage ||
        msg->type() == kKeyDownMessage) {
      m_firstSelectedIndex = itemIndex;
      m_states.resize(children().size());

      int i = 0;
      for (auto child : children()) {
        bool state = child->isSelected();
        if (msg && !msg->ctrlPressed() && !msg->cmdPressed())
          state = false;
        m_states[i] = state;
        ++i;
      }
    }
  }

  int i = 0;
  for (auto child : children()) {
    bool newState;

    if (m_multiselect) {
      ASSERT(i >= 0 && i < int(m_states.size()));
      newState = (i >= 0 && i < int(m_states.size()) ? m_states[i]: false);

      if (i >= MIN(itemIndex, m_firstSelectedIndex) &&
          i <= MAX(itemIndex, m_firstSelectedIndex)) {
        newState = !newState;
      }
    }
    else {
      newState = (child == item);
    }

    if (child->isSelected() != newState)
      child->setSelected(newState);

    ++i;
  }

  if (item)
    makeChildVisible(item);

  onChange();
}

void ListBox::selectIndex(int index, Message* msg)
{
  Widget* child = getChildByIndex(index);
  if (child)
    selectChild(child, msg);
}

int ListBox::getItemsCount() const
{
  return int(children().size());
}

void ListBox::makeChildVisible(Widget* child)
{
  View* view = View::getView(this);
  if (!view)
    return;

  gfx::Point scroll = view->viewScroll();
  gfx::Rect vp = view->viewportBounds();

  if (child->bounds().y < vp.y)
    scroll.y = child->bounds().y - bounds().y;
  else if (child->bounds().y > vp.y + vp.h - child->bounds().h)
    scroll.y = (child->bounds().y - bounds().y
                - vp.h + child->bounds().h);

  view->setViewScroll(scroll);
}

// Setup the scroll to center the selected item in the viewport
void ListBox::centerScroll()
{
  View* view = View::getView(this);
  Widget* item = getSelectedChild();

  if (view && item) {
    gfx::Rect vp = view->viewportBounds();
    gfx::Point scroll = view->viewScroll();

    scroll.y = ((item->bounds().y - bounds().y)
                - vp.h/2 + item->bounds().h/2);

    view->setViewScroll(scroll);
  }
}

inline bool sort_by_text(Widget* a, Widget* b) {
  return (base::compare_filenames(a->text(), b->text()) < 0);
}

void ListBox::sortItems()
{
  WidgetsList widgets = children();
  std::sort(widgets.begin(), widgets.end(), &sort_by_text);

  // Remove all children and add then again.
  removeAllChildren();
  for (Widget* child : widgets)
    addChild(child);
}

bool ListBox::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      centerScroll();
      break;

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        View* view = View::getView(this);
        bool pick_item = true;

        if (view && m_lastSelectedIndex >= 0) {
          gfx::Rect vp = view->viewportBounds();

          if (mousePos.y < vp.y) {
            int num = MAX(1, (vp.y - mousePos.y) / 8);
            selectIndex(MID(0, m_lastSelectedIndex-num, getItemsCount()-1), msg);
            pick_item = false;
          }
          else if (mousePos.y >= vp.y + vp.h) {
            int num = MAX(1, (mousePos.y - (vp.y+vp.h-1)) / 8);
            selectIndex(MID(0, m_lastSelectedIndex+num, getItemsCount()-1), msg);
            pick_item = false;
          }
        }

        if (pick_item) {
          Widget* picked = nullptr;

          if (view) {
            picked = view->viewport()->pick(mousePos);
          }
          else {
            picked = pick(mousePos);
          }

          // If the picked widget is a child of the list, select it
          if (picked && hasChild(picked))
            selectChild(picked, msg);
        }

        return true;
      }
      break;

    case kMouseUpMessage:
      releaseMouse();
      break;

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        auto mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point scroll = view->viewScroll();

        if (mouseMsg->preciseWheel())
          scroll += mouseMsg->wheelDelta();
        else
          scroll += mouseMsg->wheelDelta() * textHeight()*3;

        view->setViewScroll(scroll);
      }
      break;
    }

    case kKeyDownMessage:
      if (hasFocus() && !children().empty()) {
        int select = getSelectedIndex();
        int bottom = MAX(0, children().size()-1);
        View* view = View::getView(this);
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        if (keymsg->onlyCmdPressed()) {
          if (scancode == kKeyUp) scancode = kKeyHome;
          if (scancode == kKeyDown) scancode = kKeyEnd;
        }

        switch (scancode) {
          case kKeyUp:
            // Select previous element.
            if (select >= 0) {
              select = advanceIndexThroughVisibleItems(select, -1, true);
            }
            // Or select the bottom of the list if there is no
            // selected item.
            else {
              select = advanceIndexThroughVisibleItems(bottom+1, -1, true);
            }
            break;
          case kKeyDown:
            select = advanceIndexThroughVisibleItems(select, +1, true);
            break;
          case kKeyHome:
            select = advanceIndexThroughVisibleItems(-1, +1, false);
            break;
          case kKeyEnd:
            select = advanceIndexThroughVisibleItems(bottom+1, -1, false);
            break;
          case kKeyPageUp:
            if (view) {
              gfx::Rect vp = view->viewportBounds();
              select = advanceIndexThroughVisibleItems(
                select, -vp.h / textHeight(), false);
            }
            else
              select = 0;
            break;
          case kKeyPageDown:
            if (view) {
              gfx::Rect vp = view->viewportBounds();
              select = advanceIndexThroughVisibleItems(
                select, +vp.h / textHeight(), false);
            }
            else {
              select = bottom;
            }
            break;
          case kKeyLeft:
          case kKeyRight:
            if (view) {
              gfx::Rect vp = view->viewportBounds();
              gfx::Point scroll = view->viewScroll();
              int sgn = (keymsg->scancode() == kKeyLeft) ? -1: 1;

              scroll.x += vp.w/2*sgn;

              view->setViewScroll(scroll);
            }
            break;
          default:
            return Widget::onProcessMessage(msg);
        }

        selectIndex(MID(0, select, bottom), msg);
        return true;
      }
      break;

    case kDoubleClickMessage:
      onDoubleClickItem();
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void ListBox::onPaint(PaintEvent& ev)
{
  theme()->paintListBox(ev);
}

void ListBox::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  Rect cpos = childrenBounds();

  for (auto child : children()) {
    if (child->hasFlags(HIDDEN))
      continue;

    cpos.h = child->sizeHint().h;
    child->setBounds(cpos);

    cpos.y += child->bounds().h + childSpacing();
  }
}

void ListBox::onSizeHint(SizeHintEvent& ev)
{
  int w = 0, h = 0;
  int visibles = 0;

  for (auto child : children()) {
    if (child->hasFlags(HIDDEN))
      continue;

    Size reqSize = child->sizeHint();

    w = MAX(w, reqSize.w);
    h += reqSize.h;
    ++visibles;
  }
  if (visibles > 1)
    h += childSpacing() * (visibles-1);

  w += border().width();
  h += border().height();

  ev.setSizeHint(Size(w, h));
}

void ListBox::onChange()
{
  Change();
}

void ListBox::onDoubleClickItem()
{
  DoubleClickItem();
}

int ListBox::advanceIndexThroughVisibleItems(
  int startIndex, int delta, const bool loop)
{
  const int bottom = MAX(0, children().size()-1);
  const int sgn = SGN(delta);
  int index = startIndex;

  startIndex = MID(0, startIndex, bottom);
  int lastVisibleIndex = startIndex;

  bool cycle = false;

  while (delta) {
    index += sgn;

    if (cycle && index == startIndex) {
      break;
    }
    else if (index < 0) {
      if (!loop)
        break;
      index = bottom-sgn;
      cycle = true;
    }
    else if (index > bottom) {
      if (!loop)
        break;
      index = 0-sgn;
      cycle = true;
    }
    else {
      Widget* item = getChildByIndex(index);
      if (item &&
          !item->hasFlags(HIDDEN) &&
          // We can completely ignore separators from navigation
          // keys.
          !dynamic_cast<Separator*>(item)) {
        lastVisibleIndex = index;
        delta -= sgn;
      }
    }
  }
  return lastVisibleIndex;
}

} // namespace ui
