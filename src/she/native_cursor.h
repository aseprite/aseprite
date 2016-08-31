// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_NATIVE_CURSOR_H_INCLUDED
#define SHE_NATIVE_CURSOR_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace she {

  enum NativeCursor {
    kNoCursor,
    kArrowCursor,
    kCrosshairCursor,
    kIBeamCursor,
    kWaitCursor,
    kLinkCursor,
    kHelpCursor,
    kForbiddenCursor,
    kMoveCursor,
    kSizeNSCursor,
    kSizeWECursor,
    kSizeNCursor,
    kSizeNECursor,
    kSizeECursor,
    kSizeSECursor,
    kSizeSCursor,
    kSizeSWCursor,
    kSizeWCursor,
    kSizeNWCursor,
  };

} // namespace she

#endif
