// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_CFILE_H_INCLUDED
#define BASE_CFILE_H_INCLUDED
#pragma once

#include <cstdio>

namespace base {

  int fgetw(FILE* file);
  long fgetl(FILE* file);
  int fputw(int w, FILE* file);
  int fputl(long l, FILE* file);
  
} // namespace base

#endif
