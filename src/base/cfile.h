// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_CFILE_H_INCLUDED
#define BASE_CFILE_H_INCLUDED

#include <cstdio>

namespace base {

  int fgetw(FILE* file);
  long fgetl(FILE* file);
  int fputw(int w, FILE* file);
  int fputl(long l, FILE* file);
  
} // namespace base

#endif
