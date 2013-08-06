// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fs.h"

#ifdef _WIN32
  #include "base/fs_win32.h"
#else
  #include "base/fs_unix.h"
#endif
