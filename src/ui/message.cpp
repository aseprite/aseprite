// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
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

#include <cstring>

namespace ui {

Message::Message(MessageType type, KeyModifiers modifiers)
  : m_type(type)
  , m_used(false)
{
  if (modifiers == kKeyUninitializedModifier) {
    // Get modifiers from the deprecated API
    // TODO remove this
    m_modifiers = (KeyModifiers)
      ((she::is_key_pressed(kKeyLShift) || she::is_key_pressed(kKeyRShift) ? kKeyShiftModifier: 0) |
       (she::is_key_pressed(kKeyLControl) || she::is_key_pressed(kKeyRControl) ? kKeyCtrlModifier: 0) |
       (she::is_key_pressed(kKeyAlt) ? kKeyAltModifier: 0) |
       (she::is_key_pressed(kKeyCommand) ? kKeyCmdModifier: 0) |
       (she::is_key_pressed(kKeySpace) ? kKeySpaceModifier: 0) |
       (she::is_key_pressed(kKeyLWin) || she::is_key_pressed(kKeyRWin) ? kKeyWinModifier: 0));
  }
  else {
    m_modifiers = modifiers;
  }
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

  for (auto child : widget->children())
    broadcastToChildren(child);

  addRecipient(widget);
}

KeyMessage::KeyMessage(MessageType type,
                       KeyScancode scancode,
                       KeyModifiers modifiers,
                       int unicodeChar, int repeat)
  : Message(type, modifiers)
  , m_scancode(scancode)
  , m_unicodeChar(unicodeChar)
  , m_repeat(repeat)
  , m_propagate_to_children(false)
  , m_propagate_to_parent(true)
{
}

} // namespace ui
