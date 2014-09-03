// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>

static volatile int clock_var = 0; // Global timer.

static void clock_inc()
{
  ++clock_var;
}

END_OF_STATIC_FUNCTION(clock_inc);

namespace she {

void clock_init()
{
  // Install timer related stuff
  LOCK_VARIABLE(clock_var);
  LOCK_FUNCTION(clock_inc);

  install_int_ex(clock_inc, BPS_TO_TIMER(1000));
}

void clock_exit()
{
  remove_int(clock_inc);
}

int clock_value()
{
  return clock_var;
}

} // namespace she
