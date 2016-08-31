// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CURSOR_TYPE_H_INCLUDED
#define UI_CURSOR_TYPE_H_INCLUDED
#pragma once

namespace ui {

  enum CursorType {
    kOutsideDisplay = -1,

    kFirstCursorType = 0,
    kNoCursor = 0,
    kArrowCursor,
    kArrowPlusCursor,
    kCrosshairCursor,
    kForbiddenCursor,
    kHandCursor,
    kScrollCursor,
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

    kRotateNCursor,
    kRotateNECursor,
    kRotateECursor,
    kRotateSECursor,
    kRotateSCursor,
    kRotateSWCursor,
    kRotateWCursor,
    kRotateNWCursor,

    kEyedropperCursor,
    kMagnifierCursor,

    kLastCursorType = kMagnifierCursor,
    kCursorTypes,
  };

} // namespace ui

#endif
