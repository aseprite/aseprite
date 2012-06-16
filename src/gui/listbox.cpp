// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/listbox.h"

#include "gui/list.h"
#include "gui/message.h"
#include "gui/preferred_size_event.h"
#include "gui/system.h"
#include "gui/theme.h"
#include "gui/view.h"

#include <allegro/keyboard.h>

using namespace gfx;

ListBox::ListBox()
  : Widget(JI_LISTBOX)
{
  setFocusStop(true);
  initTheme();
}

ListBox::Item::Item(const char* text)
  : Widget(JI_LISTITEM)
{
  setAlign(JI_LEFT | JI_MIDDLE);
  setText(text);
  initTheme();
}

ListBox::Item* ListBox::getSelectedChild()
{
  JLink link;
  JI_LIST_FOR_EACH(this->children, link) {
    if (((Item*)link->data)->isSelected())
      return (Item*)link->data;
  }
  return 0;
}

int ListBox::getSelectedIndex()
{
  JLink link;
  int i = 0;

  JI_LIST_FOR_EACH(this->children, link) {
    if (((Item*)link->data)->isSelected())
      return i;
    i++;
  }

  return -1;
}

void ListBox::selectChild(Item* item)
{
  Item* child;
  JLink link;

  JI_LIST_FOR_EACH(this->children, link) {
    child = (Item*)link->data;

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
  Item* child = reinterpret_cast<Item*>(jlist_nth_data(this->children, index));
  if (child)
    selectChild(child);
}

int ListBox::getItemsCount()
{
  return jlist_length(this->children);
}

/* setup the scroll to center the selected item in the viewport */
void ListBox::centerScroll()
{
  View* view = View::getView(this);
  Item* item = getSelectedChild();

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

    case JM_DIRTYCHILDREN:
      dirtyChildren();
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
          JWidget picked;

          if (view) {
            picked = view->getViewport()->pick(msg->mouse.x, msg->mouse.y);
          }
          else {
            picked = this->pick(msg->mouse.x, msg->mouse.y);
          }

          /* if the picked widget is a child of the list, select it */
          if (picked && hasChild(picked)) {
            if (Item* pickedItem = dynamic_cast<Item*>(picked))
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
      if (this->hasFocus() && !jlist_empty(this->children)) {
        int select = getSelectedIndex();
        View* view = View::getView(this);
        int bottom = MAX(0, jlist_length(this->children)-1);

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
  Size reqSize;
  JLink link;

  int w = 0, h = 0;

  JI_LIST_FOR_EACH(this->children, link) {
    reqSize = reinterpret_cast<Item*>(link->data)->getPreferredSize();

    w = MAX(w, reqSize.w);
    h += reqSize.h + ((link->next)? this->child_spacing: 0);
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
  JWidget child;
  JRect cpos;
  JLink link;

  jrect_copy(this->rc, rect);
  cpos = jwidget_get_child_rect(this);

  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;

    reqSize = child->getPreferredSize();

    cpos->y2 = cpos->y1+reqSize.h;
    jwidget_set_rect(child, cpos);

    cpos->y1 += jrect_h(child->rc) + this->child_spacing;
  }

  jrect_free(cpos);
}

void ListBox::dirtyChildren()
{
  View* view = View::getView(this);
  Item* child;
  JLink link;

  if (!view) {
    JI_LIST_FOR_EACH(this->children, link)
      reinterpret_cast<Item*>(link->data)->invalidate();
  }
  else {
    gfx::Rect vp = view->getViewportBounds();

    JI_LIST_FOR_EACH(this->children, link) {
      child = reinterpret_cast<Item*>(link->data);

      if (child->rc->y2 <= vp.y)
        continue;
      else if (child->rc->y1 >= vp.y+vp.h)
        break;

      child->invalidate();
    }
  }
}

bool ListBox::Item::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS: {
      JRect crect;
      JLink link;

      jrect_copy(this->rc, &msg->setpos.rect);
      crect = jwidget_get_child_rect(this);

      JI_LIST_FOR_EACH(this->children, link)
        jwidget_set_rect(reinterpret_cast<Widget*>(link->data), crect);

      jrect_free(crect);
      return true;
    }

    case JM_DRAW:
      this->getTheme()->draw_listitem(this, &msg->draw.rect);
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void ListBox::Item::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0, h = 0;
  Size maxSize;
  Size reqSize;
  JLink link;

  if (hasText()) {
    maxSize.w = jwidget_get_text_length(this);
    maxSize.h = jwidget_get_text_height(this);
  }
  else
    maxSize.w = maxSize.h = 0;

  JI_LIST_FOR_EACH(this->children, link) {
    reqSize = reinterpret_cast<Widget*>(link->data)->getPreferredSize();

    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  w = this->border_width.l + maxSize.w + this->border_width.r;
  h = this->border_width.t + maxSize.h + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}
