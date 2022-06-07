// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DRM_H_INCLUDED
#define APP_DRM_H_INCLUDED
#pragma once

#ifdef ENABLE_DRM
  #include "drm/drm.h"
#else
  #define DRM_INVALID if (false)
#endif

#endif
