// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/selected_frames.h"

#include <algorithm>

namespace doc {

std::size_t SelectedFrames::size() const
{
  std::size_t size = 0;
  for (auto& range : m_ranges)
    size += (range.toFrame - range.fromFrame + 1);
  return size;
}

void SelectedFrames::clear()
{
  m_ranges.clear();
}

void SelectedFrames::insert(frame_t frame)
{
  if (m_ranges.empty()) {
    m_ranges.push_back(FrameRange(frame));
    return;
  }

  auto it = m_ranges.begin();
  auto end = m_ranges.end();
  auto next = it;

  for (; it!=end; it=next) {
    if (frame >= it->fromFrame &&
        frame <= it->toFrame)
      break;

    if (frame < it->fromFrame) {
      if (frame == it->fromFrame-1)
        --it->fromFrame;
      else
        m_ranges.insert(it, FrameRange(frame));
      break;
    }

    ++next;

    if (frame > it->toFrame && (next == end || frame < next->fromFrame-1)) {
      if (frame == it->toFrame+1)
        ++it->toFrame;
      else
        m_ranges.insert(next, FrameRange(frame));
      break;
    }
  }
}

void SelectedFrames::insert(frame_t fromFrame, frame_t toFrame)
{
  if (fromFrame > toFrame)
    std::swap(fromFrame, toFrame);

  // TODO improve this
  for (frame_t frame = fromFrame; frame <= toFrame; ++frame) {
    insert(frame);
  }
}

bool SelectedFrames::contains(frame_t frame) const
{
  return std::binary_search(
    m_ranges.begin(),
    m_ranges.end(),
    FrameRange(frame),
    [](const FrameRange& a,
       const FrameRange& b) -> bool {
      return (a.toFrame < b.fromFrame);
    });
}

} // namespace doc
