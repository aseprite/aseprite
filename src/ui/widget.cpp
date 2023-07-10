// Aseprite UI Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

// #define REPORT_SIGNALS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/widget.h"

#include "base/memory.h"
#include "base/string.h"
#include "base/utf8_decode.h"
#include "os/font.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/app_state.h"
#include "ui/init_theme_event.h"
#include "ui/intern.h"
#include "ui/layout_io.h"
#include "ui/load_layout_event.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/move_region.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/save_layout_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/window.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <queue>
#include <sstream>

namespace ui {

using namespace gfx;

WidgetType register_widget_type()
{
  static int type = (int)kFirstUserWidget;
  return (WidgetType)type++;
}

Widget::Widget(WidgetType type)
  : m_type(type)
  , m_flags(0)
  , m_theme(get_theme())
  , m_style(nullptr)
  , m_font(nullptr)
  , m_bgColor(gfx::ColorNone)
  , m_bounds(0, 0, 0, 0)
  , m_parent(nullptr)
  , m_parentIndex(-1)
  , m_sizeHint(nullptr)
  , m_mnemonic(0)
  , m_minSize(0, 0)
  , m_maxSize(std::numeric_limits<int>::max(),
              std::numeric_limits<int>::max())
  , m_childSpacing(0)
{
  details::addWidget(this);
}

Widget::~Widget()
{
  // First, we remove children (so children's ~Widget() can access to
  // the manager()).
  while (!m_children.empty())
    delete m_children.front();

  // Break relationship with the manager. This cannot be before
  // deleting children, if we delete children after releasing the
  // parent, a children deletion could generate a kMouseLeaveMessage
  // for the parent that will be deleted too.
  Manager* manager = this->manager();
  ASSERT(manager);
  if (manager) {
    manager->freeWidget(this);
    manager->removeMessagesFor(this);
    manager->removeMessageFilterFor(this);
  }

  // Remove this widget from parent.
  if (m_parent)
    m_parent->removeChild(this);

  // Delete fixed size hint if it isn't nullptr
  delete m_sizeHint;

  // Low level free
  details::removeWidget(this);
}

void Widget::deferDelete()
{
  if (auto man = manager())
    man->addToGarbage(this);
  else {
    ASSERT(false);
  }
}

void Widget::initTheme()
{
  InitThemeEvent ev(this, m_theme);
  onInitTheme(ev);
}

int Widget::textInt() const
{
  return onGetTextInt();
}

double Widget::textDouble() const
{
  return onGetTextDouble();
}

void Widget::setText(const std::string& text)
{
  setTextQuiet(text);
  onSetText();
}

void Widget::setTextf(const char *format, ...)
{
  // formatted string
  if (format) {
    va_list ap;
    va_start(ap, format);
    char buf[4096];
    vsprintf(buf, format, ap);
    va_end(ap);

    setText(buf);
  }
  // empty string
  else {
    setText("");
  }
}

void Widget::setTextQuiet(const std::string& text)
{
  assert_ui_thread();

  m_text = text;
  enableFlags(HAS_TEXT);
}

os::Font* Widget::font() const
{
  if (!m_font && m_theme)
    m_font = AddRef(m_theme->getWidgetFont(this));
  return m_font.get();
}

void Widget::setBgColor(gfx::Color color)
{
  assert_ui_thread();

  m_bgColor = color;
  onSetBgColor();

#ifdef _DEBUG
  if (m_style) {
    LOG(WARNING, "UI: %s: Warning setting bgColor to a widget with style (%s)\n",
        typeid(*this).name(), m_style->id().c_str());
  }
#endif
}

void Widget::setTheme(Theme* theme)
{
  assert_ui_thread();

  m_theme = theme;
  m_font = nullptr;

  for (auto child : children())
    child->setTheme(theme);
}

void Widget::setStyle(Style* style)
{
  assert_ui_thread();

  m_style = style;
  m_border = m_theme->calcBorder(this, style);
  m_bgColor = m_theme->calcBgColor(this, style);
  m_minSize = m_theme->calcMinSize(this, style);
  m_maxSize = m_theme->calcMaxSize(this, style);
  if (style->font())
    m_font = AddRef(style->font());
}

// ===============================================================
// COMMON PROPERTIES
// ===============================================================

void Widget::setVisible(bool state)
{
  assert_ui_thread();

  if (state) {
    if (hasFlags(HIDDEN)) {
      disableFlags(HIDDEN);
      invalidate();

      onVisible(true);
    }
  }
  else {
    if (!hasFlags(HIDDEN)) {
      if (auto man = manager())
        man->freeWidget(this); // Free from manager
      enableFlags(HIDDEN);

      onVisible(false);
    }
  }
}

void Widget::setEnabled(bool state)
{
  assert_ui_thread();

  if (state) {
    if (hasFlags(DISABLED)) {
      disableFlags(DISABLED);
      invalidate();

      onEnable(true);
    }
  }
  else {
    if (!hasFlags(DISABLED)) {
      if (auto man = manager())
        man->freeWidget(this); // Free from the manager

      enableFlags(DISABLED);
      invalidate();

      onEnable(false);
    }
  }
}

void Widget::setSelected(bool state)
{
  assert_ui_thread();

  if (state) {
    if (!hasFlags(SELECTED)) {
      enableFlags(SELECTED);
      invalidate();

      onSelect(true);
    }
  }
  else {
    if (hasFlags(SELECTED)) {
      disableFlags(SELECTED);
      invalidate();

      onSelect(false);
    }
  }
}

void Widget::setExpansive(bool state)
{
  assert_ui_thread();

  if (state)
    enableFlags(EXPANSIVE);
  else
    disableFlags(EXPANSIVE);
}

void Widget::setDecorative(bool state)
{
  assert_ui_thread();

  if (state)
    enableFlags(DECORATIVE);
  else
    disableFlags(DECORATIVE);
}

void Widget::setFocusStop(bool state)
{
  assert_ui_thread();

  if (state)
    enableFlags(FOCUS_STOP);
  else
    disableFlags(FOCUS_STOP);
}

void Widget::setFocusMagnet(bool state)
{
  assert_ui_thread();

  if (state)
    enableFlags(FOCUS_MAGNET);
  else
    disableFlags(FOCUS_MAGNET);
}

bool Widget::isVisible() const
{
  assert_ui_thread();

  const Widget* widget = this;
  const Widget* lastWidget = nullptr;

  do {
    if (widget->hasFlags(HIDDEN))
      return false;

    lastWidget = widget;
    widget = widget->m_parent;
  } while (widget);

  // The widget is visible if it's inside a visible manager
  return (lastWidget ? lastWidget->type() == kManagerWidget: false);
}

bool Widget::isEnabled() const
{
  assert_ui_thread();

  const Widget* widget = this;

  do {
    if (widget->hasFlags(DISABLED))
      return false;

    widget = widget->m_parent;
  } while (widget);

  return true;
}

bool Widget::isSelected() const
{
  assert_ui_thread();
  return hasFlags(SELECTED);
}

bool Widget::isExpansive() const
{
  assert_ui_thread();
  return hasFlags(EXPANSIVE);
}

bool Widget::isDecorative() const
{
  assert_ui_thread();
  return hasFlags(DECORATIVE);
}

bool Widget::isFocusStop() const
{
  assert_ui_thread();
  return hasFlags(FOCUS_STOP);
}

bool Widget::isFocusMagnet() const
{
  assert_ui_thread();
  return hasFlags(FOCUS_MAGNET);
}

// ===============================================================
// PARENTS & CHILDREN
// ===============================================================

Window* Widget::window() const
{
  assert_ui_thread();

  const Widget* widget = this;

  while (widget) {
    if (widget->type() == kWindowWidget)
      return static_cast<Window*>(const_cast<Widget*>(widget));

    widget = widget->m_parent;
  }

  return nullptr;
}

Manager* Widget::manager() const
{
  assert_ui_thread();

  const Widget* widget = this;

  while (widget) {
    if (widget->type() == kManagerWidget)
      return static_cast<Manager*>(const_cast<Widget*>(widget));

    widget = widget->m_parent;
  }

  return Manager::getDefault();
}

Display* Widget::display() const
{
  if (Window* win = this->window())
    return win->display();
  else
    return manager()->display();
}

int Widget::getChildIndex(Widget* child)
{
  if (!child)
    return -1;

#ifdef _DEBUG
  ASSERT(child->parent() == this);
  auto it = std::find(m_children.begin(), m_children.end(), child);
  ASSERT(it != m_children.end());
  ASSERT(child->parentIndex() == (it - m_children.begin()));
#endif

  return child->parentIndex();
}

Widget* Widget::nextSibling()
{
  assert_ui_thread();

  if (!m_parent)
    return nullptr;

  auto begin = m_parent->m_children.begin();
  auto end = m_parent->m_children.end();
  auto it = begin + m_parentIndex;

  if (++it == end)
    return nullptr;

  return *it;
}

Widget* Widget::previousSibling()
{
  assert_ui_thread();

  if (!m_parent)
    return nullptr;

  auto begin = m_parent->m_children.begin();
  auto it = begin + m_parentIndex;

  if (it == begin)
    return nullptr;

  return *(--it);
}

Widget* Widget::pick(const gfx::Point& pt,
                     const bool checkParentsVisibility) const
{
  assert_ui_thread();

  const Widget* inside, *picked = nullptr;

  // isVisible() checks visibility of widget's parent.
  if (((checkParentsVisibility && isVisible()) ||
       (!checkParentsVisibility && !hasFlags(HIDDEN))) &&
      (bounds().contains(pt))) {
    picked = this;

    for (Widget* child : m_children) {
      inside = child->pick(pt, false);
      if (inside) {
        picked = inside;
        break;
      }
    }
  }

  return const_cast<Widget*>(picked);
}

Widget* Widget::pickFromScreenPos(const gfx::Point& screenPos) const
{
  return pick(display()->nativeWindow()->pointFromScreen(screenPos));
}

bool Widget::hasChild(Widget* child)
{
  ASSERT_VALID_WIDGET(child);
  assert_ui_thread();

  return std::find(m_children.begin(), m_children.end(), child) != m_children.end();
}

bool Widget::hasAncestor(Widget* ancestor)
{
  for (Widget* widget=m_parent; widget; widget=widget->m_parent) {
    if (widget == ancestor)
      return true;
  }
  return false;
}

Widget* Widget::findChild(const char* id) const
{
  for (auto child : m_children) {
    if (child->id() == id)
      return child;
  }

  for (auto child : m_children) {
    auto subchild = child->findChild(id);
    if (subchild)
      return subchild;
  }

  return nullptr;
}

Widget* Widget::findSibling(const char* id) const
{
  return window()->findChild(id);
}

void Widget::addChild(Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  // Remove the widget from its current parent
  if (child->m_parent)
    child->m_parent->removeChild(child);

  int i = int(m_children.size());
  m_children.push_back(child);
  child->m_parent = this;
  child->m_parentIndex = i;
}

void Widget::removeChild(const WidgetsList::iterator& it)
{
  Widget* child = nullptr;

  ASSERT(it != m_children.end());
  if (it != m_children.end()) {
    child = *it;

    auto it2 = m_children.erase(it);
    for (auto end=m_children.end(); it2!=end; ++it2)
      --(*it2)->m_parentIndex;

    ASSERT(child);
    if (!child)
      return;
  }
  else
    return;

  if (auto man = manager()) {
    // Remove all paint messages for this widget.
    man->removeMessagesFor(child, kPaintMessage);

    // Free child from manager.
    man->freeWidget(child);
  }

  child->m_parent = nullptr;
  child->m_parentIndex = -1;
}

void Widget::removeChild(Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);
  ASSERT(child->parent() == this);
  if (child->parent() == this)
    removeChild(m_children.begin() + child->m_parentIndex);
}

void Widget::removeAllChildren()
{
  while (!m_children.empty())
    removeChild(--m_children.end());
}

void Widget::replaceChild(Widget* oldChild, Widget* newChild)
{
  ASSERT_VALID_WIDGET(oldChild);
  ASSERT_VALID_WIDGET(newChild);

#if _DEBUG
  {
    auto it = std::find(m_children.begin(), m_children.end(), oldChild);
    ASSERT(it != m_children.end());
    ASSERT(oldChild->m_parentIndex == (it - m_children.begin()));
  }
#endif

  if (oldChild->parent() != this) {
    ASSERT(false);
    return;
  }
  int index = oldChild->m_parentIndex;

  removeChild(oldChild);

  auto it = m_children.begin() + index;
  it = m_children.insert(it, newChild);
  for (auto end=m_children.end(); it!=end; ++it)
    ++(*it)->m_parentIndex;

  newChild->m_parent = this;
  newChild->m_parentIndex = index;
}

void Widget::insertChild(int index, Widget* child)
{
  ASSERT_VALID_WIDGET(this);
  ASSERT_VALID_WIDGET(child);

  index = std::clamp(index, 0, int(m_children.size()));

  auto it = m_children.begin() + index;
  it = m_children.insert(it, child);
  ++it;
  for (auto end=m_children.end(); it!=end; ++it)
    ++(*it)->m_parentIndex;

  child->m_parent = this;
  child->m_parentIndex = index;
}

void Widget::moveChildTo(Widget* thisChild, Widget* toThisPosition)
{
  ASSERT(thisChild->parent() == this);
  ASSERT(toThisPosition->parent() == this);

  const int from = thisChild->m_parentIndex;
  const int to = toThisPosition->m_parentIndex;

  auto it = m_children.begin() + from;
  it = m_children.erase(it);
  auto end = m_children.end();
  for (; it!=end; ++it)
    --(*it)->m_parentIndex;

  it = m_children.begin() + to;
  it = m_children.insert(it, thisChild);
  thisChild->m_parentIndex = to;
  for (++it, end=m_children.end(); it!=end; ++it)
    ++(*it)->m_parentIndex;
}

// ===============================================================
// LAYOUT & CONSTRAINT
// ===============================================================

void Widget::layout()
{
  setBounds(bounds());
  invalidate();
}

void Widget::loadLayout()
{
  if (!m_id.empty()) {
    auto man = manager();
    LayoutIO* io = (man ? man->getLayoutIO(): nullptr);
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
  for (auto child : m_children)
    child->loadLayout();
}

void Widget::saveLayout()
{
  if (!m_id.empty()) {
    auto man = manager();
    LayoutIO* io = (man ? man->getLayoutIO(): nullptr);
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
  for (auto child : m_children)
    child->saveLayout();
}

void Widget::setDecorativeWidgetBounds()
{
  onSetDecorativeWidgetBounds();
}

// ===============================================================
// POSITION & GEOMETRY
// ===============================================================

Rect Widget::childrenBounds() const
{
  return Rect(m_bounds.x + border().left(),
              m_bounds.y + border().top(),
              m_bounds.w - border().width(),
              m_bounds.h - border().height());
}

Rect Widget::clientChildrenBounds() const
{
  return Rect(border().left(),
              border().top(),
              m_bounds.w - border().width(),
              m_bounds.h - border().height());
}

gfx::Rect Widget::boundsOnScreen() const
{
  gfx::Rect rc = bounds();
  os::Window* nativeWindow = display()->nativeWindow();
  rc = gfx::Rect(
    nativeWindow->pointToScreen(rc.origin()),
    nativeWindow->pointToScreen(rc.point2()));
  return rc;
}

void Widget::setBounds(const Rect& rc)
{
  // Don't generate onResize() events if the app is being closed
  if (is_app_state_closing())
    return;

  ResizeEvent ev(this, rc);
  onResize(ev);
}

void Widget::setBoundsQuietly(const gfx::Rect& rc)
{
  if (m_bounds != rc) {
    m_bounds = rc;

    // Remove all paint messages for this widget.
    if (Manager* man = manager())
      man->removeMessagesFor(this, kPaintMessage);
  }

  // TODO Test moving this inside the if (m_bounds != rc) { ... }
  // block, so the widget is invalidted only when the bounds are
  // really changed.
  invalidate();
}

void Widget::setBorder(const Border& br)
{
  m_border = br;

#ifdef _DEBUG
  if (m_style) {
    LOG(WARNING, "UI: %s: Warning setting border to a widget with style (%s)\n",
        typeid(*this).name(), m_style->id().c_str());
  }
#endif
}

void Widget::setChildSpacing(int childSpacing)
{
  m_childSpacing = childSpacing;

#ifdef _DEBUG
  if (m_style) {
    LOG(WARNING, "UI: %s: Warning setting child spacing to a widget with style (%s)\n",
        typeid(*this).name(), m_style->id().c_str());
  }
#endif
}

void Widget::noBorderNoChildSpacing()
{
  m_border = gfx::Border(0, 0, 0, 0);
  m_childSpacing = 0;

#ifdef _DEBUG
  if (m_style) {
    LOG(WARNING, "UI: %s: Warning setting no border to a widget with style (%s)\n",
        typeid(*this).name(), m_style->id().c_str());
  }
#endif
}

void Widget::getRegion(gfx::Region& region)
{
  if (type() == kWindowWidget)
    theme()->getWindowMask(this, region);
  else
    region = bounds();
}

void Widget::getDrawableRegion(gfx::Region& region, DrawableRegionFlags flags)
{
  Window* window = this->window();
  Display* display = this->display();

  getRegion(region);

  // Cut the top windows areas
  if (flags & kCutTopWindows) {
    const auto& uiWindows = display->getWindows();

    // Reverse iterator
    auto it = std::find(uiWindows.rbegin(),
                        uiWindows.rend(), window);

    if (!uiWindows.empty() &&
        window != uiWindows.front() &&
        it != uiWindows.rend()) {
      // Subtract the rectangles of each window
      for (++it; it != uiWindows.rend(); ++it) {
        if (!(*it)->isVisible())
          continue;

        Region reg1;
        (*it)->getRegion(reg1);
        region -= reg1;
      }
    }
  }

  // Clip the areas where are children
  if (!(flags & kUseChildArea) && !children().empty()) {
    Region reg1;
    Region reg2(childrenBounds());

    for (auto child : children()) {
      if (child->isVisible()) {
        Region reg3;
        child->getRegion(reg3);

        if (child->hasFlags(DECORATIVE)) {
          reg1 = bounds();
          reg1.createIntersection(reg1, reg3);
        }
        else {
          reg1.createIntersection(reg2, reg3);
        }
        region -= reg1;
      }
    }
  }

  // Intersect with the parent area
  if (!hasFlags(DECORATIVE)) {
    Widget* p = this->parent();
    while (p && p->type() != kManagerWidget) {
      region &= Region(p->childrenBounds());
      p = p->parent();
    }
  }
  else {
    Widget* p = parent();
    if (p) {
      region &= Region(p->bounds());
    }
  }

  // Limit to the displayable area
  View* view = View::getView(display->containedWidget());
  Rect cpos;
  if (view)
    cpos = static_cast<View*>(view)->viewportBounds();
  else
    cpos = display->containedWidget()->bounds();

  region &= Region(cpos);
}

int Widget::textWidth() const
{
  return Graphics::measureUITextLength(text().c_str(), font());
}

int Widget::textHeight() const
{
  return font()->height();
}

void Widget::getTextIconInfo(
  gfx::Rect* box,
  gfx::Rect* text,
  gfx::Rect* icon,
  int icon_align, int icon_w, int icon_h)
{
#define SETRECT(r)                              \
  if (r) {                                      \
    r->x = r##_x;                               \
    r->y = r##_y;                               \
    r->w = r##_w;                               \
    r->h = r##_h;                               \
  }

  gfx::Rect bounds = clientBounds();
  int box_x, box_y, box_w, box_h, icon_x, icon_y;
  int text_x, text_y, text_w, text_h;

  text_x = text_y = 0;

  // Size of the text
  if (hasText()) {
    text_w = textWidth();
    text_h = textHeight();
  }
  else {
    text_w = text_h = 0;
  }

  // Box size
  if (icon_align & CENTER) {   // With the icon in the center
    if (icon_align & MIDDLE) { // With the icon inside the text
      box_w = std::max(icon_w, text_w);
      box_h = std::max(icon_h, text_h);
    }
    // With the icon in the top or bottom
    else {
      box_w = std::max(icon_w, text_w);
      box_h = icon_h + (hasText() ? childSpacing(): 0) + text_h;
    }
  }
  // With the icon in left or right that doesn't care by now
  else {
    box_w = icon_w + (hasText() ? childSpacing(): 0) + text_w;
    box_h = std::max(icon_h, text_h);
  }

  // Box position
  if (align() & RIGHT)
    box_x = bounds.x2() - box_w - border().right();
  else if (align() & CENTER) {
    box_x = CALC_FOR_CENTER(bounds.x + border().top(), bounds.w - border().width(), box_w);
  }
  else
    box_x = bounds.x + border().left();

  if (align() & BOTTOM)
    box_y = bounds.y2() - box_h - border().bottom();
  else if (align() & MIDDLE) {
    box_y = CALC_FOR_CENTER(bounds.y + border().left(), bounds.h - border().height(), box_h);
  }
  else
    box_y = bounds.y + border().top();

  // With text
  if (hasText()) {
    // Text/icon X position
    if (icon_align & RIGHT) {
      text_x = box_x;
      icon_x = box_x + box_w - icon_w;
    }
    else if (icon_align & CENTER) {
      text_x = CALC_FOR_CENTER(box_x, box_w, text_w);
      icon_x = CALC_FOR_CENTER(box_x, box_w, icon_w);
    }
    else {
      text_x = box_x + box_w - text_w;
      icon_x = box_x;
    }

    // Text Y position
    if (icon_align & BOTTOM) {
      text_y = box_y;
      icon_y = box_y + box_h - icon_h;
    }
    else if (icon_align & MIDDLE) {
      text_y = CALC_FOR_CENTER(box_y, box_h, text_h);
      icon_y = CALC_FOR_CENTER(box_y, box_h, icon_h);
    }
    else {
      text_y = box_y + box_h - text_h;
      icon_y = box_y;
    }
  }
  // Without text
  else {
    // Icon X/Y position
    icon_x = box_x;
    icon_y = box_y;
  }

  SETRECT(box);
  SETRECT(text);
  SETRECT(icon);
}

void Widget::setMinSize(const gfx::Size& sz)
{
  ASSERT(sz.w <= m_maxSize.w);
  ASSERT(sz.h <= m_maxSize.h);
  m_minSize = sz;
}

void Widget::setMaxSize(const gfx::Size& sz)
{
  ASSERT(sz.w >= m_minSize.w);
  ASSERT(sz.h >= m_minSize.h);
  m_maxSize = sz;
}

void Widget::setMinMaxSize(const gfx::Size& minSz,
                           const gfx::Size& maxSz)
{
  ASSERT(minSz.w <= maxSz.w);
  ASSERT(minSz.h <= maxSz.h);
  m_minSize = minSz;
  m_maxSize = maxSz;
}

void Widget::resetMinSize()
{
  m_minSize = gfx::Size(0, 0);
}

void Widget::resetMaxSize()
{
  m_maxSize = gfx::Size(std::numeric_limits<int>::max(),
                        std::numeric_limits<int>::max());
}

void Widget::flushRedraw()
{
  std::queue<Widget*> processing;
  Message* msg;

  if (hasFlags(DIRTY)) {
    disableFlags(DIRTY);
    processing.push(this);
  }

  Manager* manager = this->manager();
  ASSERT(manager);
  if (!manager)
    return;

  while (!processing.empty()) {
    Widget* widget = processing.front();
    processing.pop();

    ASSERT_VALID_WIDGET(widget);

    // If the widget is hidden
    if (!widget->isVisible())
      continue;

    for (auto child : widget->children()) {
      if (child->hasFlags(DIRTY)) {
        child->disableFlags(DIRTY);
        processing.push(child);
      }
    }

    if (!widget->m_updateRegion.isEmpty()) {
      // Intersect m_updateRegion with drawable area.
      {
        Region drawable;
        widget->getDrawableRegion(drawable, kCutTopWindows);
        widget->m_updateRegion &= drawable;
      }

      std::size_t c, nrects = widget->m_updateRegion.size();
      Region::const_iterator it = widget->m_updateRegion.begin();

      // Draw the widget
      Display* display = widget->display();
      int count = nrects-1;
      for (c=0; c<nrects; ++c, ++it, --count) {
        // Create the draw message
        msg = new PaintMessage(count, *it);
        msg->setDisplay(display);
        msg->setRecipient(widget);

        // Enqueue the draw message
        manager->enqueueMessage(msg);
      }

      display->addInvalidRegion(widget->m_updateRegion);
      widget->m_updateRegion.clear();
    }
  }
}

void Widget::paint(Graphics* graphics,
                   const gfx::Region& drawRegion,
                   const bool isBg)
{
  if (drawRegion.isEmpty())
    return;

  std::queue<Widget*> processing;
  processing.push(this);

  Display* display = this->display();

  while (!processing.empty()) {
    Widget* widget = processing.front();
    processing.pop();

    ASSERT_VALID_WIDGET(widget);

    // If the widget is hidden
    if (!widget->isVisible())
      continue;

    for (auto child : widget->children())
      processing.push(child);

    // Intersect drawRegion with widget's drawable region.
    Region region;
    widget->getDrawableRegion(region, kCutTopWindows);
    region.createIntersection(region, drawRegion);

    Graphics graphics2(
      display,
      base::AddRef(graphics->getInternalSurface()),
      widget->bounds().x,
      widget->bounds().y);
    graphics2.setFont(AddRef(widget->font()));

    for (const gfx::Rect& rc : region) {
      IntersectClip clip(&graphics2,
                         Rect(rc).offset(
                           -widget->bounds().x,
                           -widget->bounds().y));
      widget->paintEvent(&graphics2, isBg);
    }
  }
}

bool Widget::paintEvent(Graphics* graphics,
                        const bool isBg)
{
  // For transparent widgets we have to draw the parent first.
  if (isTransparent()) {
#if _DEBUG
    // In debug mode we can fill the area with Red so we know if the
    // we are drawing the parent correctly.
    if (window() && !window()->ownDisplay())
      graphics->fillRect(gfx::rgba(255, 0, 0), clientBounds());
#endif

    enableFlags(HIDDEN);

    Widget* parentWidget = nullptr;
    if (type() == kWindowWidget) {
      parentWidget = display()->containedWidget();

      if (get_multiple_displays()) {
        // Draw the desktop window as the background, not the manager
        // (as the manager contains all windows as children and will
        // try to draw other foreground windows here).
        if (parentWidget->type() == kManagerWidget) {
          parentWidget = static_cast<Manager*>(parentWidget)->getDesktopWindow();
        }
        // If this ui::Window has it's own native window (os::Window),
        // we'll fill a transparent rectangle because we already have
        // a transparent native window.
        else if (static_cast<Window*>(this)->ownDisplay()) {
          parentWidget = nullptr;
        }
      }
    }
    else {
      parentWidget = parent();
    }
    if (parentWidget) {
      gfx::Region rgn(parentWidget->bounds());
      rgn &= gfx::Region(
        graphics->getClipBounds().offset(
          graphics->getInternalDeltaX(),
          graphics->getInternalDeltaY()));
      parentWidget->paint(graphics, rgn, true);
    }
    else {
      // Clear surface with transparent color
      os::Paint p;
      p.color(gfx::rgba(0, 0, 0, 0));
      p.blendMode(os::BlendMode::Src);
      graphics->drawRect(clientBounds(), p);
    }

    disableFlags(HIDDEN);
  }

  PaintEvent ev(this, graphics);
  ev.setTransparentBg(isBg);
  onPaint(ev); // Fire onPaint event
  return ev.isPainted();
}

bool Widget::isDoubleBuffered() const
{
  return hasFlags(DOUBLE_BUFFERED);
}

void Widget::setDoubleBuffered(bool doubleBuffered)
{
  enableFlags(DOUBLE_BUFFERED);
}

bool Widget::isTransparent() const
{
  return hasFlags(TRANSPARENT);
}

void Widget::setTransparent(bool transparent)
{
  if (transparent)
    enableFlags(TRANSPARENT);
  else
    disableFlags(TRANSPARENT);
}

void Widget::invalidate()
{
  assert_ui_thread();
  if (!hasFlags(HIDDEN))        // Quick filter for hidden widgets
    onInvalidateRegion(Region(bounds()));
}

void Widget::invalidateRect(const gfx::Rect& rect)
{
  assert_ui_thread();
  if (!hasFlags(HIDDEN))        // Quick filter for hidden widgets
    onInvalidateRegion(Region(rect));
}

void Widget::invalidateRegion(const Region& region)
{
  assert_ui_thread();
  if (!hasFlags(HIDDEN))        // Quick filter for hidden widgets
    onInvalidateRegion(region);
}

class DeleteGraphicsAndSurface {
public:
  DeleteGraphicsAndSurface(const gfx::Rect& clip,
                           const os::SurfaceRef& surface,
                           os::SurfaceRef& dst)
    : m_pt(clip.origin())
    , m_surface(surface)
    , m_dst(dst) {
  }

  void operator()(Graphics* graphics) {
    {
      os::SurfaceLock lockSrc(m_surface.get());
      os::SurfaceLock lockDst(m_dst.get());
      m_surface->blitTo(
        m_dst.get(), 0, 0, m_pt.x, m_pt.y,
        m_surface->width(), m_surface->height());
    }
    m_surface.reset();
    delete graphics;
  }

private:
  gfx::Point m_pt;
  os::SurfaceRef m_surface;
  os::SurfaceRef m_dst;
};

GraphicsPtr Widget::getGraphics(const gfx::Rect& clip)
{
  GraphicsPtr graphics;
  Display* display = this->display();
  os::SurfaceRef dstSurface = AddRef(display->surface());

  // In case of double-buffering, we need to create the temporary
  // buffer only if the default surface is the screen.
  if (isDoubleBuffered() && dstSurface->isDirectToScreen()) {
    os::SurfaceRef surface =
      os::instance()->makeSurface(clip.w, clip.h);
    graphics.reset(new Graphics(display, surface, -clip.x, -clip.y),
                   DeleteGraphicsAndSurface(clip, surface,
                                            dstSurface));
  }
  // In other case, we can draw directly onto the screen.
  else {
    graphics.reset(new Graphics(display, dstSurface, bounds().x, bounds().y));
  }

  graphics->setFont(AddRef(font()));
  return graphics;
}

// ===============================================================
// GUI MANAGER
// ===============================================================

bool Widget::sendMessage(Message* msg)
{
  ASSERT(msg);
  return onProcessMessage(msg);
}

void Widget::closeWindow()
{
  if (Window* w = window())
    w->closeWindow(this);
}

void Widget::broadcastMouseMessage(const gfx::Point& screenPos,
                                   WidgetsList& targets)
{
  onBroadcastMouseMessage(screenPos, targets);
}

// ===============================================================
// SIZE & POSITION
// ===============================================================

/**
   Returns the preferred size of the Widget.

   It checks if the preferred size is static (it means when it was
   set through #setSizeHint before) or if it is dynamic (this is
   the default and is when the #onSizeHint is used to determined
   the preferred size).

   In another words, if you do not use #setSizeHint to set a
   <em>static preferred size</em> for the widget then #onSizeHint
   will be used to calculate it.

   @see setSizeHint, onSizeHint, #sizeHint(const Size &)
*/
Size Widget::sizeHint()
{
  if (m_sizeHint)
    return *m_sizeHint;
  else {
    SizeHintEvent ev(this, Size(0, 0));
    onSizeHint(ev);

    Size sz(ev.sizeHint());
    sz.w = std::clamp(sz.w, m_minSize.w, m_maxSize.w);
    sz.h = std::clamp(sz.h, m_minSize.h, m_maxSize.h);
    return sz;
  }
}

/**
   Returns the preferred size trying to fit in the specified size.
   Remember that if you use #setSizeHint this routine will
   return the static size which you specified manually.

   @param fitIn
       This can have both attributes (width and height) in
       zero, which means that it'll behave same as #sizeHint().
       If the width is great than zero the #onSizeHint will try to
       fit in that width (this is useful to fit Label or Edit controls
       in a specified width and calculate the height it could occupy).

   @see sizeHint
*/
Size Widget::sizeHint(const Size& fitIn)
{
  if (m_sizeHint)
    return *m_sizeHint;
  else {
    SizeHintEvent ev(this, fitIn);
    onSizeHint(ev);

    Size sz(ev.sizeHint());
    sz.w = std::clamp(sz.w, m_minSize.w, m_maxSize.w);
    sz.h = std::clamp(sz.h, m_minSize.h, m_maxSize.h);
    return sz;
  }
}

/**
   Sets a fixed preferred size specified by the user.
   Widget::sizeHint() will return this value if it's setted.
*/
void Widget::setSizeHint(const Size& fixedSize)
{
  delete m_sizeHint;
  m_sizeHint = new Size(fixedSize);
}

void Widget::setSizeHint(int fixedWidth, int fixedHeight)
{
  setSizeHint(Size(fixedWidth, fixedHeight));
}

void Widget::resetSizeHint()
{
  if (m_sizeHint) {
    delete m_sizeHint;
    m_sizeHint = nullptr;
  }
}

// ===============================================================
// FOCUS & MOUSE
// ===============================================================

void Widget::requestFocus()
{
  if (auto man = manager())
    man->setFocus(this);
}

void Widget::releaseFocus()
{
  if (hasFocus()) {
    if (auto man = manager())
      man->freeFocus();
  }
}

// Captures the mouse to send all the future mouse messsages to the
// specified widget (included the kMouseMoveMessage and kSetCursorMessage).
void Widget::captureMouse()
{
  if (auto man = manager()) {
    if (!man->getCapture()) {
      man->setCapture(this);
    }
  }
}

// Releases the capture of the mouse events.
void Widget::releaseMouse()
{
  if (auto man = manager()) {
    if (man->getCapture() == this) {
      man->freeCapture();
    }
  }
}

bool Widget::offerCapture(ui::MouseMessage* mouseMsg, int widget_type)
{
  if (hasCapture()) {
    const gfx::Point screenPos = mouseMsg->display()->nativeWindow()->pointToScreen(mouseMsg->position());
    auto man = manager();
    Widget* pick = (man ? man->pickFromScreenPos(screenPos): nullptr);
    if (pick && pick != this && pick->type() == widget_type) {
      releaseMouse();

      MouseMessage* mouseMsg2 = new MouseMessage(
        kMouseDownMessage,
        *mouseMsg,
        mouseMsg->positionForDisplay(pick->display()));
      mouseMsg2->setDisplay(pick->display());
      mouseMsg2->setRecipient(pick);
      man->enqueueMessage(mouseMsg2);
      return true;
    }
  }
  return false;
}

gfx::Point Widget::mousePosInDisplay() const
{
  return display()->nativeWindow()->pointFromScreen(get_mouse_position());
}

void Widget::setMnemonic(const int mnemonic,
                         const bool requireModifiers)
{
  static_assert((kMnemonicCharMask & kMnemonicModifiersMask) == 0);
  ASSERT((mnemonic & kMnemonicModifiersMask) == 0);
  m_mnemonic =
    (mnemonic & kMnemonicCharMask) |
    (requireModifiers ? kMnemonicModifiersMask: 0);
}

void Widget::processMnemonicFromText(const int escapeChar,
                                     const bool requireModifiers)
{
  // Avoid calling setText() when the widget doesn't have the HAS_TEXT flag
  if (!hasText())
    return;

  std::wstring newText; // wstring is used to properly push_back() multibyte chars
  if (!m_text.empty())
    newText.reserve(m_text.size());

  base::utf8_decode decode(m_text);
  while (int chr = decode.next()) {
    if (chr == escapeChar) {
      chr = decode.next();
      if (!chr) {
        break;    // Ill-formed string (it ends with escape character)
      }
      else if (chr != escapeChar) {
        setMnemonic(chr, requireModifiers);
      }
    }
    newText.push_back(chr);
  }

  setText(base::to_utf8(newText));
}

bool Widget::isMnemonicPressed(const KeyMessage* keyMsg) const
{
  int chr = std::tolower(mnemonic());
  return
    ((chr) &&
     ((chr == std::tolower(keyMsg->unicodeChar())) ||
      (chr >= 'a' && chr <= 'z' && keyMsg->scancode() == (kKeyA + chr - 'a')) ||
      (chr >= '0' && chr <= '9' && keyMsg->scancode() == (kKey0 + chr - '0'))));
}

bool Widget::onProcessMessage(Message* msg)
{
  ASSERT(msg != nullptr);

  switch (msg->type()) {

    case kOpenMessage:
    case kCloseMessage:
    case kWinMoveMessage:
      // Broadcast the message to the children.
      for (auto child : m_children)
        child->sendMessage(msg);
      break;

    case kPaintMessage: {
      const PaintMessage* ptmsg = static_cast<const PaintMessage*>(msg);
      ASSERT(ptmsg->rect().w > 0);
      ASSERT(ptmsg->rect().h > 0);

      GraphicsPtr graphics = getGraphics(toClient(ptmsg->rect()));
      return paintEvent(graphics.get(), false);
    }

    case kDoubleClickMessage: {
      // Convert double clicks into mouse down
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      MouseMessage mouseMsg2(kMouseDownMessage,
                             *mouseMsg,
                             mouseMsg->position());
      mouseMsg2.setRecipient(this);
      mouseMsg2.setDisplay(mouseMsg->display());
      sendMessage(&mouseMsg2);
      break;
    }

    case kMouseDownMessage:
    case kMouseUpMessage:
    case kMouseMoveMessage:
    case kMouseWheelMessage:
      // Propagate the message to the parent.
      if (parent())
        return parent()->sendMessage(msg);
      else
        break;

    case kSetCursorMessage:
      // Propagate the message to the parent.
      if (parent())
        return parent()->sendMessage(msg);
      else {
        set_mouse_cursor(kArrowCursor);
        return true;
      }
      break;

  }

  // Broadcast the message to the children.
  if (msg->propagateToChildren()) {
    for (auto child : m_children)
      if (child->sendMessage(msg))
        return true;
  }

  // Propagate the message to the parent.
  if (msg->propagateToParent() && parent() &&
      msg->commonAncestor() != parent()) {
    return parent()->sendMessage(msg);
  }

  return false;
}

// ===============================================================
// EVENTS
// ===============================================================

void Widget::onInvalidateRegion(const Region& region)
{
  if (!isVisible() || region.contains(bounds()) == Region::Out)
    return;

  Region reg1;
  reg1.createUnion(m_updateRegion, region);
  {
    Region reg2;
    getDrawableRegion(reg2, kCutTopWindows);
    m_updateRegion.createIntersection(reg1, reg2);
  }
  reg1.createSubtraction(region, m_updateRegion);

  setDirtyFlag();

  for (auto child : m_children)
    child->invalidateRegion(reg1);
}

void Widget::onSizeHint(SizeHintEvent& ev)
{
  if (m_style) {
    ev.setSizeHint(m_theme->calcSizeHint(this, style()));
  }
  else {
    ev.setSizeHint(m_minSize);
  }
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
  setBoundsQuietly(ev.bounds());

  // Set all the children to the same "cpos".
  gfx::Rect cpos = childrenBounds();
  for (auto child : m_children)
    child->setBounds(cpos);
}

void Widget::onPaint(PaintEvent& ev)
{
  if (m_style)
    m_theme->paintWidget(ev.graphics(), this, style(),
                         clientBounds());
}

void Widget::onBroadcastMouseMessage(const gfx::Point& screenPos,
                                     WidgetsList& targets)
{
  // Do nothing
}

void Widget::onInitTheme(InitThemeEvent& ev)
{
  // Create a copy of the children list and iterate it, just in case a
  // initTheme() modifies this list (e.g. this can happen in some
  // strange cases with viewports, where scrollbars are added/removed
  // while we init the theme if the UI scale changes).
  auto children = m_children;
  for (auto child : children)
    child->initTheme();

  if (m_theme) {
    m_theme->initWidget(this);

    if (!hasFlags(INITIALIZED))
      enableFlags(INITIALIZED);

    InitTheme();
  }
}

void Widget::onSetDecorativeWidgetBounds()
{
  if (m_theme)
    m_theme->setDecorativeWidgetBounds(this);
}

void Widget::onVisible(bool visible)
{
  // Do nothing
}

void Widget::onEnable(bool enabled)
{
  // Do nothing
}

void Widget::onSelect(bool selected)
{
  // Do nothing
}

void Widget::onSetText()
{
  invalidate();
}

void Widget::onSetBgColor()
{
  invalidate();
}

int Widget::onGetTextInt() const
{
  return std::strtol(m_text.c_str(), nullptr, 10);
}

double Widget::onGetTextDouble() const
{
  return std::strtod(m_text.c_str(), nullptr);
}

void Widget::offsetWidgets(int dx, int dy)
{
  if (dx == 0 && dy == 0)
    return;

  m_updateRegion.offset(dx, dy);
  m_bounds.offset(dx, dy);

  // Remove all paint messages for this widget.
  if (auto man = manager())
    man->removeMessagesFor(this, kPaintMessage);

  for (auto child : m_children)
    child->offsetWidgets(dx, dy);
}

void Widget::setDirtyFlag()
{
  Widget* widget = this;
  while (widget) {
    widget->enableFlags(DIRTY);
    widget = widget->parent();
  }
}

} // namespace ui
