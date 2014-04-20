// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>

namespace base {

// Reads a WORD (16 bits) using in little-endian byte ordering.
int fgetw(FILE* file)
{
  int b1, b2;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  // Little endian.
  return ((b2 << 8) | b1);
}

// Reads a DWORD (32 bits) using in little-endian byte ordering.
long fgetl(FILE* file)
{
  int b1, b2, b3, b4;

  b1 = fgetc(file);
  if (b1 == EOF)
    return EOF;

  b2 = fgetc(file);
  if (b2 == EOF)
    return EOF;

  b3 = fgetc(file);
  if (b3 == EOF)
    return EOF;

  b4 = fgetc(file);
  if (b4 == EOF)
    return EOF;

  // Little endian.
  return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

// Writes a word using in little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputw(int w, FILE* file)
{
  int b1, b2;

  // Little endian.
  b2 = (w & 0xFF00) >> 8;
  b1 = w & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      return 0;

  return -1;
}

// Writes DWORD a using in little-endian byte ordering.
// Returns 0 in success or -1 in error
int fputl(long l, FILE* file)
{
  int b1, b2, b3, b4;

  // Little endian.
  b4 = (int)((l & 0xFF000000L) >> 24);
  b3 = (int)((l & 0x00FF0000L) >> 16);
  b2 = (int)((l & 0x0000FF00L) >> 8);
  b1 = (int)l & 0x00FF;

  if (fputc(b1, file) == b1)
    if (fputc(b2, file) == b2)
      if (fputc(b3, file) == b3)
        if (fputc(b4, file) == b4)
          return 0;

  return -1;
}

} // namespace base
