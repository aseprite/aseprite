// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/register_message.h"

namespace ui {

static int registered_messages = (int)kFirstRegisteredMessage;

RegisterMessage::RegisterMessage()
{
  m_type = (MessageType)registered_messages++;
}

} // namespace ui
