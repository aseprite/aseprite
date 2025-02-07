// Aseprite UI Library
// Copyright (C) 2018-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/message.h"

#include "base/memory.h"
#include "os/system.h"
#include "ui/display.h"
#include "ui/widget.h"

#include <cstring>

namespace ui {

Message::Message(MessageType type, KeyModifiers modifiers)
  : m_type(type)
  , m_flags(0)
  , m_display(nullptr)
  , m_recipient(nullptr)
  , m_commonAncestor(nullptr)
{
  if (modifiers == kKeyUninitializedModifier && os::instance())
    m_modifiers = os::instance()->keyModifiers();
  else
    m_modifiers = modifiers;
}

Message::~Message()
{
}

void Message::setDisplay(Display* display)
{
  m_display = display;
}

void Message::setRecipient(Widget* widget)
{
  ASSERT(m_recipient == nullptr);
  ASSERT_VALID_WIDGET(widget);
  m_recipient = widget;
}

void Message::removeRecipient(Widget* widget)
{
  if (m_recipient == widget)
    m_recipient = nullptr;
}

KeyMessage::KeyMessage(MessageType type,
                       KeyScancode scancode,
                       KeyModifiers modifiers,
                       int unicodeChar,
                       int repeat)
  : Message(type, modifiers)
  , m_scancode(scancode)
  , m_unicodeChar(unicodeChar)
  , m_repeat(repeat)
  , m_isDead(false)
{
  setPropagateToParent(true);
}

gfx::Point MouseMessage::positionForDisplay(Display* anotherDisplay) const
{
  if (display() == anotherDisplay) {
    return position(); // There is no need for transformation
  }
  else {
    ASSERT(anotherDisplay);
    ASSERT(anotherDisplay->nativeWindow());
    return anotherDisplay->nativeWindow()->pointFromScreen(screenPosition());
  }
}

gfx::Point MouseMessage::screenPosition() const
{
  return display()->nativeWindow()->pointToScreen(position());
}

} // namespace ui
