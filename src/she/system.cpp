// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/system.h"

namespace she {

System* instance()
{
  static System* sys = nullptr;
  if (!sys)
    sys = create_system();
  return sys;
}

} // namespace she
