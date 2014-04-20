// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_COMPILER_SPECIFIC_H_INCLUDED
#define BASE_COMPILER_SPECIFIC_H_INCLUDED
#pragma once

#ifdef _MSC_VER
  #define OVERRIDE override
#else
  #define OVERRIDE
#endif

#endif
