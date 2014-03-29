// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_MESSAGE_H_INCLUDED
#define UI_MESSAGE_H_INCLUDED
#pragma once

#include "base/compiler_specific.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/keys.h"
#include "ui/message_type.h"
#include "ui/mouse_buttons.h"
#include "ui/widgets_list.h"

#include <string>
#include <vector>

namespace ui {

  class Timer;
  class Widget;

  class Message
  {
  public:
    typedef WidgetsList::iterator& recipients_iterator;

    Message(MessageType type);
    virtual ~Message();

    MessageType type() const { return m_type; }
    const WidgetsList& recipients() const { return m_recipients; }
    bool hasRecipients() const { return !m_recipients.empty(); }
    bool isUsed() const { return m_used; }
    void markAsUsed() { m_used = true; }
    KeyModifiers keyModifiers() const { return m_modifiers; }
    bool shiftPressed() const { return (m_modifiers & kKeyShiftModifier) == kKeyShiftModifier; }
    bool ctrlPressed() const { return (m_modifiers & kKeyCtrlModifier) == kKeyCtrlModifier; }
    bool altPressed() const { return (m_modifiers & kKeyAltModifier) == kKeyAltModifier; }
    bool onlyShiftPressed() const { return (m_modifiers & kKeyShiftModifier) == kKeyShiftModifier; }
    bool onlyCtrlPressed() const { return (m_modifiers & kKeyCtrlModifier) == kKeyCtrlModifier; }
    bool onlyAltPressed() const { return (m_modifiers & kKeyAltModifier) == kKeyAltModifier; }

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

  class KeyMessage : public Message
  {
  public:
    KeyMessage(MessageType type, KeyScancode scancode, int unicodeChar, int repeat);

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

  // Deprecated
  KeyMessage* create_message_from_readkey_value(MessageType type, int readkey_value);

  class PaintMessage : public Message
  {
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

  class MouseMessage : public Message
  {
  public:
    MouseMessage(MessageType type, MouseButtons buttons, const gfx::Point& pos)
      : Message(type), m_buttons(buttons), m_pos(pos) {
    }

    MouseButtons buttons() const { return m_buttons; }
    bool left() const { return (m_buttons & kButtonLeft) == kButtonLeft; }
    bool right() const { return (m_buttons & kButtonRight) == kButtonRight; }
    bool middle() const { return (m_buttons & kButtonMiddle) == kButtonMiddle; }

    const gfx::Point& position() const { return m_pos; }

  private:
    MouseButtons m_buttons;     // Pressed buttons
    gfx::Point m_pos;           // Mouse position
  };

  class TimerMessage : public Message
  {
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

  class DropFilesMessage : public Message
  {
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
