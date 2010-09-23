// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro/unicode.h>

char *get_pretty_memory_size(unsigned int memsize, char *buf, unsigned int bufsize)
{
  if (memsize < 1000)
    uszprintf(buf, bufsize, "%d bytes", memsize);
  else if (memsize < 1000*1000)
    uszprintf(buf, bufsize, "%0.1fK", memsize/1024.0f);
  else
    uszprintf(buf, bufsize, "%0.1fM", memsize/(1024.0f*1024.0f));

  return buf;
}
