// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_WINDOW_H_INCLUDED
#define SHE_WIN_WINDOW_H_INCLUDED
#pragma once

#include "gfx/size.h"
#include "she/native_cursor.h"
#include "she/pointer_type.h"
#include "she/win/pen.h"

#include <string>
#include <windows.h>

namespace she {
  class Event;
  class Surface;
  class WindowSystem;

  class WinWindow {
  public:
    WinWindow(int width, int height, int scale);
    ~WinWindow();

    void queueEvent(Event& ev);
    int scale() const { return m_scale; }
    void setScale(int scale);
    void setVisible(bool visible);
    void maximize();
    bool isMaximized() const;
    bool isMinimized() const;
    gfx::Size clientSize() const;
    gfx::Size restoredSize() const;
    void setTitle(const std::string& title);
    void captureMouse();
    void releaseMouse();
    void setMousePosition(const gfx::Point& position);
    bool setNativeMouseCursor(NativeCursor cursor);
    bool setNativeMouseCursor(const she::Surface* surface,
                              const gfx::Point& focus,
                              const int scale);
    void updateWindow(const gfx::Rect& bounds);
    std::string getLayout();

    void setLayout(const std::string& layout);
    void setTranslateDeadKeys(bool state);

    HWND handle() { return m_hwnd; }

  private:
    bool setCursor(HCURSOR hcursor, bool custom);
    LRESULT wndProc(UINT msg, WPARAM wparam, LPARAM lparam);
    void mouseEvent(LPARAM lparam, Event& ev);
    bool pointerEvent(WPARAM wparam, LPARAM lparam,
                      Event& ev, POINTER_INFO& pi);

    virtual void onQueueEvent(Event& ev) { }
    virtual void onResize(const gfx::Size& sz) { }
    virtual void onPaint(HDC hdc) { }

    static void registerClass();
    static HWND createHwnd(WinWindow* self, int width, int height);
    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    static WindowSystem* system();

    mutable HWND m_hwnd;
    HCURSOR m_hcursor;
    gfx::Size m_clientSize;
    gfx::Size m_restoredSize;
    int m_scale;
    bool m_isCreated;
    bool m_translateDeadKeys;
    bool m_hasMouse;
    bool m_captureMouse;
    bool m_customHcursor;

    // Windows 8 pointer API
    bool m_usePointerApi;
    bool m_ignoreMouseMessages;
    UINT32 m_lastPointerId;
    UINT32 m_capturePointerId;

    // Wintab API data
    HCTX m_hpenctx;
    PointerType m_pointerType;
    double m_pressure;
  };

} // namespace she

#endif
