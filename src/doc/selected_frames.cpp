// Aseprite Document Library
// Copyright (c) 2016-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/selected_frames.h"

#include "base/base.h"
#include "base/debug.h"
#include "base/serialization.h"

#include <algorithm>
#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

std::size_t SelectedFrames::size() const
{
  std::size_t size = 0;
  for (auto& range : m_ranges)
    size += ABS(range.toFrame - range.fromFrame) + 1;
  return size;
}

void SelectedFrames::clear()
{
  m_ranges.clear();
}

// TODO this works only for forward ranges
void SelectedFrames::insert(frame_t frame)
{
  ASSERT(frame >= 0);

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

SelectedFrames SelectedFrames::filter(frame_t fromFrame, frame_t toFrame) const
{
  SelectedFrames f;

  if (fromFrame > toFrame)
    std::swap(fromFrame, toFrame);

  for (const auto& range : m_ranges) {
    FrameRange r(range);
    const bool isForward = (r.fromFrame <= r.toFrame);

    if (isForward) {
      if (r.fromFrame < fromFrame) r.fromFrame = fromFrame;
      if (r.toFrame > toFrame) r.toFrame = toFrame;
    }
    else {
      if (r.fromFrame > toFrame) r.fromFrame = toFrame;
      if (r.toFrame < fromFrame) r.toFrame = fromFrame;
    }

    if (( isForward && r.fromFrame <= r.toFrame) ||
        (!isForward && r.fromFrame >= r.toFrame))
      f.m_ranges.push_back(r);
  }

  return f;
}

// TODO this works only for forward ranges
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

void SelectedFrames::displace(frame_t frameDelta)
{
  // Avoid setting negative numbers in frame ranges
  if (firstFrame()+frameDelta < 0)
    frameDelta = -firstFrame();

  for (auto& range : m_ranges) {
    range.fromFrame += frameDelta;
    range.toFrame += frameDelta;

    ASSERT(range.fromFrame >= 0);
    ASSERT(range.toFrame >= 0);
  }
}

SelectedFrames SelectedFrames::makeReverse() const
{
  SelectedFrames newFrames;
  for (const FrameRange& range : m_ranges)
    newFrames.m_ranges.insert(
      newFrames.m_ranges.begin(),
      FrameRange(range.toFrame, range.fromFrame));
  return newFrames;
}

SelectedFrames SelectedFrames::makePingPong() const
{
  SelectedFrames newFrames = *this;
  const int n = m_ranges.size();
  int i = 0;
  int j = m_ranges.size()-1;

  for (const FrameRange& range : m_ranges) {
    FrameRange reversedRange(range.toFrame,
                             range.fromFrame);

    if (i == 0) reversedRange.toFrame++;
    if (j == 0) reversedRange.fromFrame--;

    if (reversedRange.fromFrame >= reversedRange.toFrame)
      newFrames.m_ranges.insert(
        newFrames.m_ranges.begin() + n,
        reversedRange);

    ++i;
    --j;
  }

  return newFrames;
}

bool SelectedFrames::write(std::ostream& os) const
{
  write32(os, size());
  for (const frame_t frame : *this) {
    write32(os, frame);
  }
  return os.good();
}

bool SelectedFrames::read(std::istream& is)
{
  clear();

  int nframes = read32(is);
  for (int i=0; i<nframes && is; ++i) {
    frame_t frame = read32(is);
    insert(frame);
  }
  return is.good();
}

} // namespace doc
