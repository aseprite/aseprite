// LAF OS Library
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_VK_H_INCLUDED
#define OS_WIN_VK_H_INCLUDED
#pragma once

#include <windows.h>

#include "os/keys.h"

namespace os {

  KeyScancode win32vk_to_scancode(int vk);
  KeyModifiers get_modifiers_from_last_win32_message();

  class VkToUnicode {
  public:
    VkToUnicode() : m_size(0) {
      m_ok = (GetKeyboardState(&m_keystate[0]) ? true: false);
    }

    void toUnicode(int vk, int scancode) {
      // ToUnicode returns several characters inside the buffer in
      // case that a dead-key wasn't combined with the next pressed
      // character.
      m_size =
        ToUnicode(vk, scancode, m_keystate, m_buffer,
                  sizeof(m_buffer)/sizeof(m_buffer[0]), 0);

      // If there is a control key pressed, we'll try to get the
      // unicode character turning control/shift off.
      if (m_size == 0 ||
          (m_size == 1 && m_buffer[0] == 0 &&
           m_keystate[VK_CONTROL] | m_keystate[VK_LCONTROL] | m_keystate[VK_RCONTROL])) {
        m_keystate[VK_SHIFT] = m_keystate[VK_LSHIFT] = m_keystate[VK_RSHIFT] = 0;
        m_keystate[VK_CONTROL] = m_keystate[VK_LCONTROL] = m_keystate[VK_RCONTROL] = 0;
        m_size =
          ToUnicode(vk, scancode, m_keystate, m_buffer,
                    sizeof(m_buffer)/sizeof(m_buffer[0]), 0);
      }
    }

    operator bool() { return m_ok; }
    int size() const { return ABS(m_size); }
    LPCWSTR begin() const { return m_buffer; }
    LPCWSTR end() const { return m_buffer+size(); }

    int operator[](const int i) {
      ASSERT(i >= 0 && i < size());
      return m_buffer[i];
    }

    // ToUnicode() returns -1 if there is dead key waiting
    bool isDeadKey() const {
      return (m_size == -1);
    }

  private:
    bool m_ok;
    BYTE m_keystate[256];
    WCHAR m_buffer[8];
    int m_size;
  };

} // namespace os

#endif
