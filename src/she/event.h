// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_EVENT_H_INCLUDED
#define SHE_EVENT_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/size.h"
#include "she/keys.h"

#include <string>
#include <vector>

namespace she {

  class Display;

  class Event {
  public:
    enum Type {
      None,
      CloseDisplay,
      ResizeDisplay,
      DropFiles,
      MouseEnter,
      MouseLeave,
      MouseMove,
      MouseDown,
      MouseUp,
      MouseWheel,
      MouseDoubleClick,
      KeyDown,
      KeyUp,
      TouchMagnify,
    };

    enum MouseButton {
      NoneButton,
      LeftButton,
      RightButton,
      MiddleButton
    };

    typedef std::vector<std::string> Files;

    Event() : m_type(None),
              m_display(nullptr),
              m_scancode(kKeyNil),
              m_modifiers(kKeyUninitializedModifier),
              m_unicodeChar(0),
              m_repeat(0),
              m_preciseWheel(false),
              m_button(NoneButton) {
    }

    Type type() const { return m_type; }
    Display* display() const { return m_display; }
    const Files& files() const { return m_files; }
    KeyScancode scancode() const { return m_scancode; }
    KeyModifiers modifiers() const { return m_modifiers; }
    int unicodeChar() const { return m_unicodeChar; }
    int repeat() const { return m_repeat; }
    gfx::Point position() const { return m_position; }
    gfx::Point wheelDelta() const { return m_wheelDelta; }
    bool preciseWheel() const { return m_preciseWheel; }
    MouseButton button() const { return m_button; }
    double magnification() const { return m_magnification; }

    void setType(Type type) { m_type = type; }
    void setDisplay(Display* display) { m_display = display; }
    void setFiles(const Files& files) { m_files = files; }

    void setScancode(KeyScancode scancode) { m_scancode = scancode; }
    void setModifiers(KeyModifiers modifiers) { m_modifiers = modifiers; }
    void setUnicodeChar(int unicodeChar) { m_unicodeChar = unicodeChar; }
    void setRepeat(int repeat) { m_repeat = repeat; }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setWheelDelta(const gfx::Point& delta) { m_wheelDelta = delta; }
    void setPreciseWheel(bool precise) { m_preciseWheel = precise; }
    void setButton(MouseButton button) { m_button = button; }
    void setMagnification(double magnification) { m_magnification = magnification; }

  private:
    Type m_type;
    Display* m_display;
    Files m_files;
    KeyScancode m_scancode;
    KeyModifiers m_modifiers;
    int m_unicodeChar;
    int m_repeat; // repeat=0 means the first time the key is pressed
    gfx::Point m_position;
    gfx::Point m_wheelDelta;
    bool m_preciseWheel;
    MouseButton m_button;
    double m_magnification;
  };

} // namespace she

#endif
