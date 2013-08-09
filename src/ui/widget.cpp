// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

/* #define REPORT_SIGNALS */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/memory.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <allegro.h>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <queue>
#include <sstream>

namespace ui {

using namespace gfx;

static inline void mark_dirty_flag(Widget* widget)
{
  while (widget) {
    widget->flags |= JI_DIRTY;
    widget = widget->getParent();
  }
}

WidgetType register_widget_type()
{
  static int type = (int)kFirstUserWidget;
  return (WidgetType)type++;
}

Widget::Widget(WidgetType type)
{
  addWidget(this);

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
  this->m_parent = NULL;
  this->m_theme = CurrentTheme::get();

  this->m_align = 0;
  this->m_text = "";
  this->m_font = this->m_theme ? this->m_theme->default_font: NULL;
  this->m_bgColor = ui::ColorNone;

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
  // Break relationship with the manager.
  if (this->type != kManagerWidget) {
    Manager* manager = getManager();
    manager->freeWidget(this);
    manager->removeMessagesFor(this);
    manager->removeMessageFilterFor(this);
  }

  // Remove from parent
  if (m_parent)
    m_parent->removeChild(this);

  // Remove children. The ~Widget dtor modifies the parent's
  // m_children.
  while (!m_children.empty())
    delete m_children.front();

  /* destroy widget position */
  if (this->rc)
    jrect_free(this->rc);

  // Delete the preferred size
  delete m_preferredSize;

  // Low level free
  removeWidget(this);
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

    widget = widget->m_parent;
  } while (widget);

  return true;
}

bool Widget::isEnabled() const
{
  const Widget* widget = this;

  do {
    if (widget->flags & JI_DISABLED)
      return false;

    widget = widget->m_parent;
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
    if (widget->type == kWindowWidget)
      return dynamic_cast<Window*>(widget);

    widget = widget->m_parent;
  }

  return NULL;
}

Manager* Widget::getManager()
{
  Widget* widget = this;

  while (widget) {
    if (widget->type == kManagerWidget)
      return static_cast<Manager*>(widget);

    widget = widget->m_parent;
  }

  return Manager::getDefault();
}

void Widget::getParents(bool ascendant, WidgetsList& parents)
{
  for (Widget* widget=this; widget; widget=widget->m_parent) {
    // append parents in tail
    if (ascendant)
      parents.push_back(widget);
    // append parents in head
    else
      parents.insert(parents.begin(), widget);
  }
}

Widget* Widget::getNextSibling()
{
  if (!m_parent)
    return NULL;

  WidgetsList::iterator begin = m_parent->m_children.begin();
  WidgetsList::iterator end = m_parent->m_children.end();
  WidgetsList::iterator it = std::find(begin, end, this);

  if (it == end)
    return NULL;

  if (++it == end)
    return NULL;

  return *it;
}

Widget* Widget::getPreviousSibling()
{
  if (!m_parent)
    return NULL;

  WidgetsList::iterator begin = m_parent->m_children.begin();
  WidgetsList::iterator end = m_parent->m_children.end();
  WidgetsList::iterator it = std::find(begin, end, this);

  if (it == begin || it == end)
    return NULL;

  return *(++it);
}

Widget* Widget::pick(const gfx::Point& pt)
{
  Widget* inside, *picked = NULL;

  if (!(this->flags & JI_HIDDEN) &&           // Is visible
      jrect_point_in(this->rc, pt.x, pt.y)) { // The point is inside the bounds
    picked = this;

    UI_FOREACH_WIDGET(m_children, it) {
      inside = (*it)->pick(pt);
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

  return std::find(m_children.begin(), m_children.end(), child) != m_children.end();
}

Widget* Widget::findChild(const char* id)
{
  Widget* child;

  UI_FOREACH_WIDGET(m_children, it) {
    child = *it;
    if (child->getId() == id)
      return child;
  }

  UI_FOREACH_WIDGET(m_children, it) {
    if ((child = (*it)->findChild(id)))
      return child;
  }

  return NULL;
}

Widget* Widget::findSibling(const char* id)
{
  return getRoot()->findChild(id);
}

void Widget::addChild(Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  m_children.push_back(child);
  child->m_parent = this;
}

void Widget::removeChild(Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  WidgetsList::iterator it = std::find(m_children.begin(), m_children.end(), child);
  if (it != m_children.end())
    m_children.erase(it);

  // Free from manager
  Manager* manager = getManager();
  if (manager)
    manager->freeWidget(this);

  child->m_parent = NULL;
}

void Widget::replaceChild(Widget* oldChild, Widget* newChild)
{
  ASSERT_VALID_WIDGET(oldChild);
  ASSERT_VALID_WIDGET(newChild);

  WidgetsList::iterator before =
    std::find(m_children.begin(), m_children.end(), oldChild);
  if (before == m_children.end()) {
    ASSERT(false);
    return;
  }
  int index = before - m_children.begin();

  removeChild(oldChild);

  m_children.insert(m_children.begin()+index, newChild);
  newChild->m_parent = this;
}

void Widget::insertChild(int index, Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  m_children.insert(m_children.begin()+index, child);
  child->m_parent = this;
}

// ===============================================================
// LAYOUT & CONSTRAINT
// ===============================================================

void Widget::layout()
{
  setBounds(getBounds());
  invalidate();
}

void Widget::loadLayout()
{
  if (!m_id.empty()) {
    LayoutIO* io = getManager()->getLayoutIO();
    if (io) {
      std::string layout = io->loadLayout(this);
      if (!layout.empty()) {
        std::stringstream s(layout);
        LoadLayoutEvent ev(this, s);
        onLoadLayout(ev);
      }
    }
  }

  // Do for all children
  UI_FOREACH_WIDGET(m_children, it)
    (*it)->loadLayout();
}

void Widget::saveLayout()
{
  if (!m_id.empty()) {
    LayoutIO* io = getManager()->getLayoutIO();
    if (io) {
      std::stringstream s;
      SaveLayoutEvent ev(this, s);
      onSaveLayout(ev);

      std::string layout = s.str();
      if (!layout.empty())
        io->saveLayout(this, layout);
    }
  }

  // Do for all children
  UI_FOREACH_WIDGET(m_children, it)
    (*it)->saveLayout();
}

void Widget::setDecorativeWidgetBounds()
{
  onSetDecorativeWidgetBounds();
}

// ===============================================================
// POSITION & GEOMETRY
// ===============================================================

Rect Widget::getChildrenBounds() const
{
  return Rect(rc->x1+border_width.l,
              rc->y1+border_width.t,
              jrect_w(rc) - border_width.l - border_width.r,
              jrect_h(rc) - border_width.t - border_width.b);
}

Rect Widget::getClientChildrenBounds() const
{
  return Rect(border_width.l,
              border_width.t,
              jrect_w(rc) - border_width.l - border_width.r,
              jrect_h(rc) - border_width.t - border_width.b);
}

void Widget::setBounds(const Rect& rc)
{
  ResizeEvent ev(this, rc);
  onResize(ev);
}

void Widget::setBoundsQuietly(const gfx::Rect& rc)
{
  jrect jrc = { rc.x, rc.y, rc.x2(), rc.y2() };
  jrect_copy(this->rc, &jrc);
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

void Widget::getRegion(gfx::Region& region)
{
  if (this->type == kWindowWidget)
    getTheme()->getWindowMask(this, region);
  else
    region = getBounds();
}

void Widget::getDrawableRegion(gfx::Region& region, DrawableRegionFlags flags)
{
  Widget* window, *manager, *view;

  getRegion(region);

  // Cut the top windows areas
  if (flags & kCutTopWindows) {
    window = getRoot();
    manager = window ? window->getManager(): NULL;

    while (manager) {
      const WidgetsList& windows_list = manager->getChildren();
      WidgetsList::const_reverse_iterator it =
        std::find(windows_list.rbegin(), windows_list.rend(), window);

      if (!windows_list.empty() &&
          window != windows_list.front() &&
          it != windows_list.rend()) {
        // Subtract the rectangles
        for (++it; it != windows_list.rend(); ++it) {
          Region reg1;
          (*it)->getRegion(reg1);
          region.createSubtraction(region, reg1);
        }
      }

      window = manager->getRoot();
      manager = window ? window->getManager(): NULL;
    }
  }

  // Clip the areas where are children
  if (!(flags & kUseChildArea) && !getChildren().empty()) {
    Region reg1;
    Region reg2(getChildrenBounds());

    UI_FOREACH_WIDGET(getChildren(), it) {
      Widget* child = *it;
      if (child->isVisible()) {
        Region reg3;
        child->getRegion(reg3);

        if (child->flags & JI_DECORATIVE) {
          reg1 = getBounds();
          reg1.createIntersection(reg1, reg3);
        }
        else {
          reg1.createIntersection(reg2, reg3);
        }
        region.createSubtraction(region, reg1);
      }
    }
  }

  // Intersect with the parent area
  if (!(this->flags & JI_DECORATIVE)) {
    Widget* parent = getParent();

    while (parent) {
      region.createIntersection(region,
                                Region(parent->getChildrenBounds()));
      parent = parent->getParent();
    }
  }
  else {
    Widget* parent = getParent();
    if (parent) {
      region.createIntersection(region,
                                Region(parent->getBounds()));
    }
  }

  // Limit to the manager area
  window = getRoot();
  manager = (window ? window->getManager(): NULL);

  while (manager) {
    view = View::getView(manager);

    Rect cpos;
    if (view) {
      cpos = static_cast<View*>(view)->getViewportBounds();
    }
    else
      cpos = manager->getChildrenBounds();

    region.createIntersection(region, Region(cpos));

    window = manager->getRoot();
    manager = (window ? window->getManager(): NULL);
  }
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

void Widget::flushRedraw()
{
  std::queue<Widget*> processing;
  Message* msg;

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

    UI_FOREACH_WIDGET(widget->getChildren(), it) {
      Widget* child = *it;
      if (child->flags & JI_DIRTY) {
        child->flags ^= JI_DIRTY;
        processing.push(child);
      }
    }

    if (!widget->m_updateRegion.isEmpty()) {
      // Intersect m_updateRegion with drawable area.
      {
        Region region;
        widget->getDrawableRegion(region, kCutTopWindows);
        widget->m_updateRegion.createIntersection(widget->m_updateRegion, region);
      }

      size_t c, nrects = widget->m_updateRegion.size();
      Region::const_iterator it = widget->m_updateRegion.begin();

      // Draw the widget
      int count = nrects-1;
      for (c=0; c<nrects; ++c, ++it, --count) {
        // Create the draw message
        msg = new PaintMessage(count, *it);
        msg->addRecipient(widget);

        // Enqueue the draw message
        getManager()->enqueueMessage(msg);
      }

      widget->m_updateRegion.clear();
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
    m_updateRegion.clear();
    getDrawableRegion(m_updateRegion, kCutTopWindows);

    mark_dirty_flag(this);

    UI_FOREACH_WIDGET(getChildren(), it)
      (*it)->invalidate();
  }
}

void Widget::invalidateRect(const gfx::Rect& rect)
{
  if (isVisible())
    invalidateRegion(Region(rect));
}

void Widget::invalidateRegion(const Region& region)
{
  onInvalidateRegion(region);
}

void Widget::scrollRegion(const Region& region, int dx, int dy)
{
  if (dx != 0 || dy != 0) {
    Region reg2 = region;
    reg2.offset(dx, dy);
    reg2.createIntersection(reg2, region);
    reg2.offset(-dx, -dy);

    // Move screen pixels
    jmouse_hide();
    ji_move_region(reg2, dx, dy);
    jmouse_show();

    reg2.offset(dx, dy);

    m_updateRegion.createUnion(m_updateRegion, region);
    m_updateRegion.createSubtraction(m_updateRegion, reg2);

    mark_dirty_flag(this);

    // Generate the kPaintMessage messages for the widget's m_updateRegion
    flushRedraw();
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
 * specified widget (included the kMouseMoveMessage and kSetCursorMessage).
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
  return (this == this->pick(gfx::Point(jmouse_x(0), jmouse_y(0))));
}

bool Widget::hasCapture()
{
  return (this->flags & JI_HASCAPTURE) ? true: false;
}

int Widget::getMnemonicChar() const
{
  if (hasText()) {
    for (int c=0; m_text[c]; ++c)
      if ((m_text[c] == '&') && (m_text[c+1] != '&'))
        return tolower(m_text[c+1]);
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

  int mnemonic = getMnemonicChar();
  return (mnemonic > 0 && mnemonic == ascii);
}

/**********************************************************************/
/* widget message procedure */

bool Widget::onProcessMessage(Message* msg)
{
  ASSERT(msg != NULL);

  switch (msg->type()) {

    case kOpenMessage:
    case kCloseMessage:
    case kWinMoveMessage:
      // Broadcast the message to the children.
      UI_FOREACH_WIDGET(getChildren(), it)
        (*it)->sendMessage(msg);
      break;

    case kPaintMessage:
      // With double-buffering we create a temporary bitmap to draw
      // the widget on it and then we blit the final result to the
      // real screen. Anyway, if ji_screen is not the real hardware
      // screen, we already are painting off-screen using ji_screen,
      // so we don't need the temporary bitmap.
      if (m_doubleBuffered && ji_screen == screen) {
        const PaintMessage* ptmsg = static_cast<const PaintMessage*>(msg);

        ASSERT(ptmsg->rect().w > 0);
        ASSERT(ptmsg->rect().h > 0);

        BITMAP* bmp = create_bitmap_ex(bitmap_color_depth(ji_screen),
                                       ptmsg->rect().w,
                                       ptmsg->rect().h);

        Graphics graphics(bmp, rc->x1 - ptmsg->rect().x, rc->y1 - ptmsg->rect().y);
        graphics.setFont(getFont());

        PaintEvent ev(this, &graphics);
        onPaint(ev); // Fire onPaint event

        // Blit the temporary bitmap to the real screen
        if (ev.isPainted())
          blit(bmp, ji_screen, 0, 0, ptmsg->rect().x, ptmsg->rect().y, bmp->w, bmp->h);

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

    case kKeyDownMessage:
    case kKeyUpMessage:
      if (static_cast<KeyMessage*>(msg)->propagateToChildren()) {
        // Broadcast the message to the children.
        UI_FOREACH_WIDGET(getChildren(), it)
          (*it)->sendMessage(msg);
      }

      // Propagate the message to the parent.
      if (static_cast<KeyMessage*>(msg)->propagateToParent() && getParent() != NULL)
        return getParent()->sendMessage(msg);
      else
        break;

    case kMouseDownMessage:
    case kMouseUpMessage:
    case kDoubleClickMessage:
    case kMouseMoveMessage:
    case kMouseWheelMessage:
      // Propagate the message to the parent.
      if (getParent() != NULL)
        return getParent()->sendMessage(msg);
      else
        break;

    case kSetCursorMessage:
      // Propagate the message to the parent.
      if (getParent() != NULL)
        return getParent()->sendMessage(msg);
      else {
        jmouse_set_cursor(kArrowCursor);
        return true;
      }

  }

  return false;
}

// ===============================================================
// EVENTS
// ===============================================================

void Widget::onInvalidateRegion(const Region& region)
{
  if (isVisible() && region.contains(getBounds()) != Region::Out) {
    Region reg1;
    reg1.createUnion(m_updateRegion, region);
    {
      Region reg2;
      getDrawableRegion(reg2, kCutTopWindows);
      m_updateRegion.createIntersection(reg1, reg2);
    }
    reg1.createSubtraction(region, m_updateRegion);

    mark_dirty_flag(this);

    UI_FOREACH_WIDGET(getChildren(), it)
      (*it)->invalidateRegion(reg1);
  }
}

void Widget::onPreferredSize(PreferredSizeEvent& ev)
{
  ev.setPreferredSize(Size(min_w, min_h));
}

void Widget::onLoadLayout(LoadLayoutEvent& ev)
{
  // Do nothing
}

void Widget::onSaveLayout(SaveLayoutEvent& ev)
{
  // Do nothing
}

void Widget::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  // Set all the children to the same "cpos".
  gfx::Rect cpos = getChildrenBounds();
  UI_FOREACH_WIDGET(getChildren(), it)
    (*it)->setBounds(cpos);
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
    m_theme->initWidget(this);

    if (!(flags & JI_INITIALIZED))
      flags |= JI_INITIALIZED;
  }
}

void Widget::onSetDecorativeWidgetBounds()
{
  if (m_theme) {
    m_theme->setDecorativeWidgetBounds(this);
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
