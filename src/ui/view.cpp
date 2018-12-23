// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

// #define DEBUG_SCROLL_EVENTS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "ui/intern.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/move_region.h"
#include "ui/resize_event.h"
#include "ui/scroll_helper.h"
#include "ui/scroll_region_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

#ifdef DEBUG_SCROLL_EVENTS
#include "base/thread.h"
#include "os/display.h"
#include "os/surface.h"
#endif

#include <queue>

#define HBAR_SIZE (m_scrollbar_h.getBarWidth())
#define VBAR_SIZE (m_scrollbar_v.getBarWidth())

namespace ui {

using namespace gfx;

View::View()
  : Widget(kViewWidget)
  , m_scrollbar_h(HORIZONTAL, this)
  , m_scrollbar_v(VERTICAL, this)
{
  m_hasBars = true;

  enableFlags(IGNORE_MOUSE);
  setFocusStop(true);
  addChild(&m_viewport);
  setScrollableSize(Size(0, 0));

  initTheme();
}

bool View::hasScrollBars()
{
  return m_hasBars;
}

void View::attachToView(Widget* viewable_widget)
{
  m_viewport.addChild(viewable_widget);
}

Widget* View::attachedWidget()
{
  return UI_FIRST_WIDGET(m_viewport.children());
}

void View::makeVisibleAllScrollableArea()
{
  Size reqSize = m_viewport.calculateNeededSize();

  setMinSize(
    gfx::Size(
      + reqSize.w
      + m_viewport.border().width()
      + border().width(),

      + reqSize.h
      + m_viewport.border().height()
      + border().height()));
}

void View::hideScrollBars()
{
  m_hasBars = false;
  updateView();
}

void View::showScrollBars()
{
  m_hasBars = true;
  updateView();
}

Size View::getScrollableSize()
{
  return Size(m_scrollbar_h.size(),
              m_scrollbar_v.size());
}

void View::setScrollableSize(const Size& sz)
{
  gfx::Rect viewportArea = childrenBounds();

  if (m_hasBars) {
    setup_scrollbars(sz,
                     viewportArea,
                     *this,
                     m_scrollbar_h,
                     m_scrollbar_v);
  }
  else {
    if (m_scrollbar_h.parent()) removeChild(&m_scrollbar_h);
    if (m_scrollbar_v.parent()) removeChild(&m_scrollbar_v);
    m_scrollbar_h.setVisible(false);
    m_scrollbar_v.setVisible(false);
    m_scrollbar_h.setSize(sz.w);
    m_scrollbar_v.setSize(sz.h);
  }

  // Setup viewport
  invalidate();
  m_viewport.setBounds(viewportArea);
  setViewScroll(viewScroll()); // Setup the same scroll-point
}

Size View::visibleSize() const
{
  return Size(m_viewport.bounds().w - m_viewport.border().width(),
              m_viewport.bounds().h - m_viewport.border().height());
}

Point View::viewScroll() const
{
  return Point(m_scrollbar_h.getPos(),
               m_scrollbar_v.getPos());
}

void View::setViewScroll(const Point& pt)
{
  onSetViewScroll(pt);
}

void View::updateView()
{
  Widget* vw = UI_FIRST_WIDGET(m_viewport.children());
  Point scroll = viewScroll();

  // Set minimum (remove scroll-bars)
  setScrollableSize(Size(0, 0));

  // Set needed size
  setScrollableSize(m_viewport.calculateNeededSize());

  // If there are scroll-bars, we have to setup the scrollable-size
  // again (because they remove visible space, maybe now we need a
  // vertical or horizontal bar too).
  if (hasChild(&m_scrollbar_h) || hasChild(&m_scrollbar_v))
    setScrollableSize(m_viewport.calculateNeededSize());

  if (vw)
    setViewScroll(scroll);
  else
    setViewScroll(Point(0, 0));
}

Viewport* View::viewport()
{
  return &m_viewport;
}

Rect View::viewportBounds()
{
  return m_viewport.bounds() - m_viewport.border();
}

// static
View* View::getView(Widget* widget)
{
  if ((widget->parent()) &&
      (widget->parent()->type() == kViewViewportWidget) &&
      (widget->parent()->parent()) &&
      (widget->parent()->parent()->type() == kViewWidget))
    return static_cast<View*>(widget->parent()->parent());
  else
    return 0;
}

bool View::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
    case kFocusLeaveMessage:
      // TODO This is theme specific stuff
      // Redraw the borders each time the focus enters or leaves the view.
      {
        Region region;
        getDrawableRegion(region, kCutTopWindows);
        invalidateRegion(region);
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

void View::onInitTheme(InitThemeEvent& ev)
{
  m_viewport.initTheme();
  m_scrollbar_h.initTheme();
  m_scrollbar_v.initTheme();

  Widget::onInitTheme(ev);
}

void View::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());
  updateView();
}

void View::onSizeHint(SizeHintEvent& ev)
{
  Widget::onSizeHint(ev);
  gfx::Size sz = ev.sizeHint();
  sz += m_viewport.sizeHint();
  ev.setSizeHint(sz);
}

void View::onSetViewScroll(const gfx::Point& pt)
{
  // If the view is not visible, we don't adjust any screen region.
  if (!isVisible())
    return;

  Point oldScroll = viewScroll();
  Size maxsize = getScrollableSize();
  Size visible = visibleSize();
  Point newScroll(MID(0, pt.x, MAX(0, maxsize.w - visible.w)),
                  MID(0, pt.y, MAX(0, maxsize.h - visible.h)));

  if (newScroll == oldScroll)
    return;

  // This is the movement for the scrolled region (which is inverse to
  // the scroll position delta/movement).
  Point delta = oldScroll - newScroll;

  // Visible viewport region that is not overlapped by windows
  Region drawableRegion;
  m_viewport.getDrawableRegion(
    drawableRegion, DrawableRegionFlags(kCutTopWindows | kUseChildArea));

  // Start the region to scroll equal to the drawable viewport region.
  Rect cpos = m_viewport.childrenBounds();
  Region validRegion(cpos);
  validRegion &= drawableRegion;

  // Remove all children invalid regions from this "validRegion"
  {
    std::queue<Widget*> items;
    items.push(&m_viewport);
    while (!items.empty()) {
      Widget* item = items.front();
      items.pop();
      for (Widget* child : item->children())
        items.push(child);

      if (item->isVisible())
        validRegion -= item->getUpdateRegion();
    }
  }

  // Remove invalid region in the screen (areas that weren't
  // re-painted yet)
  Manager* manager = this->manager();
  if (manager)
    validRegion -= manager->getInvalidRegion();

  // Add extra regions that cannot be scrolled (this can be customized
  // by subclassing ui::View). We use two ScrollRegionEvent, this
  // first one with the old scroll position. And the next one with the
  // new scroll position.
  {
    ScrollRegionEvent ev(this, validRegion);
    onScrollRegion(ev);
  }

  // Move viewport children
  cpos.offset(-newScroll);
  for (auto child : m_viewport.children()) {
    Size reqSize = child->sizeHint();
    cpos.w = MAX(reqSize.w, cpos.w);
    cpos.h = MAX(reqSize.h, cpos.h);
    if (cpos.w != child->bounds().w ||
        cpos.h != child->bounds().h)
      child->setBounds(cpos);
    else
      child->offsetWidgets(cpos.x - child->bounds().x,
                           cpos.y - child->bounds().y);
  }

  // Change scroll bar positions
  m_scrollbar_h.setPos(newScroll.x);
  m_scrollbar_v.setPos(newScroll.y);

  // Region to invalidate (new visible children/child parts)
  Region invalidRegion(cpos);
  invalidRegion &= drawableRegion;

  // Move the valid screen region.
  {
    // The movable region includes the given "validRegion"
    // intersecting itself when it's in the new position, so we don't
    // overlap regions outside the "validRegion".
    Region movable = validRegion;
    movable.offset(delta);
    movable &= validRegion;
    invalidRegion -= movable;   // Remove the moved region as invalid
    movable.offset(-delta);

    ui::move_region(manager, movable, delta.x, delta.y);
  }

#ifdef DEBUG_SCROLL_EVENTS
  // Paint invalid region with red fill
  {
    auto display = manager->getDisplay();
    if (display)
      display->flip(gfx::Rect(0, 0, display_w(), display_h()));
    base::this_thread::sleep_for(0.002);
    {
      os::Surface* surface = display->getSurface();
      os::SurfaceLock lock(surface);
      for (const auto& rc : invalidRegion)
        surface->fillRect(gfx::rgba(255, 0, 0), rc);
    }
    if (display)
      display->flip(gfx::Rect(0, 0, display_w(), display_h()));
    base::this_thread::sleep_for(0.002);
  }
#endif

  // Invalidate viewport's children regions
  m_viewport.invalidateRegion(invalidRegion);

  // Notify about the new scroll position
  onScrollChange();
}

void View::onScrollRegion(ScrollRegionEvent& ev)
{
  if (auto viewable = dynamic_cast<ViewableWidget*>(attachedWidget()))
    viewable->onScrollRegion(ev);
}

void View::onScrollChange()
{
  // Do nothing
}

} // namespace ui
