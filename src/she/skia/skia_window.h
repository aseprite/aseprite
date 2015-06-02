// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_WINDOW_INCLUDED
#define SHE_SKIA_SKIA_WINDOW_INCLUDED
#pragma once

#ifdef _WIN32
  #include "she/skia/skia_window_win.h"
#elif __APPLE__
  #include "she/skia/skia_window_osx.h"
#else
  #error There is no SkiaWindow implementation for your platform
#endif

#endif
