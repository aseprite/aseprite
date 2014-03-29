// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_CURSOR_TYPE_H_INCLUDED
#define UI_CURSOR_TYPE_H_INCLUDED
#pragma once

namespace ui {

  enum CursorType {
    kFirstCursorType = 0,
    kNoCursor = 0,
    kArrowCursor,
    kArrowPlusCursor,
    kForbiddenCursor,
    kHandCursor,
    kScrollCursor,
    kMoveCursor,
    kSizeTLCursor,
    kSizeTCursor,
    kSizeTRCursor,
    kSizeLCursor,
    kSizeRCursor,
    kSizeBLCursor,
    kSizeBCursor,
    kSizeBRCursor,
    kRotateTLCursor,
    kRotateTCursor,
    kRotateTRCursor,
    kRotateLCursor,
    kRotateRCursor,
    kRotateBLCursor,
    kRotateBCursor,
    kRotateBRCursor,
    kEyedropperCursor,
    kLastCursorType = kEyedropperCursor,
    kCursorTypes
  };

} // namespace ui

#endif
