// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef VIEW_FRAMES_H_INCLUDED
#define VIEW_FRAMES_H_INCLUDED
#pragma once

namespace view {

// We have two kind of frames: one real (matches the sprite frames),
// and one virtual (matches the columns of the timeline)
enum fr_t : int {};  // real frame (to reference real sprite frames)
enum col_t : int {}; // timeline column / virtual frame

static constexpr const col_t kNoCol = col_t(-1);

} // namespace view

#endif // VIEW_FRAMES_H_INCLUDED
