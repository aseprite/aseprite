// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/x11/x11.h"

namespace she {

X11* X11::m_instance = nullptr;

// static
X11* X11::instance()
{
  ASSERT(m_instance);
  return m_instance;
}

} // namespace she
