// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_INTS_H_INCLUDED
#define BASE_INTS_H_INCLUDED
#pragma once

#include "base/config.h"

#ifdef HAVE_STDINT_H
  #include <stdint.h>
#else
  #error uint8_t, uint32_t, etc. definitions are missing
#endif

#endif
