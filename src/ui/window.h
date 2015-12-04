// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_WINDOW_H_INCLUDED
#define UI_WINDOW_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "gfx/point.h"
#include "ui/close_event.h"
#include "ui/event.h"
#include "ui/hit_test_event.h"
#include "ui/widget.h"

namespace ui {

  class Window : public Widget {
  public:
    enum Type { DesktopWindow, WithTitleBar, WithoutTitleBar };

    explicit Window(Type type, const std::string& text = "");
    ~Window();

    Widget* closer() const { return m_closer; }

    void setAutoRemap(bool state);
    void setMoveable(bool state);
    void setSizeable(bool state);
    void setOnTop(bool state);
    void setWantFocus(bool state);

    void remapWindow();
    void centerWindow();
    void positionWindow(int x, int y);
    void moveWindow(const gfx::Rect& rect);

    void openWindow();
    void openWindowInForeground();
    void closeWindow(Widget* closer);

    bool isTopLevel();
    bool isForeground() const { return m_isForeground; }
    bool isDesktop() const { return m_isDesktop; }
    bool isOnTop() const { return m_isOnTop; }
    bool isWantFocus() const { return m_isWantFocus; }
    bool isMoveable() const { return m_isMoveable; }

    HitTest hitTest(const gfx::Point& point);

    void removeDecorativeWidgets();

    // Signals
    base::Signal1<void, CloseEvent&> Close;

  protected:
    virtual bool onProcessMessage(Message* msg) override;
    virtual void onResize(ResizeEvent& ev) override;
    virtual void onSizeHint(SizeHintEvent& ev) override;
    virtual void onPaint(PaintEvent& ev) override;
    virtual void onBroadcastMouseMessage(WidgetsList& targets) override;
    virtual void onSetText() override;

    // New events
    virtual void onClose(CloseEvent& ev);
    virtual void onHitTest(HitTestEvent& ev);
    virtual void onWindowResize();

  private:
    void windowSetPosition(const gfx::Rect& rect);
    int getAction(int x, int y);
    void limitSize(int* w, int* h);
    void moveWindow(const gfx::Rect& rect, bool use_blit);

    Widget* m_closer;
    bool m_isDesktop : 1;
    bool m_isMoveable : 1;
    bool m_isSizeable : 1;
    bool m_isOnTop : 1;
    bool m_isWantFocus : 1;
    bool m_isForeground : 1;
    bool m_isAutoRemap : 1;
    int m_hitTest;
  };

} // namespace ui

#endif
