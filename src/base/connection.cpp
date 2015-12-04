// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/connection.h"

#include "base/signal.h"

namespace base {

void Connection::disconnect()
{
  if (!m_slot)
    return;

  m_signal->disconnectSlot(m_slot);
  delete m_slot;
  m_slot = NULL;
}

} // namespace base
