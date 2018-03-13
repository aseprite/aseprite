// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef FILTERS_TARGET_H_INCLUDED
#define FILTERS_TARGET_H_INCLUDED
#pragma once

#define TARGET_RED_CHANNEL              1
#define TARGET_GREEN_CHANNEL            2
#define TARGET_BLUE_CHANNEL             4
#define TARGET_ALPHA_CHANNEL            8
#define TARGET_GRAY_CHANNEL             16
#define TARGET_INDEX_CHANNEL            32

#define TARGET_ALL_CHANNELS             \
  (TARGET_RED_CHANNEL           |       \
   TARGET_GREEN_CHANNEL         |       \
   TARGET_BLUE_CHANNEL          |       \
   TARGET_ALPHA_CHANNEL         |       \
   TARGET_GRAY_CHANNEL          )

namespace filters {

  typedef int Target;

} // namespace filters

#endif
