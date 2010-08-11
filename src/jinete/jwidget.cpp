/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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

/* #define REPORT_SIGNALS */

#include "config.h"

#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <queue>
#ifdef REPORT_SIGNALS
#  include <cstdio>
#endif
#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include "jinete/jinete.h"
#include "jinete/jintern.h"
#include "Vaca/PreferredSizeEvent.h"

int ji_register_widget_type()
{
  static int type = JI_USER_WIDGET;
  return type++;
}

Widget::Widget(int type)
{
  _ji_add_widget(this);

  this->type = type;
  this->name = NULL;
  this->rc = jrect_new(0, 0, 0, 0);
  this->border_width.l = 0;
  this->border_width.t = 0;
  this->border_width.r = 0;
  this->border_width.b = 0;
  this->child_spacing = 0;
  this->flags = 0;
  this->emit_signals = 0;
  this->min_w = 0;
  this->min_h = 0;
  this->max_w = INT_MAX;
  this->max_h = INT_MAX;
  this->children = jlist_new();
  this->parent = NULL;
  this->theme = ji_get_theme();
  this->hooks = jlist_new();

  this->m_align = 0;
  this->m_text = "";
  this->m_font = this->theme ? this->theme->default_font: NULL;
  this->m_bg_color = -1;

  this->update_region = jregion_new(NULL, 0);

  this->theme_data[0] = NULL;
  this->theme_data[1] = NULL;
  this->theme_data[2] = NULL;
  this->theme_data[3] = NULL;

  this->user_data[0] = NULL;
  this->user_data[1] = NULL;
  this->user_data[2] = NULL;
  this->user_data[3] = NULL;

  m_preferredSize = NULL;
}

void jwidget_free(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);
  delete widget;
}

Widget::~Widget()
{
  JLink link, next;
  JMessage msg;

  /* send destroy message */
  msg = jmessage_new(JM_DESTROY);
  jwidget_send_message(this, msg);
  jmessage_free(msg);

  /* break relationship with the manager */
  jmanager_free_widget(this);
  jmanager_remove_messages_for(this);
  jmanager_remove_msg_filter_for(this);

  /* remove from parent */
  if (this->parent)
    jwidget_remove_child(this->parent, this);

  /* remove children */
  JI_LIST_FOR_EACH_SAFE(this->children, link, next)
    jwidget_free(reinterpret_cast<JWidget>(link->data));
  jlist_free(this->children);

  /* destroy the update region */
  if (this->update_region)
    jregion_free(this->update_region);

  /* destroy the name */
  if (this->name)
    jfree(this->name);

  /* destroy widget position */
  if (this->rc)
    jrect_free(this->rc);

  /* destroy hooks */
  JI_LIST_FOR_EACH(this->hooks, link)
    jhook_free(reinterpret_cast<JHook>(link->data));
  jlist_free(this->hooks);

  // Delete the preferred size
  delete m_preferredSize;

  /* low level free */
  _ji_remove_widget(this);
}

void jwidget_free_deferred(JWidget widget)
{
  JMessage msg;

  ASSERT_VALID_WIDGET(widget);

  msg = jmessage_new(JM_DEFERREDFREE);
  msg->deffree.widget_to_free = widget;
  /* TODO use the manager of 'widget' */
  jmessage_add_dest(msg, ji_get_default_manager());
  jmanager_enqueue_message(msg);
}

void jwidget_init_theme(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  if (widget->theme) {
    widget->theme->init_widget(widget);

    if (!(widget->flags & JI_INITIALIZED))
      widget->flags |= JI_INITIALIZED;

    jwidget_emit_signal(widget, JI_SIGNAL_INIT_THEME);
  }
}

/**********************************************************************/
/* HOOKS */

/**
 * Adds a new hook for the widget.
 *
 * @see jhook
 */
void jwidget_add_hook(JWidget widget, int type,
		      JMessageFunc msg_proc, void *data)
{
  JHook hook;

  ASSERT_VALID_WIDGET(widget);

  hook = jhook_new();
  hook->type = type;
  hook->msg_proc = msg_proc;
  hook->data = data;

  jlist_prepend(widget->hooks, hook);
}

/**
 * Returns the hook of the specified type.
 */
JHook jwidget_get_hook(JWidget widget, int type)
{
  JLink link;
  ASSERT_VALID_WIDGET(widget);

  JI_LIST_FOR_EACH(widget->hooks, link) {
    if (((JHook)link->data)->type == type)
      return ((JHook)link->data);
  }
  return NULL;
}

/**
 * Returns the data associated to the specified hook.
 */
void *jwidget_get_data(JWidget widget, int type)
{
  register JLink link;
  ASSERT_VALID_WIDGET(widget);

  JI_LIST_FOR_EACH(widget->hooks, link) {
    if (((JHook)link->data)->type == type)
      return ((JHook)link->data)->data;
  }

  return NULL;
}

/**********************************************************************/
/* main properties */

int Widget::getType()
{
  return this->type;
}

const char *Widget::getName()
{
  return this->name;
}

int Widget::getAlign() const
{
  return m_align;
}

void Widget::setName(const char *name)
{
  if (this->name)
    jfree(this->name);

  this->name = name ? jstrdup(name) : NULL;
}

void Widget::setAlign(int align)
{
  m_align = align;
}

int Widget::getTextInt() const
{
  return ustrtol(m_text.c_str(), NULL, 10);
}

double Widget::getTextDouble() const
{
  return ustrtod(m_text.c_str(), NULL);
}

void Widget::setText(const char *text)
{
  this->setTextQuiet(text);

  jwidget_emit_signal(this, JI_SIGNAL_SET_TEXT);
  dirty();
}

void Widget::setTextf(const char *format, ...)
{
  char buf[4096];

  // formatted string
  if (format) {
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);
  }
  // empty string
  else {
    ustrcpy(buf, empty_string);
  }

  setText(buf);
}

void Widget::setTextQuiet(const char *text)
{
  if (text) {
    m_text = text;
    flags &= ~JI_NOTEXT;
  }
  else {
    m_text.clear();
    flags |= JI_NOTEXT;
  }
}

FONT *Widget::getFont()
{
  return m_font;
}

void Widget::setFont(FONT* f)
{
  m_font = f;

  jwidget_emit_signal(this, JI_SIGNAL_SET_FONT);
  dirty();
}

/**********************************************************************/
/* behavior properties */

void jwidget_magnetic(JWidget widget, bool state)
{
  ASSERT_VALID_WIDGET(widget);

  if (state)
    widget->flags |= JI_MAGNETIC;
  else
    widget->flags &= ~JI_MAGNETIC;
}

void jwidget_expansive(JWidget widget, bool state)
{
  ASSERT_VALID_WIDGET(widget);

  if (state)
    widget->flags |= JI_EXPANSIVE;
  else
    widget->flags &= ~JI_EXPANSIVE;
}

void jwidget_decorative(JWidget widget, bool state)
{
  ASSERT_VALID_WIDGET(widget);

  if (state)
    widget->flags |= JI_DECORATIVE;
  else
    widget->flags &= ~JI_DECORATIVE;
}

void jwidget_focusrest(JWidget widget, bool state)
{
  ASSERT_VALID_WIDGET(widget);

  if (state)
    widget->flags |= JI_FOCUSREST;
  else
    widget->flags &= ~JI_FOCUSREST;
}

bool jwidget_is_magnetic(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return (widget->flags & JI_MAGNETIC) ? true: false;
}

bool jwidget_is_expansive(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return (widget->flags & JI_EXPANSIVE) ? true: false;
}

bool jwidget_is_decorative(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return (widget->flags & JI_DECORATIVE) ? true: false;
}

bool jwidget_is_focusrest(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return (widget->flags & JI_FOCUSREST) ? true: false;
}

/**********************************************************************/
/* status properties */

void jwidget_dirty(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);
  jwidget_invalidate(widget);
}

void Widget::setVisible(bool state)
{
  if (state) {
    if (this->flags & JI_HIDDEN) {
      this->flags &= ~JI_HIDDEN;

      jwidget_dirty(this);
      jwidget_emit_signal(this, JI_SIGNAL_SHOW);
    }
  }
  else {
    if (!(this->flags & JI_HIDDEN)) {
      jmanager_free_widget(this); // Free from manager

      this->flags |= JI_HIDDEN;
      jwidget_emit_signal(this, JI_SIGNAL_HIDE);
    }
  }
}

void Widget::setEnabled(bool state)
{
  if (state) {
    if (this->flags & JI_DISABLED) {
      this->flags &= ~JI_DISABLED;
      jwidget_dirty(this);

      jwidget_emit_signal(this, JI_SIGNAL_ENABLE);
    }
  }
  else {
    if (!(this->flags & JI_DISABLED)) {
      jmanager_free_widget(this); // Free from the manager

      this->flags |= JI_DISABLED;
      jwidget_dirty(this);

      jwidget_emit_signal(this, JI_SIGNAL_DISABLE);
    }
  }
}

void Widget::setSelected(bool state)
{
  if (state) {
    if (!(this->flags & JI_SELECTED)) {
      this->flags |= JI_SELECTED;
      jwidget_dirty(this);

      jwidget_emit_signal(this, JI_SIGNAL_SELECT);
    }
  }
  else {
    if (this->flags & JI_SELECTED) {
      this->flags &= ~JI_SELECTED;
      jwidget_dirty(this);

      jwidget_emit_signal(this, JI_SIGNAL_DESELECT);
    }
  }
}

bool Widget::isVisible() const
{
  const Widget* widget = this;

  do {
    if (widget->flags & JI_HIDDEN)
      return false;

    widget = widget->parent;
  } while (widget);

  return true;
}

bool Widget::isEnabled() const
{
  const Widget* widget = this;

  do {
    if (widget->flags & JI_DISABLED)
      return false;

    widget = widget->parent;
  } while (widget);

  return true;
}

bool Widget::isSelected() const
{
  return (this->flags & JI_SELECTED) ? true: false;
}

/**********************************************************************/
/* children handle */

void jwidget_add_child(JWidget widget, JWidget child)
{
  ASSERT_VALID_WIDGET(widget);
  ASSERT_VALID_WIDGET(child);

  jlist_append(widget->children, child);
  child->parent = widget;

  jwidget_emit_signal(widget, JI_SIGNAL_ADD_CHILD);
}

void jwidget_add_children(JWidget widget, ...)
{
  JWidget child;
  va_list ap;

  ASSERT_VALID_WIDGET(widget);

  va_start(ap, widget);

  while ((child=va_arg(ap, JWidget)) != NULL)
    jwidget_add_child(widget, child);

  va_end(ap);
}

void jwidget_remove_child(JWidget widget, JWidget child)
{
  ASSERT_VALID_WIDGET(widget);
  ASSERT_VALID_WIDGET(child);

  jlist_remove(widget->children, child);
  child->parent = NULL;
}

void jwidget_replace_child(JWidget widget, JWidget old_child, JWidget new_child)
{
  JLink before;

  ASSERT_VALID_WIDGET(widget);
  ASSERT_VALID_WIDGET(old_child);
  ASSERT_VALID_WIDGET(new_child);

  before = jlist_find(widget->children, old_child);
  if (!before)
    return;
  before = before->next;

  jwidget_remove_child(widget, old_child);

  jlist_insert_before(widget->children, before, new_child);
  new_child->parent = widget;

  jwidget_emit_signal(widget, JI_SIGNAL_ADD_CHILD);
}

/**********************************************************************/
/* parents and children */

JWidget jwidget_get_parent(JWidget widget)
{ return widget->getParent(); }

JWidget jwidget_get_manager(JWidget widget)
{ return widget->getManager(); }

JList jwidget_get_parents(JWidget widget, bool ascendant)
{ return widget->getParents(ascendant); }

JList jwidget_get_children(JWidget widget)
{ return widget->getChildren(); }

JWidget jwidget_pick(JWidget widget, int x, int y)
{ return widget->pick(x, y); }

bool jwidget_has_child(JWidget widget, JWidget child)
{ return widget->hasChild(child); }

Widget* Widget::getRoot()
{
  Widget* widget = this;

  while (widget) {
    if (widget->type == JI_FRAME)
      return widget;

    widget = widget->parent;
  }

  return NULL;
}

Widget* Widget::getParent()
{
  return this->parent;
}

Widget* Widget::getManager()
{
  Widget* widget = this;

  while (widget) {
    if (widget->type == JI_MANAGER)
      return widget;

    widget = widget->parent;
  }

  return ji_get_default_manager();
}

/* returns a list of parents (you must free the list), if "ascendant"
   is true the list is build from child to parents, else the list is
   from parent to children */
JList Widget::getParents(bool ascendant)
{
  JList list = jlist_new();

  for (Widget* widget=this; widget; widget=widget->parent) {
    // append parents in tail
    if (ascendant)
      jlist_append(list, widget);
    // append parents in head
    else
      jlist_prepend(list, widget);
  }

  return list;
}

/* returns a list of children (you must free the list) */
JList Widget::getChildren()
{
  return jlist_copy(this->children);
}

Widget* Widget::pick(int x, int y)
{
  Widget* inside, *picked = NULL;
  JLink link;

  if (!(this->flags & JI_HIDDEN) &&   /* is visible */
      jrect_point_in(this->rc, x, y)) { /* the point is inside the bounds */
    picked = this;

    JI_LIST_FOR_EACH(this->children, link) {
      inside = reinterpret_cast<Widget*>(link->data)->pick(x, y);
      if (inside) {
	picked = inside;
	break;
      }
    }
  }

  return picked;
}

bool Widget::hasChild(Widget* child)
{
  ASSERT_VALID_WIDGET(child);

  return jlist_find(this->children, child) != this->children->end ? true: false;
}

Widget* Widget::findChild(const char* name)
{
  Widget* child;
  JLink link;

  JI_LIST_FOR_EACH(this->children, link) {
    child = (Widget*)link->data;
    if (child->name != NULL && strcmp(child->name, name) == 0)
      return child;
  }

  JI_LIST_FOR_EACH(this->children, link) {
    if ((child = ((Widget*)link->data)->findChild(name)))
      return child;
  }

  return 0;
}

/**
 * Returns a widget in the same window that is located "sibling".
 */
Widget* Widget::findSibling(const char* name)
{
  return getRoot()->findChild(name);
}

/**********************************************************************/
/* position and geometry */

Rect Widget::getBounds() const
{
  return Rect(rc->x1, rc->y1, jrect_w(rc), jrect_h(rc));
}

void Widget::setBounds(const Rect& rc)
{
  jrect jrc = { rc.x, rc.y, rc.x+rc.w, rc.y+rc.h };
  jwidget_set_rect(this, &jrc);
}

void jwidget_relayout(JWidget widget)
{
  jwidget_set_rect(widget, widget->rc);
  jwidget_dirty(widget);
}

/* gets the position of the widget */
JRect jwidget_get_rect(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return jrect_new_copy(widget->rc);
}

/* gets the position for children of the widget */
JRect jwidget_get_child_rect(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return jrect_new(widget->rc->x1 + widget->border_width.l,
		   widget->rc->y1 + widget->border_width.t,
		   widget->rc->x2 - widget->border_width.r,
		   widget->rc->y2 - widget->border_width.b);
}

JRegion jwidget_get_region(JWidget widget)
{
  JRegion region;

  ASSERT_VALID_WIDGET(widget);

  if (widget->type == JI_FRAME)
    region = widget->theme->get_window_mask(widget);
  else
    region = jregion_new(widget->rc, 1);

  return region;
}

/* gets the region to be able to draw in */
JRegion jwidget_get_drawable_region(JWidget widget, int flags)
{
  JWidget window, manager, view, child;
  JRegion region, reg1, reg2, reg3;
  JList windows_list;
  JLink link;
  JRect cpos;

  ASSERT_VALID_WIDGET(widget);

  region = jwidget_get_region(widget);

  /* cut the top windows areas */
  if (flags & JI_GDR_CUTTOPWINDOWS) {
    window = widget->getRoot();
    manager = window ? jwidget_get_manager(window): NULL;

    while (manager) {
      windows_list = manager->children;
      link = jlist_find(windows_list, window);

      if (!jlist_empty(windows_list) &&
	  window != jlist_first(windows_list)->data &&
	  link != windows_list->end) {
	/* subtract the rectangles */
	for (link=link->prev; link != windows_list->end; link=link->prev) {
	  reg1 = jwidget_get_region(reinterpret_cast<JWidget>(link->data));
	  jregion_subtract(region, region, reg1);
	  jregion_free(reg1);
	}
      }

      window = manager->getRoot();
      manager = window ? window->getManager(): NULL;
    }
  }

  /* clip the areas where are children */
  if (!(flags & JI_GDR_USECHILDAREA) && !jlist_empty(widget->children)) {
    cpos = jwidget_get_child_rect(widget);
    reg1 = jregion_new(NULL, 0);
    reg2 = jregion_new(cpos, 1);
    JI_LIST_FOR_EACH(widget->children, link) {
      child = reinterpret_cast<JWidget>(link->data);
      if (child->isVisible()) {
	reg3 = jwidget_get_region(child);
	if (child->flags & JI_DECORATIVE) {
	  jregion_reset(reg1, widget->rc);
	  jregion_intersect(reg1, reg1, reg3);
	}
	else {
	  jregion_intersect(reg1, reg2, reg3);
	}
	jregion_subtract(region, region, reg1);
	jregion_free(reg3);
      }
    }
    jregion_free(reg1);
    jregion_free(reg2);
    jrect_free(cpos);
  }

  /* intersect with the parent area */
  if (!(widget->flags & JI_DECORATIVE)) {
    JWidget parent = widget->parent;

    reg1 = jregion_new(NULL, 0);

    while (parent) {
      cpos = jwidget_get_child_rect(parent);
      jregion_reset(reg1, cpos);
      jregion_intersect(region, region, reg1);
      jrect_free(cpos);

      parent = parent->parent;
    }

    jregion_free(reg1);
  }
  else {
    JWidget parent = widget->parent;

    if (parent) {
      cpos = jwidget_get_rect(parent);
      reg1 = jregion_new(cpos, 1);
      jregion_intersect(region, region, reg1);
      jregion_free(reg1);
      jrect_free(cpos);
    }
  }

  /* limit to the manager area */
  window = widget->getRoot();
  manager = window ? window->getManager(): NULL;

  while (manager) {
    view = jwidget_get_view(manager);
    if (view)
      cpos = jview_get_viewport_position(view);
    else
      cpos = jwidget_get_child_rect(manager);
/*     if (!manager->parent) */
/*       cpos = jwidget_get_rect(manager); */
/*     else */
/*       cpos = jwidget_get_child_rect(manager->parent); */

    reg1 = jregion_new(cpos, 1);
    jregion_intersect(region, region, reg1);
    jregion_free(reg1);
    jrect_free(cpos);

    window = manager->getRoot();
    manager = window ? window->getManager(): NULL;
  }

  /* return the region */
  return region;
}

int jwidget_get_bg_color(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return widget->getBgColor();
}

JTheme jwidget_get_theme(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return widget->theme;
}

int jwidget_get_text_length(JWidget widget)
{
#if 1
  return ji_font_text_len(widget->getFont(), widget->getText());
#else  /* use cached text size */
  return widget->text_size_pix;
#endif
}

int jwidget_get_text_height(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  return text_height(widget->getFont());
}

void jwidget_get_texticon_info(JWidget widget,
			       JRect box, JRect text, JRect icon,
			       int icon_align, int icon_w, int icon_h)
{
#define SETRECT(r)				\
  if (r) {					\
    r->x1 = r##_x;				\
    r->y1 = r##_y;				\
    r->x2 = r##_x+r##_w;			\
    r->y2 = r##_y+r##_h;			\
  }

  int box_x, box_y, box_w, box_h, icon_x, icon_y;
  int text_x, text_y, text_w, text_h;

  ASSERT_VALID_WIDGET(widget);

  text_x = text_y = 0;

  /* size of the text */
  if (widget->hasText()) {
    text_w = jwidget_get_text_length(widget);
    text_h = jwidget_get_text_height(widget);
  }
  else {
    text_w = text_h = 0;
  }

  /* box size */
  if (icon_align & JI_CENTER) {	  /* with the icon in the center */
    if (icon_align & JI_MIDDLE) { /* with the icon inside the text */
      box_w = MAX(icon_w, text_w);
      box_h = MAX(icon_h, text_h);
    }
    /* with the icon in the top or bottom */
    else {
      box_w = MAX(icon_w, text_w);
      box_h = icon_h + (widget->hasText() ? widget->child_spacing: 0) + text_h;
    }
  }
  /* with the icon in left or right that doesn't care by now */
  else {
    box_w = icon_w + (widget->hasText() ? widget->child_spacing: 0) + text_w;
    box_h = MAX(icon_h, text_h);
  }

  /* box position */
  if (widget->getAlign() & JI_RIGHT)
    box_x = widget->rc->x2 - box_w - widget->border_width.r;
  else if (widget->getAlign() & JI_CENTER)
    box_x = (widget->rc->x1+widget->rc->x2)/2 - box_w/2;
  else
    box_x = widget->rc->x1 + widget->border_width.l;

  if (widget->getAlign() & JI_BOTTOM)
    box_y = widget->rc->y2 - box_h - widget->border_width.b;
  else if (widget->getAlign() & JI_MIDDLE)
    box_y = (widget->rc->y1+widget->rc->y2)/2 - box_h/2;
  else
    box_y = widget->rc->y1 + widget->border_width.t;

  /* with text */
  if (widget->hasText()) {
    /* text/icon X position */
    if (icon_align & JI_RIGHT) {
      text_x = box_x;
      icon_x = box_x + box_w - icon_w;
    }
    else if (icon_align & JI_CENTER) {
      text_x = box_x + box_w/2 - text_w/2;
      icon_x = box_x + box_w/2 - icon_w/2;
    }
    else {
      text_x = box_x + box_w - text_w;
      icon_x = box_x;
    }

    /* text Y position */
    if (icon_align & JI_BOTTOM) {
      text_y = box_y;
      icon_y = box_y + box_h - icon_h;
    }
    else if (icon_align & JI_MIDDLE) {
      text_y = box_y + box_h/2 - text_h/2;
      icon_y = box_y + box_h/2 - icon_h/2;
    }
    else {
      text_y = box_y + box_h - text_h;
      icon_y = box_y;
    }
  }
  /* without text */
  else {
    /* icon X/Y position */
    icon_x = box_x;
    icon_y = box_y;
  }

  SETRECT(box);
  SETRECT(text);
  SETRECT(icon);
}

void jwidget_noborders(JWidget widget)
{
  widget->border_width.l = 0;
  widget->border_width.t = 0;
  widget->border_width.r = 0;
  widget->border_width.b = 0;
  widget->child_spacing = 0;

  jwidget_dirty(widget);
}

void jwidget_set_border(JWidget widget, int value)
{
  ASSERT_VALID_WIDGET(widget);

  widget->border_width.l = value;
  widget->border_width.t = value;
  widget->border_width.r = value;
  widget->border_width.b = value;

  jwidget_dirty(widget);
}

void jwidget_set_border(JWidget widget, int l, int t, int r, int b)
{
  ASSERT_VALID_WIDGET(widget);

  widget->border_width.l = l;
  widget->border_width.t = t;
  widget->border_width.r = r;
  widget->border_width.b = b;

  jwidget_dirty(widget);
}

void jwidget_set_rect(JWidget widget, JRect rect)
{
  JMessage msg;

  ASSERT_VALID_WIDGET(widget);

  msg = jmessage_new(JM_SETPOS);
  jrect_copy(&msg->setpos.rect, rect);
  jwidget_send_message(widget, msg);
  jmessage_free(msg);
}

void jwidget_set_min_size(JWidget widget, int w, int h)
{
  ASSERT_VALID_WIDGET(widget);

  widget->min_w = w;
  widget->min_h = h;
}

void jwidget_set_max_size(JWidget widget, int w, int h)
{
  ASSERT_VALID_WIDGET(widget);

  widget->max_w = w;
  widget->max_h = h;
}

void jwidget_set_bg_color(JWidget widget, int color)
{
  ASSERT_VALID_WIDGET(widget);

  widget->setBgColor(color);
}

void jwidget_set_theme(JWidget widget, JTheme theme)
{
  ASSERT_VALID_WIDGET(widget);

  widget->theme = theme;
  /* TODO mmhhh... maybe some JStyle in JWidget should be great */
  widget->setFont(widget->theme ? widget->theme->default_font: NULL);
}

/**********************************************************************/
/* drawing methods */

void jwidget_flush_redraw(JWidget widget)
{
  std::queue<Widget*> processing;
  int c, nrects;
  JMessage msg;
  JLink link;
  JRect rc;

  processing.push(widget);

  while (!processing.empty()) {
    widget = processing.front();
    processing.pop();

    ASSERT_VALID_WIDGET(widget);

    // If the widget is hidden
    if (!widget->isVisible())
      continue;

    JI_LIST_FOR_EACH(widget->children, link)
      processing.push((Widget*)link->data);

    nrects = JI_REGION_NUM_RECTS(widget->update_region);
    if (nrects > 0) {
      /* get areas to draw */
      JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
      jregion_intersect(widget->update_region,
			widget->update_region, region);
      jregion_free(region);

      nrects = JI_REGION_NUM_RECTS(widget->update_region);

      /* draw the widget */
      for (c=0, rc=JI_REGION_RECTS(widget->update_region);
	   c<nrects;
	   c++, rc++) {
	/* create the draw message */
	msg = jmessage_new(JM_DRAW);
	msg->draw.count = nrects-1 - c;
	msg->draw.rect = *rc;
	jmessage_add_dest(msg, widget);

	/* enqueue the draw message */
	jmanager_enqueue_message(msg);
      }

      jregion_empty(widget->update_region);
    }
  }
}

void jwidget_invalidate(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  if (widget->isVisible()) {
    JRegion reg1 = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
    JLink link;

    jregion_copy(widget->update_region, reg1);
    jregion_free(reg1);

    JI_LIST_FOR_EACH(widget->children, link)
      jwidget_invalidate(reinterpret_cast<JWidget>(link->data));
  }
}

void jwidget_invalidate_rect(JWidget widget, const JRect rect)
{
  ASSERT_VALID_WIDGET(widget);

  if (widget->isVisible()) {
    JRegion reg1 = jregion_new(rect, 1);
    jwidget_invalidate_region(widget, reg1);
    jregion_free(reg1);
  }
}

void jwidget_invalidate_region(JWidget widget, const JRegion region)
{
  ASSERT_VALID_WIDGET(widget);

  if (widget->isVisible() &&
      jregion_rect_in(region, widget->rc) != JI_RGNOUT) {
    JRegion reg1 = jregion_new(NULL, 0);
    JRegion reg2 = jwidget_get_drawable_region(widget,
					       JI_GDR_CUTTOPWINDOWS);
    JLink link;

    jregion_union(reg1, widget->update_region, region);
    jregion_intersect(widget->update_region, reg1, reg2);
    jregion_free(reg2);

    jregion_subtract(reg1, region, widget->update_region);

    JI_LIST_FOR_EACH(widget->children, link)
      jwidget_invalidate_region(reinterpret_cast<JWidget>(link->data), reg1);

    jregion_free(reg1);
  }
}

void jwidget_scroll(JWidget widget, JRegion region, int dx, int dy)
{
  if (dx != 0 || dy != 0) {
    JRegion reg2 = jregion_new(NULL, 0);
    
    jregion_copy(reg2, region);
    jregion_translate(reg2, dx, dy);
    jregion_intersect(reg2, reg2, region);

    jregion_translate(reg2, -dx, -dy);

    jmouse_hide();
    ji_move_region(reg2, dx, dy);
    jmouse_show();

    jregion_translate(reg2, dx, dy);

    jregion_union(widget->update_region, widget->update_region, region);
    jregion_subtract(widget->update_region, widget->update_region, reg2);

    // Generate the JM_DRAW messages for the widget's update_region
    jwidget_flush_redraw(widget);

    jregion_free(reg2);
  }
}

/**********************************************************************/
/* signal handle */

void jwidget_signal_on(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  widget->emit_signals--;
}

void jwidget_signal_off(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  widget->emit_signals++;
}

bool jwidget_emit_signal(JWidget widget, int signal_num)
{
  ASSERT_VALID_WIDGET(widget);

  if (!widget->emit_signals) {
    JMessage msg;
    bool ret;

#ifdef REPORT_SIGNALS
    printf("Signal: %d (%d)\n", signal_num, widget->id);
#endif

    msg = jmessage_new(JM_SIGNAL);
    msg->signal.num = signal_num;
    msg->signal.from = widget;

    ret = jwidget_send_message(widget, msg);

    /* send the signal to the window too */
    if (!ret && widget->type != JI_FRAME) {
      Widget* window = widget->getRoot();
      if (window)
	ret = window->sendMessage(msg);
    }

    jmessage_free(msg);
    return ret;
  }
  else
    return false;
}

/**********************************************************************/
/* manager handler */

bool jwidget_send_message(JWidget widget, JMessage msg)
{ return widget->sendMessage(msg); }

void jwidget_close_window(JWidget widget)
{ widget->closeWindow(); }

bool Widget::sendMessage(JMessage msg)
{
  bool done = false;
  JHook hook;
  JLink link;

  ASSERT(msg != NULL);

  JI_LIST_FOR_EACH(this->hooks, link) {
    hook = reinterpret_cast<JHook>(link->data);
    if (hook->msg_proc) {
      done = (*hook->msg_proc)(this, msg);
      if (done)
	break;
    }
  }

  if (!done)
    done = this->onProcessMessage(msg);

  return done;
}

void Widget::closeWindow()
{
  if (Frame* frame = static_cast<Frame*>(getRoot()))
    frame->closeWindow(this);
}

// ===============================================================
// SIZE & POSITION
// ===============================================================

/**
   Returns the preferred size of the Widget.

   It checks if the preferred size is static (it means when it was
   set through #setPreferredSize before) or if it is dynamic (this is
   the default and is when the #onPreferredSize is used to determined
   the preferred size).

   In another words, if you do not use #setPreferredSize to set a
   <em>static preferred size</em> for the widget then #onPreferredSize
   will be used to calculate it.

   @see setPreferredSize, onPreferredSize, #getPreferredSize(const Size &)
*/
Size Widget::getPreferredSize()
{
  if (m_preferredSize != NULL)
    return *m_preferredSize;
  else {
    PreferredSizeEvent ev(this, Size(0, 0));
    onPreferredSize(ev);

    Size sz(ev.getPreferredSize());
    sz.w = MID(this->min_w, sz.w, this->max_w);
    sz.h = MID(this->min_h, sz.h, this->max_h);
    return sz;
  }
}

/**
   Returns the preferred size trying to fit in the specified size.
   Remember that if you use #setPreferredSize this routine will
   return the static size which you specified manually.

   @param fitIn
       This can have both attributes (width and height) in
       zero, which means that it'll behave same as #getPreferredSize().
       If the width is great than zero the #onPreferredSize will try to
       fit in that width (this is useful to fit @link Vaca::Label Label@endlink
       or @link Vaca::Edit Edit@endlink controls in a specified width and
       calculate the height it could occupy).

   @see getPreferredSize
*/
Size Widget::getPreferredSize(const Size& fitIn)
{
  if (m_preferredSize != NULL)
    return *m_preferredSize;
  else {
    PreferredSizeEvent ev(this, fitIn);
    onPreferredSize(ev);

    Size sz(ev.getPreferredSize());
    sz.w = MID(this->min_w, sz.w, this->max_w);
    sz.h = MID(this->min_h, sz.h, this->max_h);
    return sz;
  }
}

/**
   Sets a fixed preferred size specified by the user.
   Widget::getPreferredSize() will return this value if it's setted.
*/
void Widget::setPreferredSize(const Size& fixedSize)
{
  delete m_preferredSize;
  m_preferredSize = new Size(fixedSize);
}

void Widget::setPreferredSize(int fixedWidth, int fixedHeight)
{
  setPreferredSize(Size(fixedWidth, fixedHeight));
}

// ===============================================================
// FOCUS & MOUSE
// ===============================================================

void Widget::requestFocus()
{
  jmanager_set_focus(this);
}

void Widget::releaseFocus()
{
  if (hasFocus())
    jmanager_free_focus();
}

/**
 * Captures the mouse to send all the future mouse messsages to the
 * specified widget (included the JM_MOTION and JM_SETCURSOR).
 */
void Widget::captureMouse()
{
  if (!jmanager_get_capture()) {
    jmanager_set_capture(this);

#ifdef ALLEGRO_WINDOWS
    SetCapture(win_get_window());
#endif
  }
}

/**
 * Releases the capture of the mouse events.
 */
void Widget::releaseMouse()
{
  if (jmanager_get_capture() == this) {
    jmanager_free_capture();

#ifdef ALLEGRO_WINDOWS
    ::ReleaseCapture();		// Win32 API
#endif
  }
}

bool Widget::hasFocus()
{
  return (this->flags & JI_HASFOCUS) ? true: false;
}

bool Widget::hasMouse()
{
  return (this->flags & JI_HASMOUSE) ? true: false;
}

bool Widget::hasMouseOver()
{
  return (this == this->pick(jmouse_x(0), jmouse_y(0)));
}

bool Widget::hasCapture()
{
  return (this->flags & JI_HASCAPTURE) ? true: false;
}

/**********************************************************************/
/* miscellaneous */

JWidget jwidget_find_name(JWidget widget, const char *name)
{
  ASSERT_VALID_WIDGET(widget);
  return widget->findChild(name);
}

bool jwidget_check_underscored(JWidget widget, int scancode)
{
  int c, ascii;

  ASSERT_VALID_WIDGET(widget);

  ascii = 0;
  if (scancode >= KEY_0 && scancode <= KEY_9)
    ascii = '0' + (scancode - KEY_0);
  else if (scancode >= KEY_A && scancode <= KEY_Z)
    ascii = 'a' + (scancode - KEY_A);
  else
    return false;

  if (widget->hasText()) {
    const char* text = widget->getText();

    for (c=0; text[c]; c++)
      if ((text[c] == '&') && (text[c+1] != '&'))
	if (ascii == tolower(text[c+1]))
	  return true;
  }

  return false;
}

/**********************************************************************/
/* widget message procedure */

bool Widget::onProcessMessage(JMessage msg)
{
  JWidget widget = this;

  ASSERT(msg != NULL);
  ASSERT_VALID_WIDGET(widget);

  switch (msg->type) {

    case JM_OPEN:
    case JM_CLOSE:
    case JM_WINMOVE: {
      JLink link;

      /* broadcast the message to the children */
      JI_LIST_FOR_EACH(widget->children, link)
	jwidget_send_message(reinterpret_cast<JWidget>(link->data), msg);
      break;
    }

    case JM_DRAW:
      // do nothing
      break;

    case JM_REQSIZE:
      msg->reqsize.w = widget->min_w;
      msg->reqsize.h = widget->min_h;
      return true;

    case JM_SETPOS: {
      JRect cpos;
      JLink link;

      jrect_copy(widget->rc, &msg->setpos.rect);
      cpos = jwidget_get_child_rect(widget);

      /* set all the children to the same "cpos" */
      JI_LIST_FOR_EACH(widget->children, link)
	jwidget_set_rect(reinterpret_cast<JWidget>(link->data), cpos);

      jrect_free(cpos);
      return true;
    }

    case JM_DIRTYCHILDREN: {
      JLink link;

      JI_LIST_FOR_EACH(widget->children, link)
	jwidget_dirty(reinterpret_cast<JWidget>(link->data));

      return true;
    }

    case JM_KEYPRESSED:
    case JM_KEYRELEASED:
      if (msg->key.propagate_to_children) {
	JLink link;

	/* broadcast the message to the children */
	JI_LIST_FOR_EACH(widget->children, link)
	  jwidget_send_message(reinterpret_cast<JWidget>(link->data), msg);
      }

      /* propagate the message to the parent */
      if (msg->key.propagate_to_parent && widget->parent != NULL)
	return jwidget_send_message(widget->parent, msg);
      else
	break;

    case JM_BUTTONPRESSED:
    case JM_BUTTONRELEASED:
    case JM_DOUBLECLICK:
    case JM_MOTION:
    case JM_WHEEL:
      /* propagate the message to the parent */
      if (widget->parent != NULL)
	return jwidget_send_message(widget->parent, msg);
      else
	break;

    case JM_SETCURSOR:
      /* propagate the message to the parent */
      if (widget->parent != NULL)
	return jwidget_send_message(widget->parent, msg);
      else {
	jmouse_set_cursor(JI_CURSOR_NORMAL);
	return true;
      }

  }

  return false;
}

// ===============================================================
// EVENTS
// ===============================================================

/**
   Calculates the preferred size for the widget.

   The default implementation get the preferred size of the current
   layout manager. Also, if there exists layout-free widgets inside
   this parent (like a StatusBar), they preferred-sizes are
   accumulated.

   @see Layout#getPreferredSize,
*/
void Widget::onPreferredSize(PreferredSizeEvent& ev)
{
  JMessage msg = jmessage_new(JM_REQSIZE);
  jwidget_send_message(this, msg);
  Size sz(msg->reqsize.w, msg->reqsize.h);
  jmessage_free(msg);

  ev.setPreferredSize(sz);
}
