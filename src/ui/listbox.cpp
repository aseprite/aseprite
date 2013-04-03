// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/listbox.h"

#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"

#include <allegro/keyboard.h>

using namespace gfx;

namespace ui {

ListBox::ListBox()
  : Widget(JI_LISTBOX)
{
  setFocusStop(true);
  initTheme();
}

ListItem* ListBox::getSelectedChild()
{
  UI_FOREACH_WIDGET(getChildren(), it) {
    ASSERT(dynamic_cast<ListItem*>(*it) != NULL);

    if (static_cast<ListItem*>(*it)->isSelected())
      return static_cast<ListItem*>(*it);
  }
  return 0;
}

int ListBox::getSelectedIndex()
{
  int i = 0;

  UI_FOREACH_WIDGET(getChildren(), it) {
    if (static_cast<ListItem*>(*it)->isSelected())
      return i;
    i++;
  }

  return -1;
}

void ListBox::selectChild(ListItem* item)
{
  UI_FOREACH_WIDGET(getChildren(), it) {
    ListItem* child = static_cast<ListItem*>(*it);

    if (child->isSelected()) {
      if (item && child == item)
        return;

      child->setSelected(false);
    }
  }

  if (item) {
    View* view = View::getView(this);

    item->setSelected(true);

    if (view) {
      gfx::Rect vp = view->getViewportBounds();
      gfx::Point scroll = view->getViewScroll();

      if (item->rc->y1 < vp.y)
          scroll.y = item->rc->y1 - this->rc->y1;
      else if (item->rc->y1 > vp.y + vp.h - jrect_h(item->rc))
        scroll.y = (item->rc->y1 - this->rc->y1
                    - vp.h + jrect_h(item->rc));

      view->setViewScroll(scroll);
    }
  }

  onChangeSelectedItem();
}

void ListBox::selectIndex(int index)
{
  const WidgetsList& children = getChildren();
  if (index < 0 || index >= (int)children.size())
    return;

  ListItem* child = static_cast<ListItem*>(children[index]);
  ASSERT(child);
  selectChild(child);
}

size_t ListBox::getItemsCount() const
{
  return getChildren().size();
}

/* setup the scroll to center the selected item in the viewport */
void ListBox::centerScroll()
{
  View* view = View::getView(this);
  ListItem* item = getSelectedChild();

  if (view && item) {
    gfx::Rect vp = view->getViewportBounds();
    gfx::Point scroll = view->getViewScroll();

    scroll.y = ((item->rc->y1 - this->rc->y1)
                - vp.h/2 + jrect_h(item->rc)/2);

    view->setViewScroll(scroll);
  }
}

bool ListBox::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      layoutListBox(&msg->setpos.rect);
      return true;

    case JM_DRAW:
      this->getTheme()->draw_listbox(this, &msg->draw.rect);
      return true;

    case JM_OPEN:
      centerScroll();
      break;

    case JM_BUTTONPRESSED:
      captureMouse();

    case JM_MOTION:
      if (hasCapture()) {
        int select = getSelectedIndex();
        View* view = View::getView(this);
        bool pick_item = true;

        if (view) {
          gfx::Rect vp = view->getViewportBounds();

          if (msg->mouse.y < vp.y) {
            int num = MAX(1, (vp.y - msg->mouse.y) / 8);
            selectIndex(select-num);
            pick_item = false;
          }
          else if (msg->mouse.y >= vp.y + vp.h) {
            int num = MAX(1, (msg->mouse.y - (vp.y+vp.h-1)) / 8);
            selectIndex(select+num);
            pick_item = false;
          }
        }

        if (pick_item) {
          Widget* picked;

          if (view) {
            picked = view->getViewport()->pick(msg->mouse.x, msg->mouse.y);
          }
          else {
            picked = this->pick(msg->mouse.x, msg->mouse.y);
          }

          /* if the picked widget is a child of the list, select it */
          if (picked && hasChild(picked)) {
            if (ListItem* pickedItem = dynamic_cast<ListItem*>(picked))
              selectChild(pickedItem);
          }
        }

        return true;
      }
      break;

    case JM_BUTTONRELEASED:
      releaseMouse();
      break;

    case JM_WHEEL: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->getViewScroll();
        scroll.y += (jmouse_z(1) - jmouse_z(0)) * jwidget_get_text_height(this)*3;
        view->setViewScroll(scroll);
      }
      break;
    }

    case JM_KEYPRESSED:
      if (hasFocus() && !getChildren().empty()) {
        int select = getSelectedIndex();
        View* view = View::getView(this);
        int bottom = MAX(0, getChildren().size()-1);

        switch (msg->key.scancode) {
          case KEY_UP:
            select--;
            break;
          case KEY_DOWN:
            select++;
            break;
          case KEY_HOME:
            select = 0;
            break;
          case KEY_END:
            select = bottom;
            break;
          case KEY_PGUP:
            if (view) {
              gfx::Rect vp = view->getViewportBounds();
              select -= vp.h / jwidget_get_text_height(this);
            }
            else
              select = 0;
            break;
          case KEY_PGDN:
            if (view) {
              gfx::Rect vp = view->getViewportBounds();
              select += vp.h / jwidget_get_text_height(this);
            }
            else
              select = bottom;
            break;
          case KEY_LEFT:
          case KEY_RIGHT:
            if (view) {
              gfx::Rect vp = view->getViewportBounds();
              gfx::Point scroll = view->getViewScroll();
              int sgn = (msg->key.scancode == KEY_LEFT) ? -1: 1;

              scroll.x += vp.w/2*sgn;

              view->setViewScroll(scroll);
            }
            break;
          default:
            return Widget::onProcessMessage(msg);
        }

        selectIndex(MID(0, select, bottom));
        return true;
      }
      break;

    case JM_DOUBLECLICK:
      onDoubleClickItem();
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void ListBox::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0, h = 0;

  UI_FOREACH_WIDGET_WITH_END(getChildren(), it, end) {
    Size reqSize = static_cast<ListItem*>(*it)->getPreferredSize();

    w = MAX(w, reqSize.w);
    h += reqSize.h + (it+1 != end ? this->child_spacing: 0);
  }

  w += this->border_width.l + this->border_width.r;
  h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}

void ListBox::onChangeSelectedItem()
{
  ChangeSelectedItem();
}

void ListBox::onDoubleClickItem()
{
  DoubleClickItem();
}

void ListBox::layoutListBox(JRect rect)
{
  Size reqSize;
  JRect cpos;

  jrect_copy(this->rc, rect);
  cpos = jwidget_get_child_rect(this);

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;

    reqSize = child->getPreferredSize();

    cpos->y2 = cpos->y1+reqSize.h;
    jwidget_set_rect(child, cpos);

    cpos->y1 += jrect_h(child->rc) + this->child_spacing;
  }

  jrect_free(cpos);
}

} // namespace ui
