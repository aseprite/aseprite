// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/message.h"

#include "base/memory.h"
#include "ui/manager.h"
#include "ui/widget.h"

#include <allegro/keyboard.h>
#include <string.h>

namespace ui {

Message::Message(MessageType type)
  : m_type(type)
  , m_used(false)
  , m_modifiers((KeyModifiers)((key[KEY_LSHIFT] || key[KEY_RSHIFT] ? kKeyShiftModifier: 0) |
                               (key[KEY_LCONTROL] || key[KEY_RCONTROL] ? kKeyCtrlModifier: 0) |
                               (key[KEY_ALT] ? kKeyAltModifier: 0)))
{
}

Message::~Message()
{
}

void Message::addRecipient(Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  m_recipients.push_back(widget);
}

void Message::prependRecipient(Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  m_recipients.insert(m_recipients.begin(), widget);
}

void Message::removeRecipient(Widget* widget)
{
  for (WidgetsList::iterator
         it = m_recipients.begin(),
         end = m_recipients.end(); it != end; ++it) {
    if (*it == widget)
      *it = NULL;
  }
}

void Message::broadcastToChildren(Widget* widget)
{
  ASSERT_VALID_WIDGET(widget);

  UI_FOREACH_WIDGET(widget->getChildren(), it)
    broadcastToChildren(*it);

  addRecipient(widget);
}

KeyMessage::KeyMessage(MessageType type, KeyScancode scancode, int unicodeChar, int repeat)
  : Message(type)
  , m_scancode(scancode)
  , m_unicodeChar(unicodeChar)
  , m_repeat(repeat)
  , m_propagate_to_children(false)
  , m_propagate_to_parent(true)
{
}

KeyMessage* create_message_from_readkey_value(MessageType type, int readkey_value)
{
  return new KeyMessage(type,
                        static_cast<KeyScancode>((readkey_value >> 8) & 0xff),
                        (readkey_value & 0xff), 0);
}

} // namespace ui
