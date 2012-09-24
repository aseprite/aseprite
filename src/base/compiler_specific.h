// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_COMPILER_SPECIFIC_H_INCLUDED
#define BASE_COMPILER_SPECIFIC_H_INCLUDED

#ifdef _MSC_VER
  #define OVERRIDE override
#else
  #define OVERRIDE
#endif

#endif
