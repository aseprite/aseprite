// SHE library
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_WINDOW_H_INCLUDED
#define SHE_WIN_WINDOW_H_INCLUDED
#pragma once

#include "base/time.h"
#include "gfx/size.h"
#include "she/event.h"
#include "she/native_cursor.h"
#include "she/pointer_type.h"
#include "she/win/pen.h"

#include <string>
#include <windows.h>
#include <interactioncontext.h>

#define SHE_USE_POINTER_API_FOR_MOUSE 0

namespace she {
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
    bool pointerEvent(WPARAM wparam, Event& ev, POINTER_INFO& pi);
    void handlePointerButtonChange(Event& ev, POINTER_INFO& pi);
    void handleInteractionContextOutput(
      const INTERACTION_CONTEXT_OUTPUT* output);

    virtual void onQueueEvent(Event& ev) { }
    virtual void onResize(const gfx::Size& sz) { }
    virtual void onPaint(HDC hdc) { }

    static void registerClass();
    static HWND createHwnd(WinWindow* self, int width, int height);
    static LRESULT CALLBACK staticWndProc(
      HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static void CALLBACK staticInteractionContextCallback(
      void* clientData,
      const INTERACTION_CONTEXT_OUTPUT* output);

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
    UINT32 m_lastPointerId;
    UINT32 m_capturePointerId;
    HINTERACTIONCONTEXT m_ictx;

#if SHE_USE_POINTER_API_FOR_MOUSE
    // Emulate double-click with pointer API. I guess that this should
    // be done by the Interaction Context API but it looks like
    // messages with pointerType != PT_TOUCH or PT_PEN are just
    // ignored by the ProcessPointerFramesInteractionContext()
    // function even when we call AddPointerInteractionContext() with
    // the given PT_MOUSE pointer.
    bool m_emulateDoubleClick;
    base::tick_t m_doubleClickMsecs;
    base::tick_t m_lastPointerDownTime;
    Event::MouseButton m_lastPointerDownButton;
    int m_pointerDownCount;
#endif

    // Wintab API data
    HCTX m_hpenctx;
    PointerType m_pointerType;
    double m_pressure;
  };

} // namespace she

#endif
