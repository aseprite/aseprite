/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

/* Based on code from GTK+ 2.1.2 (gtk+/gtk/gtkhbox.c) */

#include "jinete/list.h"
#include "jinete/message.h"
#include "jinete/rect.h"
#include "jinete/widget.h"

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
      return TRUE;

    case JM_SETPOS:
      box_set_position(widget, &msg->setpos.rect);
      return TRUE;
  }

  return FALSE;
}

static void box_request_size(JWidget widget, int *w, int *h)
{
#define GET_CHILD_SIZE(w, h)			\
  {						\
    if (widget->align & JI_HOMOGENEOUS)		\
      *w = MAX(*w, req_##w);			\
    else					\
      *w += req_##w;				\
						\
    *h = MAX(*h, req_##h);			\
  }

#define FINAL_SIZE(w)					\
  {							\
    if (widget->align & JI_HOMOGENEOUS)			\
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
    if (jwidget_is_visible(child))
      nvis_children++;
  }

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (jwidget_is_hidden(child))
      continue;

    jwidget_request_size(child, &req_w, &req_h);

    if (widget->align & JI_HORIZONTAL) {
      GET_CHILD_SIZE(w, h);
    }
    else {
      GET_CHILD_SIZE(h, w);
    }
  }

  if (nvis_children > 0) {
    if (widget->align & JI_HORIZONTAL) {
      FINAL_SIZE(w);
    }
    else {
      FINAL_SIZE(h);
    }
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void box_set_position (JWidget widget, JRect rect)
{
#define FIXUP(x, y, w, h, l, t, r, b)					\
  {									\
    if (nvis_children > 0) {						\
      if (widget->align & JI_HOMOGENEOUS) {				\
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
      h = MAX (1, jrect_##h(widget->rc)				\
		  - widget->border_width.t				\
		  - widget->border_width.b);				\
									\
      JI_LIST_FOR_EACH(widget->children, link) {			\
	child = (JWidget)link->data;					\
									\
	if (jwidget_is_visible(child)) {				\
	  if (widget->align & JI_HOMOGENEOUS) {				\
	    if (nvis_children == 1)					\
	      child_width = width;					\
	    else							\
	      child_width = extra;					\
									\
	    nvis_children -= 1;						\
	    width -= extra;						\
	  }								\
	  else {							\
	    jwidget_request_size(child, &req_w, &req_h);		\
									\
	    child_width = req_##w;/* + child->padding * 2; */		\
									\
	    if (jwidget_is_expansive(child)) {			\
	      if (nexpand_children == 1)				\
		child_width += width;					\
	      else							\
		child_width += extra;					\
									\
	      nexpand_children -= 1;					\
	      width -= extra;						\
	    }								\
	  }								\
									\
	  w = MAX (1, child_width/*  - child->padding * 2 */);		\
									\
	  if (widget->align & JI_HORIZONTAL)				\
	    jrect_replace(&cpos, x, y, x+w, y+h);			\
	  else								\
	    jrect_replace(&cpos, y, x, y+h, x+w);			\
									\
	  jwidget_set_rect(child, &cpos);				\
	  /* 	    x = x + child->padding; */				\
	  /* 	  child->w = w; */					\
	  /* child->h = h; */						\
	  /* jwidget_size (child, w, h); */				\
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

    if (jwidget_is_visible(child)) {
      nvis_children++;
      if (jwidget_is_expansive(child))
	nexpand_children++;
    }
  }

  jwidget_request_size(widget, &req_w, &req_h);

  if (widget->align & JI_HORIZONTAL) {
    FIXUP(x, y, w, h, l, t, r, b);
  }
  else {
    FIXUP(y, x, h, w, t, l, b, r);
  }
}
