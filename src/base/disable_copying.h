// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_DISABLE_COPYING_H_INCLUDED
#define BASE_DISABLE_COPYING_H_INCLUDED
#pragma once

#define DISABLE_COPYING(ClassName)              \
  private:                                      \
    ClassName(const ClassName&);                \
    ClassName& operator=(const ClassName&);

#endif
