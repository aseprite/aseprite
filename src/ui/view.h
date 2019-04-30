// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_VIEW_H_INCLUDED
#define UI_VIEW_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/scroll_bar.h"
#include "ui/viewport.h"
#include "ui/widget.h"

namespace ui {
  class ScrollRegionEvent;

  class ViewableWidget {
  public:
    virtual ~ViewableWidget() { }
    virtual void onScrollRegion(ScrollRegionEvent& ev) = 0;
  };

  class View : public Widget
             , public ScrollableViewDelegate {
  public:
    View();

    bool hasScrollBars();
    ScrollBar* horizontalBar() { return &m_scrollbar_h; }
    ScrollBar* verticalBar() { return &m_scrollbar_v; }

    void attachToView(Widget* viewableWidget);
    Widget* attachedWidget();

    void hideScrollBars();
    void showScrollBars();
    void makeVisibleAllScrollableArea();

    // Returns the maximum viewable size requested by the attached
    // widget in the viewport.
    gfx::Size getScrollableSize() const;
    void setScrollableSize(const gfx::Size& sz,
                           const bool setScrollPos = true);

    // Returns the visible/available size to see the attached widget.
    gfx::Size visibleSize() const override;
    gfx::Point viewScroll() const override;
    void setViewScroll(const gfx::Point& pt) override;

    void updateView(const bool restoreScrollPos = true);

    Viewport* viewport();
    gfx::Rect viewportBounds();

    // For viewable widgets
    static View* getView(const Widget* viewableWidget);

  protected:
    // Events
    bool onProcessMessage(Message* msg) override;
    void onInitTheme(InitThemeEvent& ev) override;
    void onResize(ResizeEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;

    virtual void onSetViewScroll(const gfx::Point& pt);
    virtual void onScrollRegion(ScrollRegionEvent& ev);
    virtual void onScrollChange();

  private:
    void updateAttachedWidgetBounds(const gfx::Point& scrollPos);
    gfx::Point limitScrollPosToViewport(const gfx::Point& pt) const;

    bool m_hasBars;
    Viewport m_viewport;
    ScrollBar m_scrollbar_h;
    ScrollBar m_scrollbar_v;
  };

} // namespace ui

#endif
