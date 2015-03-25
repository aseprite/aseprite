// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_WINDOW_H_INCLUDED
#define SHE_WIN_WINDOW_H_INCLUDED
#pragma once

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "she/keys.h"

#ifndef WM_MOUSEHWHEEL
  #define WM_MOUSEHWHEEL 0x020E
#endif

namespace she {

static KeyScancode vkToScancode(int vk) {
  static KeyScancode keymap[0xFF] = {
    // 0x00
    kKeyNil,
    kKeyNil, // 0x01 - VK_LBUTTON
    kKeyNil, // 0x02 - VK_RBUTTON
    kKeyNil, // 0x03 - VK_CANCEL
    kKeyNil, // 0x04 - VK_MBUTTON
    kKeyNil, // 0x05 - VK_XBUTTON1
    kKeyNil, // 0x06 - VK_XBUTTON2
    kKeyNil, // 0x07 - Unassigned
    kKeyBackspace, // 0x08 - VK_BACK
    kKeyTab, // 0x09 - VK_TAB
    kKeyNil, // 0x0A - Reserved
    kKeyNil, // 0x0B - Reserved
    kKeyNil, // 0x0C - VK_CLEAR
    kKeyEnter, // 0x0D - VK_RETURN
    kKeyNil, // 0x0E - Undefined
    kKeyNil, // 0x0F - Undefined
    // 0x10
    kKeyLShift, // 0x10 - VK_SHIFT
    kKeyLControl, // 0x11 - VK_CONTROL
    kKeyMenu, // 0x12 - VK_MENU
    kKeyPause, // 0x13 - VK_PAUSE
    kKeyCapsLock, // 0x14 - VK_CAPITAL
    kKeyNil, // 0x15 - VK_KANA
    kKeyNil, // 0x17 - VK_JUNJA
    kKeyNil, // 0x18 - VK_FINAL
    kKeyKanji, // 0x19 - VK_KANJI
    kKeyNil, // 0x1A - Unknown
    kKeyEsc, // 0x1B - VK_ESCAPE
    kKeyConvert, // 0x1C - VK_CONVERT
    kKeyNoconvert, // 0x1D - VK_NONCONVERT
    kKeyNil, // 0x1E - VK_ACCEPT
    kKeyNil, // 0x1F - VK_MODECHANGE
    // 0x20
    kKeySpace, // 0x20 - VK_SPACE
    kKeyPageUp, // 0x21 - VK_PRIOR
    kKeyPageDown, // 0x22 - VK_NEXT
    kKeyEnd, // 0x23 - VK_END
    kKeyHome, // 0x24 - VK_HOME
    kKeyLeft, // 0x25 - VK_LEFT
    kKeyUp, // 0x26 - VK_UP
    kKeyRight, // 0x27 - VK_RIGHT
    kKeyDown, // 0x28 - VK_DOWN
    kKeyNil, // 0x29 - VK_SELECT
    kKeyPrtscr, // 0x2A - VK_PRINT
    kKeyNil, // 0x2B - VK_EXECUTE
    kKeyNil, // 0x2C - VK_SNAPSHOT
    kKeyInsert, // 0x2D - VK_INSERT
    kKeyDel, // 0x2E - VK_DELETE
    kKeyNil, // 0x2F - VK_HELP
    // 0x30
    kKey0, // 0x30 - VK_0
    kKey1, // 0x31 - VK_1
    kKey2, // 0x32 - VK_2
    kKey3, // 0x33 - VK_3
    kKey4, // 0x34 - VK_4
    kKey5, // 0x35 - VK_5
    kKey6, // 0x36 - VK_6
    kKey7, // 0x37 - VK_7
    kKey8, // 0x38 - VK_8
    kKey9, // 0x39 - VK_9
    // 0x40
    kKeyNil, // 0x40 - Unassigned
    kKeyA, // 0x41 - VK_A
    kKeyB, // 0x42 - VK_B
    kKeyC, // 0x43 - VK_C
    kKeyD, // 0x44 - VK_D
    kKeyE, // 0x45 - VK_E
    kKeyF, // 0x46 - VK_F
    kKeyG, // 0x47 - VK_G
    kKeyH, // 0x48 - VK_H
    kKeyI, // 0x49 - VK_I
    kKeyJ, // 0x4A - VK_J
    kKeyK, // 0x4B - VK_K
    kKeyL, // 0x4C - VK_L
    kKeyM, // 0x4D - VK_M
    kKeyN, // 0x4E - VK_N
    kKeyO, // 0x4F - VK_O
    // 0x50
    kKeyP, // 0x50 - VK_P
    kKeyQ, // 0x51 - VK_Q
    kKeyR, // 0x52 - VK_R
    kKeyS, // 0x53 - VK_S
    kKeyT, // 0x54 - VK_T
    kKeyU, // 0x55 - VK_U
    kKeyV, // 0x56 - VK_V
    kKeyW, // 0x57 - VK_W
    kKeyX, // 0x58 - VK_X
    kKeyY, // 0x59 - VK_Y
    kKeyZ, // 0x5A - VK_Z
    kKeyLWin, // 0x5B - VK_LWIN
    kKeyRWin, // 0x5C - VK_RWIN
    kKeyNil, // 0x5D - VK_APPS
    kKeyNil, // 0x5E - Reserved
    kKeyNil, // 0x5F - VK_SLEEP
    // 0x60
    kKey0Pad, // 0x60 - VK_NUMPAD0
    kKey1Pad, // 0x61 - VK_NUMPAD1
    kKey2Pad, // 0x62 - VK_NUMPAD2
    kKey3Pad, // 0x63 - VK_NUMPAD3
    kKey4Pad, // 0x64 - VK_NUMPAD4
    kKey5Pad, // 0x65 - VK_NUMPAD5
    kKey6Pad, // 0x66 - VK_NUMPAD6
    kKey7Pad, // 0x67 - VK_NUMPAD7
    kKey8Pad, // 0x68 - VK_NUMPAD8
    kKey9Pad, // 0x69 - VK_NUMPAD9
    kKeyAsterisk, // 0x6A - VK_MULTIPLY
    kKeyPlusPad, // 0x6B - VK_ADD
    kKeyNil, // 0x6C - VK_SEPARATOR
    kKeyMinusPad, // 0x6D - VK_SUBTRACT
    kKeyNil, // 0x6E - VK_DECIMAL
    kKeyNil, // 0x6F - VK_DIVIDE
    // 0x70
    kKeyNil, // 0x70 - VK_F1
    kKeyNil, // 0x71 - VK_F2
    kKeyNil, // 0x72 - VK_F3
    kKeyNil, // 0x73 - VK_F4
    kKeyNil, // 0x74 - VK_F5
    kKeyNil, // 0x75 - VK_F6
    kKeyNil, // 0x76 - VK_F7
    kKeyNil, // 0x77 - VK_F8
    kKeyNil, // 0x78 - VK_F9
    kKeyNil, // 0x79 - VK_F10
    kKeyNil, // 0x7A - VK_F11
    kKeyNil, // 0x7B - VK_F12
    kKeyNil, // 0x7C - VK_F13
    kKeyNil, // 0x7D - VK_F14
    kKeyNil, // 0x7E - VK_F15
    kKeyNil, // 0x7F - VK_F16
    // 0x80
    kKeyNil, // 0x80 - VK_F17
    kKeyNil, // 0x81 - VK_F18
    kKeyNil, // 0x82 - VK_F19
    kKeyNil, // 0x83 - VK_F20
    kKeyNil, // 0x84 - VK_F21
    kKeyNil, // 0x85 - VK_F22
    kKeyNil, // 0x86 - VK_F23
    kKeyNil, // 0x87 - VK_F24
    kKeyNil, // 0x88 - Unassigned
    kKeyNil, // 0x89 - Unassigned
    kKeyNil, // 0x8A - Unassigned
    kKeyNil, // 0x8B - Unassigned
    kKeyNil, // 0x8C - Unassigned
    kKeyNil, // 0x8D - Unassigned
    kKeyNil, // 0x8E - Unassigned
    kKeyNil, // 0x8F - Unassigned
    // 0x90
    kKeyNil, // 0x90 - VK_NUMLOCK
    kKeyNil, // 0x91 - VK_SCROLL
    kKeyEqualsPad, // 0x92 - VK_OEM_NEC_EQUAL / VK_OEM_FJ_JISHO
    kKeyNil, // 0x93- VK_OEM_FJ_MASSHOU
    kKeyNil, // 0x94- VK_OEM_FJ_TOUROKU
    kKeyNil, // 0x95 - VK_OEM_FJ_LOYA
    kKeyNil, // 0x96 - VK_OEM_FJ_ROYA
    kKeyNil, // 0x97 - Unassigned
    kKeyNil, // 0x98 - Unassigned
    kKeyNil, // 0x99 - Unassigned
    kKeyNil, // 0x9A - Unassigned
    kKeyNil, // 0x9B - Unassigned
    kKeyNil, // 0x9C - Unassigned
    kKeyNil, // 0x9D - Unassigned
    kKeyNil, // 0x9E - Unassigned
    kKeyNil, // 0x9F - Unassigned
    // 0xA0
    kKeyNil, // 0xA0 - VK_LSHIFT
    kKeyNil, // 0xA1 - VK_RSHIFT
    kKeyNil, // 0xA2 - VK_LCONTROL
    kKeyNil, // 0xA3 - VK_RCONTROL
    kKeyNil, // 0xA4 - VK_LMENU
    kKeyNil, // 0xA5 - VK_RMENU
    kKeyNil, // 0xA6 - VK_BROWSER_BACK
    kKeyNil, // 0xA7 - VK_BROWSER_FORWARD
    kKeyNil, // 0xA8 - VK_BROWSER_REFRESH
    kKeyNil, // 0xA9 - VK_BROWSER_STOP
    kKeyNil, // 0xAA - VK_BROWSER_SEARCH
    kKeyNil, // 0xAB - VK_BROWSER_FAVORITES
    kKeyNil, // 0xAC - VK_BROWSER_HOME
    kKeyNil, // 0xAD - VK_VOLUME_MUTE
    kKeyNil, // 0xAE - VK_VOLUME_DOWN
    kKeyNil, // 0xAF - VK_VOLUME_UP
    // 0xB0
    kKeyNil, // 0xB0 - VK_MEDIA_NEXT_TRACK
    kKeyNil, // 0xB1 - VK_MEDIA_PREV_TRACK
    kKeyNil, // 0xB2 - VK_MEDIA_STOP
    kKeyNil, // 0xB3 - VK_MEDIA_PLAY_PAUSE
    kKeyNil, // 0xB4 - VK_LAUNCH_MAIL
    kKeyNil, // 0xB5- VK_LAUNCH_MEDIA_SELECT
    kKeyNil, // 0xB6 - VK_LAUNCH_APP1
    kKeyNil, // 0xB7 - VK_LAUNCH_APP2
    kKeyNil, // 0xB8 - Reserved
    kKeyNil, // 0xB9 - Reserved
    kKeyNil, // 0xBA - VK_OEM_1
    kKeyNil, // 0xBB - VK_OEM_PLUS
    kKeyNil, // 0xBC - VK_OEM_COMMA
    kKeyNil, // 0xBD - VK_OEM_MINUS
    kKeyNil, // 0xBE - VK_OEM_PERIOD
    kKeyNil, // 0xBF - VK_OEM_2
    // 0xC0
    kKeyNil, // 0xC0 - VK_OEM_3
    kKeyNil, // 0xC1 - Reserved
    kKeyNil, // 0xC2 - Reserved
    kKeyNil, // 0xC3 - Reserved
    kKeyNil, // 0xC4 - Reserved
    kKeyNil, // 0xC5 - Reserved
    kKeyNil, // 0xC6 - Reserved
    kKeyNil, // 0xC7 - Reserved
    kKeyNil, // 0xC8 - Reserved
    kKeyNil, // 0xC9 - Reserved
    kKeyNil, // 0xCA - Reserved
    kKeyNil, // 0xCB - Reserved
    kKeyNil, // 0xCC - Reserved
    kKeyNil, // 0xCD - Reserved
    kKeyNil, // 0xCE - Reserved
    kKeyNil, // 0xCF - Reserved
    // 0xD0
    kKeyNil, // 0xD0 - Reserved
    kKeyNil, // 0xD1 - Reserved
    kKeyNil, // 0xD2 - Reserved
    kKeyNil, // 0xD3 - Reserved
    kKeyNil, // 0xD4 - Reserved
    kKeyNil, // 0xD5 - Reserved
    kKeyNil, // 0xD6 - Reserved
    kKeyNil, // 0xD7 - Reserved
    kKeyNil, // 0xD8 - Unassigned
    kKeyNil, // 0xD9 - Unassigned
    kKeyNil, // 0xDA - Unassigned
    kKeyNil, // 0xDB - VK_OEM_4
    kKeyNil, // 0xDC - VK_OEM_5
    kKeyNil, // 0xDD - VK_OEM_6
    kKeyNil, // 0xDE - VK_OEM_7
    kKeyNil, // 0xDF - VK_OEM_8
    // 0xE0
    kKeyNil, // 0xE0 - Reserved
    kKeyNil, // 0xE1 - VK_OEM_AX
    kKeyNil, // 0xE2 - VK_OEM_102
    kKeyNil, // 0xE3 - VK_ICO_HELP
    kKeyNil, // 0xE4 - VK_ICO_00
    kKeyNil, // 0xE5 - VK_PROCESSKEY
    kKeyNil, // 0xE6 - VK_ICO_CLEAR
    kKeyNil, // 0xE7 - VK_PACKET
    kKeyNil, // 0xE8 - Unassigned
    kKeyNil, // 0xE9 - VK_OEM_RESET
    kKeyNil, // 0xEA - VK_OEM_JUMP
    kKeyNil, // 0xEB - VK_OEM_PA1
    kKeyNil, // 0xEC - VK_OEM_PA2
    kKeyNil, // 0xED - VK_OEM_PA3
    kKeyNil, // 0xEE - VK_OEM_WSCTRL
    kKeyNil, // 0xEF - VK_OEM_CUSEL
    // 0xF0
    kKeyNil, // 0xF0 - VK_OEM_ATTN
    kKeyNil, // 0xF1 - VK_OEM_FINISH
    kKeyNil, // 0xF2 - VK_OEM_COPY
    kKeyNil, // 0xF3 - VK_OEM_AUTO
    kKeyNil, // 0xF4 - VK_OEM_ENLW
    kKeyNil, // 0xF5 - VK_OEM_BACKTAB
    kKeyNil, // 0xF6 - VK_ATTN
    kKeyNil, // 0xF7 - VK_CRSEL
    kKeyNil, // 0xF8 - VK_EXSEL
    kKeyNil, // 0xF9 - VK_EREOF
    kKeyNil, // 0xFA - VK_PLAY
    kKeyNil, // 0xFB - VK_ZOOM
    kKeyNil, // 0xFC - VK_NONAME
    kKeyNil, // 0xFD - VK_PA1
    kKeyNil, // 0xFE - VK_OEM_CLEAR
    kKeyNil, // 0xFF - Reserved
  };
  if (vk < 0 || vk > 255)
    vk = kKeyNil;
  return keymap[vk];
}

  #define SHE_WND_CLASS_NAME L"Aseprite.Window"

  template<typename T>
  class Window {
  public:
    Window() {
      registerClass();
      m_hwnd = createHwnd(this);
      m_hasMouse = false;
      m_captureMouse = false;
      m_scale = 1;
    }

    void queueEvent(Event& ev) {
      static_cast<T*>(this)->queueEventImpl(ev);
    }

    int scale() const {
      return m_scale;
    }

    void setScale(int scale) {
      m_scale = scale;
    }

    void setVisible(bool visible) {
      if (visible) {
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
        UpdateWindow(m_hwnd);
        DrawMenuBar(m_hwnd);
      }
      else
        ShowWindow(m_hwnd, SW_HIDE);
    }

    void maximize() {
      ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    bool isMaximized() const {
      return (IsZoomed(m_hwnd) ? true: false);
    }

    gfx::Size clientSize() const {
      return m_clientSize;
    }

    gfx::Size restoredSize() const {
      return m_restoredSize;
    }

    void setText(const std::string& text) {
      SetWindowText(m_hwnd, base::from_utf8(text).c_str());
    }

    void captureMouse() {
      m_captureMouse = true;
    }

    void releaseMouse() {
      m_captureMouse = false;
    }

    void invalidate() {
      InvalidateRect(m_hwnd, NULL, FALSE);
    }

    HWND handle() {
      return m_hwnd;
    }

  private:
    LRESULT wndProc(UINT msg, WPARAM wparam, LPARAM lparam) {
      switch (msg) {

        case WM_CLOSE: {
          Event ev;
          ev.setType(Event::CloseDisplay);
          queueEvent(ev);
          break;
        }

        case WM_PAINT: {
          PAINTSTRUCT ps;
          HDC hdc = BeginPaint(m_hwnd, &ps);
          static_cast<T*>(this)->paintImpl(hdc);
          EndPaint(m_hwnd, &ps);
          return true;
        }

        case WM_SIZE: {
          switch (wparam) {
            case SIZE_MAXIMIZED:
            case SIZE_RESTORED:
              m_clientSize.w = GET_X_LPARAM(lparam);
              m_clientSize.h = GET_Y_LPARAM(lparam);
              break;
          }

          WINDOWPLACEMENT pl;
          pl.length = sizeof(pl);
          if (GetWindowPlacement(m_hwnd, &pl)) {
            m_restoredSize = gfx::Size(
              pl.rcNormalPosition.right - pl.rcNormalPosition.left,
              pl.rcNormalPosition.bottom - pl.rcNormalPosition.top);
          }
          break;
        }

        case WM_MOUSEMOVE: {
          // Adjust capture
          if (m_captureMouse) {
            if (GetCapture() != m_hwnd)
              SetCapture(m_hwnd);
          }
          else {
            if (GetCapture() == m_hwnd)
              ReleaseCapture();
          }

          Event ev;
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));

          if (!m_hasMouse) {
            m_hasMouse = true;

            ev.setType(Event::MouseEnter);
            queueEvent(ev);

            // Track mouse to receive WM_MOUSELEAVE message.
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            _TrackMouseEvent(&tme);
          }

          ev.setType(Event::MouseMove);
          queueEvent(ev);
          break;
        }

        case WM_NCMOUSEMOVE:
        case WM_MOUSELEAVE:
          if (m_hasMouse) {
            m_hasMouse = false;

            Event ev;
            ev.setType(Event::MouseLeave);
            queueEvent(ev);
          }
          break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
          Event ev;
          ev.setType(Event::MouseDown);
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));
          ev.setButton(
            msg == WM_LBUTTONDOWN ? Event::LeftButton:
            msg == WM_RBUTTONDOWN ? Event::RightButton:
            msg == WM_MBUTTONDOWN ? Event::MiddleButton: Event::NoneButton);
          queueEvent(ev);
          break;
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
          Event ev;
          ev.setType(Event::MouseUp);
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));
          ev.setButton(
            msg == WM_LBUTTONUP ? Event::LeftButton:
            msg == WM_RBUTTONUP ? Event::RightButton:
            msg == WM_MBUTTONUP ? Event::MiddleButton: Event::NoneButton);
          queueEvent(ev);

          // Avoid popup menu for scrollbars
          if (msg == WM_RBUTTONUP)
            return 0;

          break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK: {
          Event ev;
          ev.setType(Event::MouseDoubleClick);
          ev.setPosition(gfx::Point(
              GET_X_LPARAM(lparam) / m_scale,
              GET_Y_LPARAM(lparam) / m_scale));
          ev.setButton(
            msg == WM_LBUTTONDBLCLK ? Event::LeftButton:
            msg == WM_RBUTTONDBLCLK ? Event::RightButton:
            msg == WM_MBUTTONDBLCLK ? Event::MiddleButton: Event::NoneButton);
          queueEvent(ev);
          break;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
          RECT rc;
          ::GetWindowRect(m_hwnd, &rc);

          Event ev;
          ev.setType(Event::MouseWheel);
          ev.setPosition((gfx::Point(
                GET_X_LPARAM(lparam),
                GET_Y_LPARAM(lparam)) - gfx::Point(rc.left, rc.top))
            / m_scale);

          int z = ((short)HIWORD(wparam)) / WHEEL_DELTA;
          gfx::Point delta(
            (msg == WM_MOUSEHWHEEL ? z: 0),
            (msg == WM_MOUSEWHEEL ? -z: 0));
          ev.setWheelDelta(delta);

          //PRINTF("WHEEL: %d %d\n", delta.x, delta.y);

          queueEvent(ev);
          break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL: {
          RECT rc;
          ::GetWindowRect(m_hwnd, &rc);

          POINT pos;
          ::GetCursorPos(&pos);

          Event ev;
          ev.setType(Event::MouseWheel);
          ev.setPosition((gfx::Point(pos.x, pos.y) - gfx::Point(rc.left, rc.top))
            / m_scale);

          int bar = (msg == WM_HSCROLL ? SB_HORZ: SB_VERT);
          int z = GetScrollPos(m_hwnd, bar);

          switch (LOWORD(wparam)) {
            case SB_LEFT:
            case SB_LINELEFT:
              --z;
              break;
            case SB_PAGELEFT:
              z -= 2;
              break;
            case SB_RIGHT:
            case SB_LINERIGHT:
              ++z;
              break;
            case SB_PAGERIGHT:
              z += 2;
              break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
            case SB_ENDSCROLL:
              // Do nothing
              break;
          }

          gfx::Point delta(
            (msg == WM_HSCROLL ? (z-50): 0),
            (msg == WM_VSCROLL ? (z-50): 0));
          ev.setWheelDelta(delta);

          //PRINTF("SCROLL: %d %d\n", delta.x, delta.y);

          SetScrollPos(m_hwnd, bar, 50, FALSE);

          queueEvent(ev);
          break;
        }

        case WM_KEYDOWN: {
          Event ev;
          ev.setType(Event::KeyDown);
          ev.setScancode(vkToScancode(wparam));
          ev.setUnicodeChar(0);  // TODO
          ev.setRepeat(lparam & 15);
          queueEvent(ev);
          break;
        }

        case WM_KEYUP: {
          Event ev;
          ev.setType(Event::KeyUp);
          ev.setScancode(vkToScancode(wparam));
          ev.setUnicodeChar(0);  // TODO
          ev.setRepeat(lparam & 15);
          queueEvent(ev);
          break;
        }

        case WM_UNICHAR: {
          Event ev;
          ev.setType(Event::KeyUp);
          ev.setScancode(kKeyNil);    // TODO
          ev.setUnicodeChar(wparam);
          ev.setRepeat(lparam & 15);
          queueEvent(ev);
          break;
        }

        case WM_DROPFILES: {
          HDROP hdrop = (HDROP)(wparam);
          Event::Files files;

          int count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
          for (int index=0; index<count; ++index) {
            int length = DragQueryFile(hdrop, index, NULL, 0);
            if (length > 0) {
              std::vector<TCHAR> str(length+1);
              DragQueryFile(hdrop, index, &str[0], str.size());
              files.push_back(base::to_utf8(&str[0]));
            }
          }

          DragFinish(hdrop);

          Event ev;
          ev.setType(Event::DropFiles);
          ev.setFiles(files);
          queueEvent(ev);
          break;
        }

      }

      return DefWindowProc(m_hwnd, msg, wparam, lparam);
    }

    static void registerClass() {
      HMODULE instance = GetModuleHandle(nullptr);

      WNDCLASSEX wcex;
      if (GetClassInfoEx(instance, SHE_WND_CLASS_NAME, &wcex))
        return;                 // Already registered

      wcex.cbSize        = sizeof(WNDCLASSEX);
      wcex.style         = 0;
      wcex.lpfnWndProc   = &Window::staticWndProc;
      wcex.cbClsExtra    = 0;
      wcex.cbWndExtra    = 0;
      wcex.hInstance     = instance;
      wcex.hIcon         = nullptr;
      wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
      wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
      wcex.lpszMenuName  = nullptr;
      wcex.lpszClassName = SHE_WND_CLASS_NAME;
      wcex.hIconSm       = nullptr;

      if (RegisterClassEx(&wcex) == 0)
        throw std::runtime_error("Error registering window class");
    }

    static HWND createHwnd(Window* self) {
      HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
        SHE_WND_CLASS_NAME,
        L"",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        LPVOID(self));
      if (!hwnd)
        return nullptr;

      SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(self));
      return hwnd;
    }

    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
      Window* wnd;

      if (msg == WM_CREATE)
        wnd = (Window*)lparam;
      else
        wnd = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

      if (wnd)
        return wnd->wndProc(msg, wparam, lparam);
      else
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    mutable HWND m_hwnd;
    gfx::Size m_clientSize;
    gfx::Size m_restoredSize;
    int m_scale;
    bool m_hasMouse;
    bool m_captureMouse;
  };

} // namespace she

#endif
