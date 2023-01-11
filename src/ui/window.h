// Aseprite UI Library
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_WINDOW_H_INCLUDED
#define UI_WINDOW_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "obs/signal.h"
#include "ui/close_event.h"
#include "ui/event.h"
#include "ui/hit_test_event.h"
#include "ui/widget.h"

namespace ui {

  class ButtonBase;
  class Label;

  class Window : public Widget {
  public:
    enum Type { DesktopWindow, WithTitleBar, WithoutTitleBar };

    explicit Window(Type type, const std::string& text = "");
    ~Window();

    // Preset parent display (instead of using the foreground/main
    // window). Useful to display an alert/subdialog inside a specific
    // window.
    void setParentDisplay(Display* display) { m_parentDisplay = display; }
    Display* parentDisplay() const { return m_parentDisplay; }

    bool ownDisplay() const { return m_ownDisplay; }
    Display* display() const;
    void setDisplay(Display* display, const bool own);
    bool hasDisplaySet() const { return m_display != nullptr; }

    Widget* closer() const { return m_closer; }

    void setAutoRemap(bool state);
    void setMoveable(bool state);
    void setSizeable(bool state);
    void setOnTop(bool state);
    void setWantFocus(bool state);

    void remapWindow();
    void centerWindow(Display* parentDisplay = nullptr);
    void moveWindow(const gfx::Rect& rect);

    // Expands or shrink the window to the given side (generally sizeHint())
    void expandWindow(const gfx::Size& size);

    void openWindow();
    void openWindowInForeground();
    void closeWindow(Widget* closer);

    bool isTopLevel();
    bool isForeground() const { return m_isForeground; }
    bool isDesktop() const { return m_isDesktop; }
    bool isOnTop() const { return m_isOnTop; }
    bool isWantFocus() const { return m_isWantFocus; }
    bool isSizeable() const { return m_isSizeable; }
    bool isMoveable() const { return m_isMoveable; }

    bool shouldCreateNativeWindow() const {
      return !isDesktop();
    }

    HitTest hitTest(const gfx::Point& point);

    // Last native window frame bounds. Saved just before we close the
    // native window so we can save this information in the
    // configuration file.
    gfx::Rect lastNativeFrame() const { return m_lastFrame; }
    void loadNativeFrame(const gfx::Rect& frame);

    // Esc key closes the current window (presses the little close
    // button decorator) only on foreground windows, but you can
    // override this to allow this behavior in other kind of windows.
    virtual bool shouldProcessEscKeyToCloseWindow() const {
      return isForeground();
    }

    // Signals
    obs::signal<void (Event&)> Open;
    obs::signal<void (CloseEvent&)> Close;

  protected:
    ButtonBase* closeButton() { return m_closeButton; }

    virtual bool onProcessMessage(Message* msg) override;
    virtual void onInvalidateRegion(const gfx::Region& region) override;
    virtual void onResize(ResizeEvent& ev) override;
    virtual void onSizeHint(SizeHintEvent& ev) override;
    virtual void onBroadcastMouseMessage(const gfx::Point& screenPos,
                                         WidgetsList& targets) override;
    virtual void onSetText() override;

    // New events
    virtual void onOpen(Event& ev);
    virtual void onBeforeClose(CloseEvent& ev) {}
    virtual void onClose(CloseEvent& ev);
    virtual void onHitTest(HitTestEvent& ev);
    virtual void onWindowResize();
    virtual void onWindowMovement();
    virtual void onBuildTitleLabel();

  private:
    void windowSetPosition(const gfx::Rect& rect);
    int getAction(int x, int y);
    void limitSize(int* w, int* h);
    void moveWindow(const gfx::Rect& rect, bool use_blit);

    Display* m_parentDisplay = nullptr;
    Display* m_display;
    Widget* m_closer;
    Label* m_titleLabel;
    ButtonBase* m_closeButton;
    bool m_ownDisplay : 1;
    bool m_isDesktop : 1;
    bool m_isMoveable : 1;
    bool m_isSizeable : 1;
    bool m_isOnTop : 1;
    bool m_isWantFocus : 1;
    bool m_isForeground : 1;
    bool m_isAutoRemap : 1;
    int m_hitTest;
    gfx::Rect m_lastFrame;
  };

} // namespace ui

#endif
