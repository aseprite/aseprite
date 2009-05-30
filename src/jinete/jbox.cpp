/* Jinete - a GUI library
 * Copyright (C) 2003-2009 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Based on code from GTK+ 2.1.2 (gtk+/gtk/gtkhbox.c) */

#include "jinete/jlist.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jwidget.h"

static bool box_msg_proc(JWidget widget, JMessage msg);
static void box_request_size(JWidget widget, int *w, int *h);
static void box_set_position(JWidget widget, JRect rect);

JWidget jbox_new(int align)
{
  JWidget widget = jwidget_new(JI_BOX);

  jwidget_add_hook(widget, JI_BOX, box_msg_proc, NULL);
  jwidget_set_align(widget, align);
  jwidget_init_theme(widget);

  return widget;
}

static bool box_msg_proc(JWidget widget, JMessage msg)
{
  switch(msg->type) {

    case JM_REQSIZE:
      box_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      box_set_position(widget, &msg->setpos.rect);
      return true;
  }

  return false;
}

static void box_request_size(JWidget widget, int *w, int *h)
{
#define GET_CHILD_SIZE(w, h)			\
  {						\
    if (widget->align() & JI_HOMOGENEOUS)	\
      *w = MAX(*w, req_##w);			\
    else					\
      *w += req_##w;				\
						\
    *h = MAX(*h, req_##h);			\
  }

#define FINAL_SIZE(w)					\
  {							\
    if (widget->align() & JI_HOMOGENEOUS)		\
      *w *= nvis_children;				\
							\
    *w += widget->child_spacing * (nvis_children-1);	\
  }

  int nvis_children;
  int req_w, req_h;
  JWidget child;
  JLink link;

  nvis_children = 0;
  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;
    if (!(child->flags & JI_HIDDEN))
      nvis_children++;
  }

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (child->flags & JI_HIDDEN)
      continue;

    jwidget_request_size(child, &req_w, &req_h);

    if (widget->align() & JI_HORIZONTAL) {
      GET_CHILD_SIZE(w, h);
    }
    else {
      GET_CHILD_SIZE(h, w);
    }
  }

  if (nvis_children > 0) {
    if (widget->align() & JI_HORIZONTAL) {
      FINAL_SIZE(w);
    }
    else {
      FINAL_SIZE(h);
    }
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void box_set_position(JWidget widget, JRect rect)
{
#define FIXUP(x, y, w, h, l, t, r, b)					\
  {									\
    if (nvis_children > 0) {						\
      if (widget->align() & JI_HOMOGENEOUS) {				\
	width = (jrect_##w(widget->rc)					\
		 - widget->border_width.l				\
		 - widget->border_width.r				\
		 - widget->child_spacing * (nvis_children - 1));	\
	extra = width / nvis_children;					\
      }									\
      else if (nexpand_children > 0) {					\
	width = jrect_##w(widget->rc) - req_##w;			\
	extra = width / nexpand_children;				\
      }									\
      else {								\
	width = 0;							\
	extra = 0;							\
      }									\
									\
      x = widget->rc->x##1 + widget->border_width.l;			\
      y = widget->rc->y##1 + widget->border_width.t;			\
      h = MAX(1, jrect_##h(widget->rc)					\
		 - widget->border_width.t				\
		 - widget->border_width.b);				\
									\
      JI_LIST_FOR_EACH(widget->children, link) {			\
	child = (JWidget)link->data;					\
									\
	if (!(child->flags & JI_HIDDEN)) {				\
	  if (widget->align() & JI_HOMOGENEOUS) {			\
	    if (nvis_children == 1)					\
	      child_width = width;					\
	    else							\
	      child_width = extra;					\
									\
	    --nvis_children;						\
	    width -= extra;						\
	  }								\
	  else {							\
	    jwidget_request_size(child, &req_w, &req_h);		\
									\
	    child_width = req_##w;					\
									\
	    if (jwidget_is_expansive(child)) {				\
	      if (nexpand_children == 1)				\
		child_width += width;					\
	      else							\
		child_width += extra;					\
									\
	      --nexpand_children;					\
	      width -= extra;						\
	    }								\
	  }								\
									\
	  w = MAX(1, child_width);					\
									\
	  if (widget->align() & JI_HORIZONTAL)				\
	    jrect_replace(&cpos, x, y, x+w, y+h);			\
	  else								\
	    jrect_replace(&cpos, y, x, y+h, x+w);			\
									\
	  jwidget_set_rect(child, &cpos);				\
									\
	  x += child_width + widget->child_spacing;			\
	}								\
      }									\
    }									\
  }

  struct jrect cpos;
  JWidget child;
  int nvis_children = 0;
  int nexpand_children = 0;
  int req_w, req_h;
  int child_width;
  JLink link;
  int width;
  int extra;
  int x, y, w, h;

  jrect_copy(widget->rc, rect);

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (!(child->flags & JI_HIDDEN)) {
      nvis_children++;
      if (jwidget_is_expansive(child))
	nexpand_children++;
    }
  }

  jwidget_request_size(widget, &req_w, &req_h);

  if (widget->align() & JI_HORIZONTAL) {
    FIXUP(x, y, w, h, l, t, r, b);
  }
  else {
    FIXUP(y, x, h, w, t, l, b, r);
  }
}
