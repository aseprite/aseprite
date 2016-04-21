// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MESSAGE_H_INCLUDED
#define UI_MESSAGE_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/keys.h"
#include "ui/message_type.h"
#include "ui/mouse_buttons.h"
#include "ui/pointer_type.h"
#include "ui/widgets_list.h"

#include <string>
#include <vector>

namespace ui {

  class Timer;
  class Widget;

  class Message {
  public:
    typedef WidgetsList::iterator& recipients_iterator;

    Message(MessageType type,
            KeyModifiers modifiers = kKeyUninitializedModifier);
    virtual ~Message();

    MessageType type() const { return m_type; }
    const WidgetsList& recipients() const { return m_recipients; }
    bool hasRecipients() const { return !m_recipients.empty(); }
    bool isUsed() const { return m_used; }
    void markAsUsed() { m_used = true; }
    KeyModifiers modifiers() const { return m_modifiers; }
    bool shiftPressed() const { return (m_modifiers & kKeyShiftModifier) == kKeyShiftModifier; }
    bool ctrlPressed() const { return (m_modifiers & kKeyCtrlModifier) == kKeyCtrlModifier; }
    bool altPressed() const { return (m_modifiers & kKeyAltModifier) == kKeyAltModifier; }
    bool cmdPressed() const { return (m_modifiers & kKeyCmdModifier) == kKeyCmdModifier; }
    bool winPressed() const { return (m_modifiers & kKeyWinModifier) == kKeyWinModifier; }
    bool onlyShiftPressed() const { return m_modifiers == kKeyShiftModifier; }
    bool onlyCtrlPressed() const { return m_modifiers == kKeyCtrlModifier; }
    bool onlyAltPressed() const { return m_modifiers == kKeyAltModifier; }
    bool onlyCmdPressed() const { return m_modifiers == kKeyCmdModifier; }
    bool onlyWinPressed() const { return m_modifiers == kKeyWinModifier; }

    void addRecipient(Widget* widget);
    void prependRecipient(Widget* widget);
    void removeRecipient(Widget* widget);

    void broadcastToChildren(Widget* widget);

  private:
    MessageType m_type;         // Type of message
    WidgetsList m_recipients; // List of recipients of the message
    bool m_used;              // Was used
    KeyModifiers m_modifiers; // Key modifiers pressed when message was created
  };

  class KeyMessage : public Message {
  public:
    KeyMessage(MessageType type,
               KeyScancode scancode,
               KeyModifiers modifiers,
               int unicodeChar,
               int repeat);

    KeyScancode scancode() const { return m_scancode; }
    int unicodeChar() const { return m_unicodeChar; }
    int repeat() const { return m_repeat; }
    bool propagateToChildren() const { return m_propagate_to_children; }
    bool propagateToParent() const { return m_propagate_to_parent; }
    void setPropagateToChildren(bool flag) { m_propagate_to_children = flag; }
    void setPropagateToParent(bool flag) { m_propagate_to_parent = flag; }

  private:
    KeyScancode m_scancode;
    int m_unicodeChar;
    int m_repeat; // repeat=0 means the first time the key is pressed
    bool m_propagate_to_children : 1;
    bool m_propagate_to_parent : 1;
  };

  class PaintMessage : public Message {
  public:
    PaintMessage(int count, const gfx::Rect& rect)
      : Message(kPaintMessage), m_count(count), m_rect(rect) {
    }

    int count() const { return m_count; }
    const gfx::Rect& rect() const { return m_rect; }

  private:
    int m_count;             // Cound=0 if it's last msg of draw-chain
    gfx::Rect m_rect;        // Area to draw
  };

  class MouseMessage : public Message {
  public:
    MouseMessage(MessageType type,
                 PointerType pointerType,
                 MouseButtons buttons,
                 KeyModifiers modifiers,
                 const gfx::Point& pos,
                 const gfx::Point& wheelDelta = gfx::Point(0, 0),
                 bool preciseWheel = false)
      : Message(type, modifiers),
        m_pointerType(pointerType),
        m_buttons(buttons),
        m_pos(pos),
        m_wheelDelta(wheelDelta),
        m_preciseWheel(preciseWheel) {
    }

    PointerType pointerType() const { return m_pointerType; }
    MouseButtons buttons() const { return m_buttons; }
    bool left() const { return (m_buttons & kButtonLeft) == kButtonLeft; }
    bool right() const { return (m_buttons & kButtonRight) == kButtonRight; }
    bool middle() const { return (m_buttons & kButtonMiddle) == kButtonMiddle; }
    gfx::Point wheelDelta() const { return m_wheelDelta; }
    bool preciseWheel() const { return m_preciseWheel; }

    const gfx::Point& position() const { return m_pos; }

  private:
    PointerType m_pointerType;
    MouseButtons m_buttons;     // Pressed buttons
    gfx::Point m_pos;           // Mouse position
    gfx::Point m_wheelDelta;    // Wheel axis variation
    bool m_preciseWheel;
  };

  class TouchMessage : public Message {
  public:
    TouchMessage(MessageType type,
                 KeyModifiers modifiers,
                 const gfx::Point& pos,
                 double magnification)
      : Message(type, modifiers),
        m_pos(pos),
        m_magnification(magnification) {
    }

    const gfx::Point& position() const { return m_pos; }
    double magnification() const { return m_magnification; }

  private:
    gfx::Point m_pos;           // Mouse position
    double m_magnification;
  };

  class TimerMessage : public Message {
  public:
    TimerMessage(int count, Timer* timer)
      : Message(kTimerMessage), m_count(count), m_timer(timer) {
    }

    int count() const { return m_count; }
    Timer* timer() { return m_timer; }

  private:
    int m_count;                    // Accumulated calls
    Timer* m_timer;                 // Timer handle
  };

  class DropFilesMessage : public Message {
  public:
    typedef std::vector<std::string> Files;

    DropFilesMessage(const Files& files)
      : Message(kDropFilesMessage), m_files(files) {
    }

    const Files& files() const { return m_files; }

  private:
    Files m_files;
  };

} // namespace ui

#endif
