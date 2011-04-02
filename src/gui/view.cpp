// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gfx/size.h"
#include "gui/intern.h"
#include "gui/list.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/region.h"
#include "gui/system.h"
#include "gui/theme.h"
#include "gui/view.h"
#include "gui/widget.h"

#define BAR_SIZE getTheme()->scrollbar_size

using namespace gfx;

View::View()
  : Widget(JI_VIEW)
  , m_scrollbar_h(JI_HORIZONTAL)
  , m_scrollbar_v(JI_VERTICAL)
{
  m_hasBars = true;

  jwidget_focusrest(this, true);
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
  /* TODO */
  /* jwidget_emit_signal(this, JI_SIGNAL_VIEW_ATTACH); */
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

Size View::getScrollableSize()
{
  return Size(m_scrollbar_h.getSize(),
	      m_scrollbar_v.getSize());
}

void View::setScrollableSize(const Size& sz)
{
#define CHECK(w, h, l, t, r, b)					\
  ((sz.w > jrect_##w(m_viewport.rc)				\
                    - m_viewport.border_width.l			\
		    - m_viewport.border_width.r) &&		\
   (BAR_SIZE < jrect_##w(pos)) && (BAR_SIZE < jrect_##h(pos)))

  JRect pos, rect;

  m_scrollbar_h.setSize(sz.w);
  m_scrollbar_v.setSize(sz.h);

  pos = jwidget_get_child_rect(this);

  // Setup scroll-bars
  removeChild(&m_scrollbar_h);
  removeChild(&m_scrollbar_v);

  if (m_hasBars) {
    if (CHECK(w, h, l, t, r, b)) {
      pos->y2 -= BAR_SIZE;
      addChild(&m_scrollbar_h);

      if (CHECK(h, w, t, l, b, r)) {
	pos->x2 -= BAR_SIZE;
	if (CHECK(w, h, l, t, r, b))
	  addChild(&m_scrollbar_v);
	else {
	  pos->x2 += BAR_SIZE;
	  pos->y2 += BAR_SIZE;
	  removeChild(&m_scrollbar_h);
	}
      }
    }
    else if (CHECK(h, w, t, l, b, r)) {
      pos->x2 -= BAR_SIZE;
      addChild(&m_scrollbar_v);

      if (CHECK(w, h, l, t, r, b)) {
        pos->y2 -= BAR_SIZE;
	if (CHECK(h, w, t, l, b, r))
	  addChild(&m_scrollbar_h);
	else {
	  pos->x2 += BAR_SIZE;
	  pos->y2 += BAR_SIZE;
	  removeChild(&m_scrollbar_v);
	}
      }
    }

    if (hasChild(&m_scrollbar_h)) {
      rect = jrect_new(pos->x1, pos->y2,
		       pos->x1+jrect_w(pos), pos->y2+BAR_SIZE);
      jwidget_set_rect(&m_scrollbar_h, rect);
      jrect_free(rect);

      m_scrollbar_h.setVisible(true);
    }
    else
      m_scrollbar_h.setVisible(false);

    if (hasChild(&m_scrollbar_v)) {
      rect = jrect_new(pos->x2, pos->y1,
		       pos->x2+BAR_SIZE, pos->y1+jrect_h(pos));
      jwidget_set_rect(&m_scrollbar_v, rect);
      jrect_free(rect);

      m_scrollbar_v.setVisible(true);
    }
    else
      m_scrollbar_v.setVisible(false);
  }

  // Setup viewport
  invalidate();
  jwidget_set_rect(&m_viewport, pos);
  setViewScroll(getViewScroll()); // Setup the same scroll-point

  jrect_free(pos);
}

Size View::getVisibleSize()
{
  return Size(jrect_w(m_viewport.rc) - m_viewport.border_width.l - m_viewport.border_width.r,
	      jrect_h(m_viewport.rc) - m_viewport.border_width.t - m_viewport.border_width.b);
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

  jwidget_set_rect(&m_viewport, m_viewport.rc);
  invalidate();
}

void View::updateView()
{
  Widget* vw = reinterpret_cast<Widget*>(jlist_first_data(m_viewport.children));
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
      (widget->getParent()->type == JI_VIEW_VIEWPORT) &&
      (widget->getParent()->getParent()) &&
      (widget->getParent()->getParent()->type == JI_VIEW))
    return static_cast<View*>(widget->getParent()->getParent());
  else
    return 0;
}

bool View::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      Size viewSize = m_viewport.getPreferredSize();
      msg->reqsize.w = viewSize.w;
      msg->reqsize.h = viewSize.h;

      msg->reqsize.w += this->border_width.l + this->border_width.r;
      msg->reqsize.h += this->border_width.t + this->border_width.b;
      return true;
    }

    case JM_SETPOS:
      jrect_copy(this->rc, &msg->setpos.rect);
      updateView();
      return true;

    case JM_DRAW:
      getTheme()->draw_view(this, &msg->draw.rect);
      return true;

    case JM_FOCUSENTER:
    case JM_FOCUSLEAVE:
      /* TODO add something to avoid this (theme specific stuff) */
      {
	JRegion reg1 = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
	jregion_union(this->update_region, this->update_region, reg1);
	jregion_free(reg1);
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

// static
void View::displaceWidgets(Widget* widget, int x, int y)
{
  JLink link;

  jrect_displace(widget->rc, x, y);

  JI_LIST_FOR_EACH(widget->children, link)
    displaceWidgets(reinterpret_cast<JWidget>(link->data), x, y);
}
