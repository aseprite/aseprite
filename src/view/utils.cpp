// Aseprite View Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "view/utils.h"

#include "doc/selected_frames.h"
#include "view/timeline_adapter.h"

namespace view {

VirtualRange to_virtual_range(const TimelineAdapter* adapter, const RealRange& realRange)
{
  VirtualRange range(realRange);
  if (range.enabled()) {
    doc::SelectedFrames frames;
    for (auto frame : realRange.selectedFrames()) {
      col_t col = adapter->toColFrame(fr_t(frame));
      if (col != kNoCol)
        frames.insert(doc::frame_t(col));
    }
    range.setSelectedFrames(frames, false);
  }
  return range;
}

RealRange to_real_range(const TimelineAdapter* adapter, const VirtualRange& virtualRange)
{
  RealRange range(virtualRange);
  if (range.enabled()) {
    doc::SelectedFrames frames;
    for (auto frame : virtualRange.selectedFrames())
      frames.insert(doc::frame_t(adapter->toRealFrame(col_t(frame))));
    range.setSelectedFrames(frames, false);
  }
  return range;
}

} // namespace view
