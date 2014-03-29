// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <cstdio>

namespace base {

using namespace std;

string get_pretty_memory_size(size_t memsize)
{
  char buf[256];

  if (memsize < 1000) {
    sprintf(buf, "%lu bytes", memsize);
  }
  else if (memsize < 1000*1000) {
    sprintf(buf, "%0.1fK", memsize/1024.0f);
  }
  else {
    sprintf(buf, "%0.1fM", memsize/(1024.0f*1024.0f));
  }

  return buf;
}

} // namespace base
