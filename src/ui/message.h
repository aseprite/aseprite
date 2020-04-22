// Aseprite UI Library
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_MESSAGE_H_INCLUDED
#define UI_MESSAGE_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/keys.h"
#include "ui/message_type.h"
#include "ui/mouse_button.h"
#include "ui/pointer_type.h"

namespace ui {

  class Timer;
  class Widget;

  class Message {
    enum Flags {
      FromFilter          = 1,  // Sent from pre-filter
      PropagateToChildren = 2,
      PropagateToParent   = 4,
    };
  public:
    Message(MessageType type,
            KeyModifiers modifiers = kKeyUninitializedModifier);
    virtual ~Message();

    MessageType type() const { return m_type; }
    Widget* recipient() const { return m_recipient; }
    bool fromFilter() const { return hasFlag(FromFilter); }
    void setFromFilter(const bool state) { setFlag(FromFilter, state); }
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

    void setRecipient(Widget* widget);
    void removeRecipient(Widget* widget);

    bool propagateToChildren() const { return hasFlag(PropagateToChildren); }
    bool propagateToParent() const { return hasFlag(PropagateToParent); }
    void setPropagateToChildren(const bool state) { setFlag(PropagateToChildren, state); }
    void setPropagateToParent(const bool state) { setFlag(PropagateToParent, state); }

    Widget* commonAncestor() { return m_commonAncestor; }
    void setCommonAncestor(Widget* widget) { m_commonAncestor = widget; }

  private:
    bool hasFlag(const Flags flag) const {
      return (m_flags & flag) == flag;
    }
    void setFlag(const Flags flag, const bool state) {
      m_flags = (state ? (m_flags | flag):
                         (m_flags & ~flag));
    }

    MessageType m_type;       // Type of message
    int m_flags;              // Special flags for this message
    Widget* m_recipient;      // Recipient of this message
    Widget* m_commonAncestor; // Common ancestor between the Leave <-> Enter messages
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
    bool isDeadKey() const { return m_isDead; }
    void setDeadKey(bool state) { m_isDead = state; }

  private:
    KeyScancode m_scancode;
    int m_unicodeChar;
    int m_repeat; // repeat=0 means the first time the key is pressed
    bool m_isDead;
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
                 MouseButton button,
                 KeyModifiers modifiers,
                 const gfx::Point& pos,
                 const gfx::Point& wheelDelta = gfx::Point(0, 0),
                 bool preciseWheel = false,
                 float pressure = 0.0f)
      : Message(type, modifiers),
        m_pointerType(pointerType),
        m_button(button),
        m_pos(pos),
        m_wheelDelta(wheelDelta),
        m_preciseWheel(preciseWheel),
        m_pressure(pressure) {
    }

    PointerType pointerType() const { return m_pointerType; }
    MouseButton button() const { return m_button; }
    bool left() const { return (m_button == kButtonLeft); }
    bool right() const { return (m_button == kButtonRight); }
    bool middle() const { return (m_button == kButtonMiddle); }
    gfx::Point wheelDelta() const { return m_wheelDelta; }
    bool preciseWheel() const { return m_preciseWheel; }
    float pressure() const { return m_pressure; }

    const gfx::Point& position() const { return m_pos; }

  private:
    PointerType m_pointerType;
    MouseButton m_button;       // Pressed button
    gfx::Point m_pos;           // Mouse position
    gfx::Point m_wheelDelta;    // Wheel axis variation
    bool m_preciseWheel;
    float m_pressure;
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
    DropFilesMessage(const base::paths& files)
      : Message(kDropFilesMessage)
      , m_files(files) {
    }

    const base::paths& files() const { return m_files; }

  private:
    base::paths m_files;
  };

} // namespace ui

#endif
