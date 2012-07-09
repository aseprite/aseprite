// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

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

#include "base/memory.h"
#include "ui/gui.h"
#include "ui/intern.h"

using namespace gfx;

namespace ui {

static inline void mark_dirty_flag(Widget* widget)
{
  while (widget) {
    widget->flags |= JI_DIRTY;
    widget = widget->parent;
  }
}

int ji_register_widget_type()
{
  static int type = JI_USER_WIDGET;
  return type++;
}

Widget::Widget(int type)
{
  _ji_add_widget(this);

  this->type = type;
  this->rc = jrect_new(0, 0, 0, 0);
  this->border_width.l = 0;
  this->border_width.t = 0;
  this->border_width.r = 0;
  this->border_width.b = 0;
  this->child_spacing = 0;
  this->flags = 0;
  this->min_w = 0;
  this->min_h = 0;
  this->max_w = INT_MAX;
  this->max_h = INT_MAX;
  this->children = jlist_new();
  this->parent = NULL;
  this->m_theme = CurrentTheme::get();

  this->m_align = 0;
  this->m_text = "";
  this->m_font = this->m_theme ? this->m_theme->default_font: NULL;
  this->m_bg_color = -1;

  this->m_update_region = jregion_new(NULL, 0);

  this->theme_data[0] = NULL;
  this->theme_data[1] = NULL;
  this->theme_data[2] = NULL;
  this->theme_data[3] = NULL;

  this->user_data[0] = NULL;
  this->user_data[1] = NULL;
  this->user_data[2] = NULL;
  this->user_data[3] = NULL;

  m_preferredSize = NULL;
  m_doubleBuffered = false;
}

Widget::~Widget()
{
  JLink link, next;

  // Break relationship with the manager.
  if (this->type != JI_MANAGER) {
    Manager* manager = getManager();
    manager->freeWidget(this);
    manager->removeMessagesFor(this);
    manager->removeMessageFilterFor(this);
  }

  // Remove from parent
  if (this->parent)
    this->parent->removeChild(this);

  /* remove children */
  JI_LIST_FOR_EACH_SAFE(this->children, link, next)
    delete reinterpret_cast<Widget*>(link->data);
  jlist_free(this->children);

  /* destroy the update region */
  if (m_update_region)
    jregion_free(m_update_region);

  /* destroy widget position */
  if (this->rc)
    jrect_free(this->rc);

  // Delete the preferred size
  delete m_preferredSize;

  /* low level free */
  _ji_remove_widget(this);
}

void Widget::deferDelete()
{
  getManager()->addToGarbage(this);
}

void Widget::initTheme()
{
  InitThemeEvent ev(this, m_theme);
  onInitTheme(ev);
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
  setTextQuiet(text);
  onSetText();
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

FONT *Widget::getFont() const
{
  return m_font;
}

void Widget::setFont(FONT* f)
{
  m_font = f;
  invalidate();
}

void Widget::setTheme(Theme* theme)
{
  m_theme = theme;

  // TODO maybe some Style in Widget should be great
  setFont(m_theme ? m_theme->default_font: NULL);
}

// ===============================================================
// COMMON PROPERTIES
// ===============================================================

void Widget::setVisible(bool state)
{
  if (state) {
    if (this->flags & JI_HIDDEN) {
      this->flags &= ~JI_HIDDEN;
      invalidate();
    }
  }
  else {
    if (!(this->flags & JI_HIDDEN)) {
      getManager()->freeWidget(this); // Free from manager

      this->flags |= JI_HIDDEN;
    }
  }
}

void Widget::setEnabled(bool state)
{
  if (state) {
    if (this->flags & JI_DISABLED) {
      this->flags &= ~JI_DISABLED;
      invalidate();

      onEnable();
    }
  }
  else {
    if (!(this->flags & JI_DISABLED)) {
      getManager()->freeWidget(this); // Free from the manager

      this->flags |= JI_DISABLED;
      invalidate();

      onDisable();
    }
  }
}

void Widget::setSelected(bool state)
{
  if (state) {
    if (!(this->flags & JI_SELECTED)) {
      this->flags |= JI_SELECTED;
      invalidate();

      onSelect();
    }
  }
  else {
    if (this->flags & JI_SELECTED) {
      this->flags &= ~JI_SELECTED;
      invalidate();

      onDeselect();
    }
  }
}

void Widget::setExpansive(bool state)
{
  if (state)
    this->flags |= JI_EXPANSIVE;
  else
    this->flags &= ~JI_EXPANSIVE;
}

void Widget::setDecorative(bool state)
{
  if (state)
    this->flags |= JI_DECORATIVE;
  else
    this->flags &= ~JI_DECORATIVE;
}

void Widget::setFocusStop(bool state)
{
  if (state)
    this->flags |= JI_FOCUSSTOP;
  else
    this->flags &= ~JI_FOCUSSTOP;
}

void Widget::setFocusMagnet(bool state)
{
  if (state)
    this->flags |= JI_FOCUSMAGNET;
  else
    this->flags &= ~JI_FOCUSMAGNET;
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

bool Widget::isExpansive() const
{
  return (this->flags & JI_EXPANSIVE) ? true: false;
}

bool Widget::isDecorative() const
{
  return (this->flags & JI_DECORATIVE) ? true: false;
}

bool Widget::isFocusStop() const
{
  return (this->flags & JI_FOCUSSTOP) ? true: false;
}

bool Widget::isFocusMagnet() const
{
  return (this->flags & JI_FOCUSMAGNET) ? true: false;
}

// ===============================================================
// PARENTS & CHILDREN
// ===============================================================

Window* Widget::getRoot()
{
  Widget* widget = this;

  while (widget) {
    if (widget->type == JI_WINDOW)
      return dynamic_cast<Window*>(widget);

    widget = widget->parent;
  }

  return NULL;
}

Widget* Widget::getParent()
{
  return this->parent;
}

Manager* Widget::getManager()
{
  Widget* widget = this;

  while (widget) {
    if (widget->type == JI_MANAGER)
      return static_cast<Manager*>(widget);

    widget = widget->parent;
  }

  return Manager::getDefault();
}

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

JList Widget::getChildren()
{
  return jlist_copy(this->children);
}

Widget* Widget::getNextSibling()
{
  if (!parent)
    return NULL;

  JLink link = jlist_find(parent->children, this);
  ASSERT(link != NULL);
  if (!link)
    return NULL;

  if (link == jlist_last(parent->children))
    return NULL;

  return reinterpret_cast<Widget*>(link->next->data);
}

Widget* Widget::getPreviousSibling()
{
  if (!parent)
    return NULL;

  JLink link = jlist_find(parent->children, this);
  ASSERT(link != NULL);
  if (!link)
    return NULL;

  if (link == jlist_first(parent->children))
    return NULL;

  return reinterpret_cast<Widget*>(link->prev->data);
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

Widget* Widget::findChild(const char* id)
{
  Widget* child;
  JLink link;

  JI_LIST_FOR_EACH(this->children, link) {
    child = (Widget*)link->data;
    if (child->getId() == id)
      return child;
  }

  JI_LIST_FOR_EACH(this->children, link) {
    if ((child = ((Widget*)link->data)->findChild(id)))
      return child;
  }

  return 0;
}

Widget* Widget::findSibling(const char* id)
{
  return getRoot()->findChild(id);
}

void Widget::addChild(Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  jlist_append(children, child);
  child->parent = this;
}

void Widget::removeChild(Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  jlist_remove(children, child);
  child->parent = NULL;
}

void Widget::replaceChild(Widget* oldChild, Widget* newChild)
{
  ASSERT_VALID_WIDGET(oldChild);
  ASSERT_VALID_WIDGET(newChild);

  JLink before = jlist_find(children, oldChild);
  if (!before)
    return;

  before = before->next;

  removeChild(oldChild);

  jlist_insert_before(children, before, newChild);
  newChild->parent = this;
}

void Widget::insertChild(int index, Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  jlist_insert(children, child, index);
  child->parent = this;
}

// ===============================================================
// LAYOUT & CONSTRAINT
// ===============================================================

void Widget::layout()
{
  jwidget_set_rect(this, rc);
  invalidate();
}

/**********************************************************************/
/* position and geometry */

Rect Widget::getBounds() const
{
  return Rect(rc->x1, rc->y1, jrect_w(rc), jrect_h(rc));
}

Rect Widget::getClientBounds() const
{
  return Rect(0, 0, jrect_w(rc), jrect_h(rc));
}

void Widget::setBounds(const Rect& rc)
{
  jrect jrc = { rc.x, rc.y, rc.x+rc.w, rc.y+rc.h };
  jwidget_set_rect(this, &jrc);
}

Border Widget::getBorder() const
{
  return Border(border_width.l, border_width.t, border_width.r, border_width.b);
}

void Widget::setBorder(const Border& br)
{
  border_width.l = br.left();
  border_width.t = br.top();
  border_width.r = br.right();
  border_width.b = br.bottom();
}

/* gets the position of the widget */
JRect jwidget_get_rect(Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  return jrect_new_copy(widget->rc);
}

/* gets the position for children of the widget */
JRect jwidget_get_child_rect(Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  return jrect_new(widget->rc->x1 + widget->border_width.l,
                   widget->rc->y1 + widget->border_width.t,
                   widget->rc->x2 - widget->border_width.r,
                   widget->rc->y2 - widget->border_width.b);
}

JRegion jwidget_get_region(Widget* widget)
{
  JRegion region;

  ASSERT_VALID_WIDGET(widget);

  if (widget->type == JI_WINDOW)
    region = widget->getTheme()->get_window_mask(widget);
  else
    region = jregion_new(widget->rc, 1);

  return region;
}

/* gets the region to be able to draw in */
JRegion jwidget_get_drawable_region(Widget* widget, int flags)
{
  Widget* window, *manager, *view, *child;
  JRegion region, reg1, reg2, reg3;
  JList windows_list;
  JLink link;
  JRect cpos;

  ASSERT_VALID_WIDGET(widget);

  region = jwidget_get_region(widget);

  /* cut the top windows areas */
  if (flags & JI_GDR_CUTTOPWINDOWS) {
    window = widget->getRoot();
    manager = window ? window->getManager(): NULL;

    while (manager) {
      windows_list = manager->children;
      link = jlist_find(windows_list, window);

      if (!jlist_empty(windows_list) &&
          window != jlist_first(windows_list)->data &&
          link != windows_list->end) {
        /* subtract the rectangles */
        for (link=link->prev; link != windows_list->end; link=link->prev) {
          reg1 = jwidget_get_region(reinterpret_cast<Widget*>(link->data));
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
      child = reinterpret_cast<Widget*>(link->data);
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
    Widget* parent = widget->parent;

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
    Widget* parent = widget->parent;

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
    view = View::getView(manager);
    if (view) {
      Rect vp = static_cast<View*>(view)->getViewportBounds();
      cpos = jrect_new(vp.x, vp.y, vp.x+vp.w, vp.y+vp.h);
    }
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

int jwidget_get_bg_color(Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  return widget->getBgColor();
}

int jwidget_get_text_length(const Widget* widget)
{
#if 1
  return ji_font_text_len(widget->getFont(), widget->getText());
#else  /* use cached text size */
  return widget->text_size_pix;
#endif
}

int jwidget_get_text_height(const Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  return text_height(widget->getFont());
}

void jwidget_get_texticon_info(Widget* widget,
                               JRect box, JRect text, JRect icon,
                               int icon_align, int icon_w, int icon_h)
{
#define SETRECT(r)                              \
  if (r) {                                      \
    r->x1 = r##_x;                              \
    r->y1 = r##_y;                              \
    r->x2 = r##_x+r##_w;                        \
    r->y2 = r##_y+r##_h;                        \
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
  if (icon_align & JI_CENTER) {   /* with the icon in the center */
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

void jwidget_noborders(Widget* widget)
{
  widget->border_width.l = 0;
  widget->border_width.t = 0;
  widget->border_width.r = 0;
  widget->border_width.b = 0;
  widget->child_spacing = 0;

  widget->invalidate();
}

void jwidget_set_border(Widget* widget, int value)
{
  ASSERT_VALID_WIDGET(widget);

  widget->border_width.l = value;
  widget->border_width.t = value;
  widget->border_width.r = value;
  widget->border_width.b = value;

  widget->invalidate();
}

void jwidget_set_border(Widget* widget, int l, int t, int r, int b)
{
  ASSERT_VALID_WIDGET(widget);

  widget->border_width.l = l;
  widget->border_width.t = t;
  widget->border_width.r = r;
  widget->border_width.b = b;

  widget->invalidate();
}

void jwidget_set_rect(Widget* widget, JRect rect)
{
  Message* msg;

  ASSERT_VALID_WIDGET(widget);

  msg = jmessage_new(JM_SETPOS);
  jrect_copy(&msg->setpos.rect, rect);
  widget->sendMessage(msg);
  jmessage_free(msg);
}

void jwidget_set_min_size(Widget* widget, int w, int h)
{
  ASSERT_VALID_WIDGET(widget);

  widget->min_w = w;
  widget->min_h = h;
}

void jwidget_set_max_size(Widget* widget, int w, int h)
{
  ASSERT_VALID_WIDGET(widget);

  widget->max_w = w;
  widget->max_h = h;
}

void jwidget_set_bg_color(Widget* widget, int color)
{
  ASSERT_VALID_WIDGET(widget);

  widget->setBgColor(color);
}

void Widget::flushRedraw()
{
  std::queue<Widget*> processing;
  int c, nrects;
  Message* msg;
  JLink link;
  JRect rc;

  if (this->flags & JI_DIRTY) {
    this->flags ^= JI_DIRTY;
    processing.push(this);
  }

  while (!processing.empty()) {
    Widget* widget = processing.front();
    processing.pop();

    ASSERT_VALID_WIDGET(widget);

    // If the widget is hidden
    if (!widget->isVisible())
      continue;

    JI_LIST_FOR_EACH(widget->children, link) {
      Widget* child = (Widget*)link->data;
      if (child->flags & JI_DIRTY) {
        child->flags ^= JI_DIRTY;
        processing.push(child);
      }
    }

    nrects = JI_REGION_NUM_RECTS(widget->m_update_region);
    if (nrects > 0) {
      /* get areas to draw */
      JRegion region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
      jregion_intersect(widget->m_update_region,
                        widget->m_update_region, region);
      jregion_free(region);

      nrects = JI_REGION_NUM_RECTS(widget->m_update_region);

      /* draw the widget */
      for (c=0, rc=JI_REGION_RECTS(widget->m_update_region);
           c<nrects;
           c++, rc++) {
        /* create the draw message */
        msg = jmessage_new(JM_DRAW);
        msg->draw.count = nrects-1 - c;
        msg->draw.rect = *rc;
        jmessage_add_dest(msg, widget);

        /* enqueue the draw message */
        getManager()->enqueueMessage(msg);
      }

      jregion_empty(widget->m_update_region);
    }
  }
}

bool Widget::isDoubleBuffered()
{
  return m_doubleBuffered;
}

void Widget::setDoubleBuffered(bool doubleBuffered)
{
  m_doubleBuffered = doubleBuffered;
}

void Widget::invalidate()
{
  if (isVisible()) {
    JRegion reg1 = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    JLink link;

    jregion_copy(this->m_update_region, reg1);
    jregion_free(reg1);

    mark_dirty_flag(this);

    JI_LIST_FOR_EACH(this->children, link)
      reinterpret_cast<Widget*>(link->data)->invalidate();
  }
}

void Widget::invalidateRect(const gfx::Rect& rect)
{
  if (isVisible()) {
    JRect tmp = jrect_new(rect.x, rect.y, rect.x+rect.w, rect.y+rect.h);
    invalidateRect(tmp);
    jrect_free(tmp);
  }
}

void Widget::invalidateRect(const JRect rect)
{
  if (isVisible()) {
    JRegion reg1 = jregion_new(rect, 1);
    invalidateRegion(reg1);
    jregion_free(reg1);
  }
}

void Widget::invalidateRegion(const JRegion region)
{
  onInvalidateRegion(region);
}

void Widget::scrollRegion(JRegion region, int dx, int dy)
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

    jregion_union(this->m_update_region, this->m_update_region, region);
    jregion_subtract(this->m_update_region, this->m_update_region, reg2);

    mark_dirty_flag(this);

    // Generate the JM_DRAW messages for the widget's m_update_region
    this->flushRedraw();

    jregion_free(reg2);
  }
}

// ===============================================================
// GUI MANAGER
// ===============================================================

bool Widget::sendMessage(Message* msg)
{
  ASSERT(msg != NULL);
  return onProcessMessage(msg);
}

void Widget::closeWindow()
{
  if (Window* window = getRoot())
    window->closeWindow(this);
}

void Widget::broadcastMouseMessage(WidgetsList& targets)
{
  onBroadcastMouseMessage(targets);
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
       fit in that width (this is useful to fit Label or Edit controls
       in a specified width and calculate the height it could occupy).

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
  getManager()->setFocus(this);
}

void Widget::releaseFocus()
{
  if (hasFocus())
    getManager()->freeFocus();
}

/**
 * Captures the mouse to send all the future mouse messsages to the
 * specified widget (included the JM_MOTION and JM_SETCURSOR).
 */
void Widget::captureMouse()
{
  if (!getManager()->getCapture()) {
    getManager()->setCapture(this);
    jmouse_capture();
  }
}

/**
 * Releases the capture of the mouse events.
 */
void Widget::releaseMouse()
{
  if (getManager()->getCapture() == this) {
    getManager()->freeCapture();
    jmouse_release();
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

int Widget::getMnemonicChar() const
{
  if (hasText()) {
    const char* text = getText();
    for (int c=0; text[c]; ++c)
      if ((text[c] == '&') && (text[c+1] != '&'))
        return tolower(text[c+1]);
  }
  return 0;
}

bool Widget::isScancodeMnemonic(int scancode) const
{
  int ascii = 0;
  if (scancode >= KEY_0 && scancode <= KEY_9)
    ascii = '0' + (scancode - KEY_0);
  else if (scancode >= KEY_A && scancode <= KEY_Z)
    ascii = 'a' + (scancode - KEY_A);
  else
    return false;

  return (getMnemonicChar() == ascii);
}

/**********************************************************************/
/* widget message procedure */

bool Widget::onProcessMessage(Message* msg)
{
  Widget* widget = this;

  ASSERT(msg != NULL);
  ASSERT_VALID_WIDGET(widget);

  switch (msg->type) {

    case JM_OPEN:
    case JM_CLOSE:
    case JM_WINMOVE: {
      JLink link;

      // Broadcast the message to the children.
      JI_LIST_FOR_EACH(widget->children, link)
        reinterpret_cast<Widget*>(link->data)->sendMessage(msg);
      break;
    }

    case JM_DRAW:
      // With double-buffering we create a temporary bitmap to draw
      // the widget on it and then we blit the final result to the
      // real screen. Anyway, if ji_screen is not the real hardware
      // screen, we already are painting off-screen using ji_screen,
      // so we don't need the temporary bitmap.
      if (m_doubleBuffered && ji_screen == screen) {
        ASSERT(jrect_w(&msg->draw.rect) > 0);
        ASSERT(jrect_h(&msg->draw.rect) > 0);

        BITMAP* bmp = create_bitmap_ex(bitmap_color_depth(ji_screen),
                                       jrect_w(&msg->draw.rect),
                                       jrect_h(&msg->draw.rect));

        Graphics graphics(bmp, rc->x1-msg->draw.rect.x1, rc->y1-msg->draw.rect.y1);
        graphics.setFont(getFont());

        PaintEvent ev(this, &graphics);
        onPaint(ev); // Fire onPaint event

        // Blit the temporary bitmap to the real screen
        if (ev.isPainted())
          blit(bmp, ji_screen, 0, 0, msg->draw.rect.x1, msg->draw.rect.y1, bmp->w, bmp->h);

        destroy_bitmap(bmp);
        return ev.isPainted();
      }
      // Paint directly on ji_screen (in this case "ji_screen" can be
      // the screen or a memory bitmap).
      else {
        Graphics graphics(ji_screen, rc->x1, rc->y1);
        graphics.setFont(getFont());

        PaintEvent ev(this, &graphics);
        onPaint(ev); // Fire onPaint event
        return ev.isPainted();
      }

    case JM_REQSIZE:
      msg->reqsize.w = widget->min_w;
      msg->reqsize.h = widget->min_h;
      return true;

    case JM_SETPOS: {
      JRect cpos;
      JLink link;

      jrect_copy(widget->rc, &msg->setpos.rect);
      cpos = jwidget_get_child_rect(widget);

      // Set all the children to the same "cpos".
      JI_LIST_FOR_EACH(widget->children, link)
        jwidget_set_rect(reinterpret_cast<Widget*>(link->data), cpos);

      jrect_free(cpos);
      return true;
    }

    case JM_DIRTYCHILDREN: {
      JLink link;

      JI_LIST_FOR_EACH(widget->children, link)
        reinterpret_cast<Widget*>(link->data)->invalidate();

      return true;
    }

    case JM_KEYPRESSED:
    case JM_KEYRELEASED:
      if (msg->key.propagate_to_children) {
        JLink link;

        // Broadcast the message to the children.
        JI_LIST_FOR_EACH(widget->children, link)
          reinterpret_cast<Widget*>(link->data)->sendMessage(msg);
      }

      // Propagate the message to the parent.
      if (msg->key.propagate_to_parent && widget->parent != NULL)
        return widget->parent->sendMessage(msg);
      else
        break;

    case JM_BUTTONPRESSED:
    case JM_BUTTONRELEASED:
    case JM_DOUBLECLICK:
    case JM_MOTION:
    case JM_WHEEL:
      // Propagate the message to the parent.
      if (widget->parent != NULL)
        return widget->parent->sendMessage(msg);
      else
        break;

    case JM_SETCURSOR:
      // Propagate the message to the parent.
      if (widget->parent != NULL)
        return widget->parent->sendMessage(msg);
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

void Widget::onInvalidateRegion(const JRegion region)
{
  if (isVisible() &&
      jregion_rect_in(region, this->rc) != JI_RGNOUT) {
    JRegion reg1 = jregion_new(NULL, 0);
    JRegion reg2 = jwidget_get_drawable_region(this,
                                               JI_GDR_CUTTOPWINDOWS);
    JLink link;

    jregion_union(reg1, this->m_update_region, region);
    jregion_intersect(this->m_update_region, reg1, reg2);
    jregion_free(reg2);

    jregion_subtract(reg1, region, this->m_update_region);

    mark_dirty_flag(this);

    JI_LIST_FOR_EACH(this->children, link)
      reinterpret_cast<Widget*>(link->data)->invalidateRegion(reg1);

    jregion_free(reg1);
  }
}

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
  Message* msg = jmessage_new(JM_REQSIZE);
  sendMessage(msg);
  Size sz(msg->reqsize.w, msg->reqsize.h);
  jmessage_free(msg);

  ev.setPreferredSize(sz);
}

void Widget::onPaint(PaintEvent& ev)
{
  // Do nothing
}

void Widget::onBroadcastMouseMessage(WidgetsList& targets)
{
  // Do nothing
}

void Widget::onInitTheme(InitThemeEvent& ev)
{
  if (m_theme) {
    m_theme->init_widget(this);

    if (!(flags & JI_INITIALIZED))
      flags |= JI_INITIALIZED;
  }
}

void Widget::onEnable()
{
  // Do nothing
}

void Widget::onDisable()
{
  // Do nothing
}

void Widget::onSelect()
{
  // Do nothing
}

void Widget::onDeselect()
{
  // Do nothing
}

void Widget::onSetText()
{
  invalidate();
}

} // namespace ui
