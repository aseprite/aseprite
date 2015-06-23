// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "ui/intern.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/resize_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

#define HBAR_SIZE (m_scrollbar_h.getBarWidth())
#define VBAR_SIZE (m_scrollbar_v.getBarWidth())

namespace ui {

using namespace gfx;

View::View()
  : Widget(kViewWidget)
  , m_scrollbar_h(HORIZONTAL)
  , m_scrollbar_v(VERTICAL)
{
  m_hasBars = true;

  this->setFocusStop(true);
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
  return UI_FIRST_WIDGET(m_viewport.getChildren());
}

void View::makeVisibleAllScrollableArea()
{
  Size reqSize = m_viewport.calculateNeededSize();

  this->min_w =
    + this->border_width.l
    + m_viewport.border_width.l
    + reqSize.w
    + m_viewport.border_width.r
    + this->border_width.r;

  this->min_h =
    + this->border_width.t
    + m_viewport.border_width.t
    + reqSize.h
    + m_viewport.border_width.b
    + this->border_width.b;
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
  return Size(m_scrollbar_h.getSize(),
              m_scrollbar_v.getSize());
}

void View::setScrollableSize(const Size& sz)
{
#define CHECK(w, h, l, t, r, b)                                 \
  ((sz.w > (m_viewport.getBounds().w                            \
            - m_viewport.border_width.l                         \
            - m_viewport.border_width.r)) &&                    \
   (VBAR_SIZE < pos.w) && (HBAR_SIZE < pos.h))

  m_scrollbar_h.setSize(sz.w);
  m_scrollbar_v.setSize(sz.h);

  gfx::Rect pos = getChildrenBounds();

  // Setup scroll-bars
  if (m_scrollbar_h.getParent()) removeChild(&m_scrollbar_h);
  if (m_scrollbar_v.getParent()) removeChild(&m_scrollbar_v);

  if (m_hasBars) {
    if (CHECK(w, h, l, t, r, b)) {
      pos.h -= HBAR_SIZE;
      addChild(&m_scrollbar_h);

      if (CHECK(h, w, t, l, b, r)) {
        pos.w -= VBAR_SIZE;
        if (CHECK(w, h, l, t, r, b))
          addChild(&m_scrollbar_v);
        else {
          pos.w += VBAR_SIZE;
          pos.h += HBAR_SIZE;
          removeChild(&m_scrollbar_h);
        }
      }
    }
    else if (CHECK(h, w, t, l, b, r)) {
      pos.w -= VBAR_SIZE;
      addChild(&m_scrollbar_v);

      if (CHECK(w, h, l, t, r, b)) {
        pos.h -= HBAR_SIZE;
        if (CHECK(h, w, t, l, b, r))
          addChild(&m_scrollbar_h);
        else {
          pos.w += VBAR_SIZE;
          pos.h += HBAR_SIZE;
          removeChild(&m_scrollbar_v);
        }
      }
    }

    if (hasChild(&m_scrollbar_h)) {
      m_scrollbar_h.setBounds(gfx::Rect(pos.x, pos.y2(), pos.w, HBAR_SIZE));
      m_scrollbar_h.setVisible(true);
    }
    else
      m_scrollbar_h.setVisible(false);

    if (hasChild(&m_scrollbar_v)) {
      m_scrollbar_v.setBounds(gfx::Rect(pos.x2(), pos.y, VBAR_SIZE, pos.h));
      m_scrollbar_v.setVisible(true);
    }
    else
      m_scrollbar_v.setVisible(false);
  }

  // Setup viewport
  invalidate();
  m_viewport.setBounds(pos);
  setViewScroll(getViewScroll()); // Setup the same scroll-point
}

Size View::getVisibleSize()
{
  return Size(m_viewport.getBounds().w - m_viewport.border_width.l - m_viewport.border_width.r,
              m_viewport.getBounds().h - m_viewport.border_width.t - m_viewport.border_width.b);
}

Point View::getViewScroll()
{
  return Point(m_scrollbar_h.getPos(),
               m_scrollbar_v.getPos());
}

void View::setViewScroll(const Point& pt)
{
  Point oldScroll = getViewScroll();
  Size maxsize = getScrollableSize();
  Size visible = getVisibleSize();
  Point newScroll(MID(0, pt.x, MAX(0, maxsize.w - visible.w)),
                  MID(0, pt.y, MAX(0, maxsize.h - visible.h)));

  if (newScroll == oldScroll)
    return;

  m_scrollbar_h.setPos(newScroll.x);
  m_scrollbar_v.setPos(newScroll.y);

  m_viewport.layout();

  onScrollChange();
}

void View::updateView()
{
  Widget* vw = UI_FIRST_WIDGET(m_viewport.getChildren());
  Point scroll = getViewScroll();

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

Viewport* View::getViewport()
{
  return &m_viewport;
}

Rect View::getViewportBounds()
{
  return m_viewport.getBounds() - m_viewport.getBorder();
}

// static
View* View::getView(Widget* widget)
{
  if ((widget->getParent()) &&
      (widget->getParent()->type == kViewViewportWidget) &&
      (widget->getParent()->getParent()) &&
      (widget->getParent()->getParent()->type == kViewWidget))
    return static_cast<View*>(widget->getParent()->getParent());
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

void View::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());
  updateView();
}

void View::onPreferredSize(PreferredSizeEvent& ev)
{
  Size viewSize = m_viewport.getPreferredSize();
  viewSize.w += this->border_width.l + this->border_width.r;
  viewSize.h += this->border_width.t + this->border_width.b;
  ev.setPreferredSize(viewSize);
}

void View::onPaint(PaintEvent& ev)
{
  getTheme()->paintView(ev);
}

void View::onScrollChange()
{
  // Do nothing
}

} // namespace ui
