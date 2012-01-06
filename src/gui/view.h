// ASE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_VIEW_H_INCLUDED
#define GUI_VIEW_H_INCLUDED

#include "base/compiler_specific.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "gui/scroll_bar.h"
#include "gui/viewport.h"
#include "gui/widget.h"

class View : public Widget
{
public:
  View();

  bool hasScrollBars();

  void attachToView(Widget* viewableWidget);
  void hideScrollBars();
  void makeVisibleAllScrollableArea();

  // Returns the maximum viewable size requested by the attached
  // widget in the viewport.
  gfx::Size getScrollableSize();
  void setScrollableSize(const gfx::Size& sz);

  // Returns the visible/available size to see the attached widget.
  gfx::Size getVisibleSize();

  gfx::Point getViewScroll();
  void setViewScroll(const gfx::Point& pt);

  void updateView();

  Viewport* getViewport();
  gfx::Rect getViewportBounds();

  // For viewable widgets
  static View* getView(Widget* viewableWidget);

protected:
  // Events
  bool onProcessMessage(Message* msg) OVERRIDE;

private:
  static void displaceWidgets(Widget* widget, int x, int y);

  bool m_hasBars;
  Viewport m_viewport;
  ScrollBar m_scrollbar_h;
  ScrollBar m_scrollbar_v;
};

#endif
