// LAF OS Library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/debug.h"
#include "os/system.h"

namespace os {

static System* g_system = nullptr;

System* create_system_impl();   // Defined on each back-end

System* create_system()
{
  ASSERT(!g_system);
  return g_system = create_system_impl();
}

System* instance()
{
  return g_system;
}

} // namespace os
