// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_LOG_H_INCLUDED
#define APP_CRASH_LOG_H_INCLUDED
#pragma once

#include "base/debug.h"

#ifdef _DEBUG
  #define RECO_TRACE(...) TRACE(__VA_ARGS__)
#else
  #define RECO_TRACE(...)
#endif

#endif
