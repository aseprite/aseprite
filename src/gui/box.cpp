// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

/* Based on code from GTK+ 2.1.2 (gtk+/gtk/gtkhbox.c) */

#include "config.h"

#include "gfx/size.h"
#include "gui/box.h"
#include "gui/list.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/preferred_size_event.h"
#include "gui/theme.h"

using namespace gfx;

static bool box_msg_proc(JWidget widget, Message* msg);
static void box_request_size(JWidget widget, int *w, int *h);
static void box_set_position(JWidget widget, JRect rect);

Box::Box(int align)
  : Widget(JI_BOX)
{
  setAlign(align);
  initTheme();
}

bool Box::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      box_set_position(&msg->setpos.rect);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void Box::onPreferredSize(PreferredSizeEvent& ev)
{
#define GET_CHILD_SIZE(w, h)                    \
  {                                             \
    if (getAlign() & JI_HOMOGENEOUS)            \
      w = MAX(w, reqSize.w);                    \
    else                                        \
      w += reqSize.w;                           \
                                                \
    h = MAX(h, reqSize.h);                      \
  }

#define FINAL_SIZE(w)                                   \
  {                                                     \
    if (getAlign() & JI_HOMOGENEOUS)                    \
      w *= nvis_children;                               \
                                                        \
    w += child_spacing * (nvis_children-1);             \
  }

  int w, h, nvis_children;
  JWidget child;
  JLink link;

  nvis_children = 0;
  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;
    if (!(child->flags & JI_HIDDEN))
      nvis_children++;
  }

  w = h = 0;

  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;

    if (child->flags & JI_HIDDEN)
      continue;

    Size reqSize = child->getPreferredSize();

    if (this->getAlign() & JI_HORIZONTAL) {
      GET_CHILD_SIZE(w, h);
    }
    else {
      GET_CHILD_SIZE(h, w);
    }
  }

  if (nvis_children > 0) {
    if (this->getAlign() & JI_HORIZONTAL) {
      FINAL_SIZE(w);
    }
    else {
      FINAL_SIZE(h);
    }
  }

  w += border_width.l + border_width.r;
  h += border_width.t + border_width.b;

  ev.setPreferredSize(Size(w, h));
}

void Box::onPaint(PaintEvent& ev)
{
  getTheme()->paintBox(ev);
}

void Box::box_set_position(JRect rect)
{
#define FIXUP(x, y, w, h, l, t, r, b)                                   \
  {                                                                     \
    if (nvis_children > 0) {                                            \
      if (getAlign() & JI_HOMOGENEOUS) {                                \
        width = (jrect_##w(this->rc)                                    \
                 - this->border_width.l                                 \
                 - this->border_width.r                                 \
                 - this->child_spacing * (nvis_children - 1));          \
        extra = width / nvis_children;                                  \
      }                                                                 \
      else if (nexpand_children > 0) {                                  \
        width = jrect_##w(this->rc) - reqSize.w;                        \
        extra = width / nexpand_children;                               \
      }                                                                 \
      else {                                                            \
        width = 0;                                                      \
        extra = 0;                                                      \
      }                                                                 \
                                                                        \
      x = this->rc->x##1 + this->border_width.l;                        \
      y = this->rc->y##1 + this->border_width.t;                        \
      h = MAX(1, jrect_##h(this->rc)                                    \
                 - this->border_width.t                                 \
                 - this->border_width.b);                               \
                                                                        \
      JI_LIST_FOR_EACH(this->children, link) {                          \
        child = (JWidget)link->data;                                    \
                                                                        \
        if (!(child->flags & JI_HIDDEN)) {                              \
          if (this->getAlign() & JI_HOMOGENEOUS) {                      \
            if (nvis_children == 1)                                     \
              child_width = width;                                      \
            else                                                        \
              child_width = extra;                                      \
                                                                        \
            --nvis_children;                                            \
            width -= extra;                                             \
          }                                                             \
          else {                                                        \
            reqSize = child->getPreferredSize();                        \
                                                                        \
            child_width = reqSize.w;                                    \
                                                                        \
            if (jwidget_is_expansive(child)) {                          \
              if (nexpand_children == 1)                                \
                child_width += width;                                   \
              else                                                      \
                child_width += extra;                                   \
                                                                        \
              --nexpand_children;                                       \
              width -= extra;                                           \
            }                                                           \
          }                                                             \
                                                                        \
          w = MAX(1, child_width);                                      \
                                                                        \
          if (this->getAlign() & JI_HORIZONTAL)                         \
            jrect_replace(&cpos, x, y, x+w, y+h);                       \
          else                                                          \
            jrect_replace(&cpos, y, x, y+h, x+w);                       \
                                                                        \
          jwidget_set_rect(child, &cpos);                               \
                                                                        \
          x += child_width + this->child_spacing;                       \
        }                                                               \
      }                                                                 \
    }                                                                   \
  }

  struct jrect cpos;
  Widget* child;
  int nvis_children = 0;
  int nexpand_children = 0;
  int child_width;
  JLink link;
  int width;
  int extra;
  int x, y, w, h;

  jrect_copy(this->rc, rect);

  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;

    if (!(child->flags & JI_HIDDEN)) {
      nvis_children++;
      if (jwidget_is_expansive(child))
        nexpand_children++;
    }
  }

  Size reqSize = this->getPreferredSize();

  if (this->getAlign() & JI_HORIZONTAL) {
    FIXUP(x, y, w, h, l, t, r, b);
  }
  else {
    FIXUP(y, x, h, w, t, l, b, r);
  }
}
