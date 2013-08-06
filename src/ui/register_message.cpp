// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

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
