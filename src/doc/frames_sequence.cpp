// Aseprite Document Library
// Copyright (c) 2023 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/frames_sequence.h"

#include "base/base.h"
#include "base/debug.h"
#include "base/serialization.h"

#include <algorithm>
#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

FramesSequence::FramesSequence(const SelectedFrames& selectedFrames)
{
  // TODO: This might be optimized by copying each range directly, but
  // for this I would need that SelectedFrames make this class its friend.
  for (auto frame : selectedFrames) {
    this->insert(frame);
  }
}

std::size_t FramesSequence::size() const
{
  std::size_t size = 0;
  for (auto& range : m_ranges)
    size += ABS(range.toFrame - range.fromFrame) + 1;
  return size;
}

void FramesSequence::clear()
{
  m_ranges.clear();
}

void FramesSequence::insert(frame_t frame)
{
  ASSERT(frame >= 0);

  if (m_ranges.empty()) {
    m_ranges.push_back(FrameRange(frame));
    return;
  }

  auto& lastRange = m_ranges.back();
  int rangeStep = (lastRange.fromFrame < lastRange.toFrame) -
                  (lastRange.fromFrame > lastRange.toFrame);
  int step = (lastRange.toFrame < frame) - (lastRange.toFrame > frame);
  if (step && (rangeStep == step || !rangeStep) && frame - lastRange.toFrame == step)
    lastRange.toFrame = frame;
  else
    m_ranges.push_back(FrameRange(frame));
}

void FramesSequence::insert(frame_t fromFrame, frame_t toFrame)
{
  int step = (fromFrame <= toFrame ? 1 : -1);
  for (frame_t frame = fromFrame; frame != toFrame; frame += step) {
    insert(frame);
  }
  insert(toFrame);
}

FramesSequence FramesSequence::filter(frame_t fromFrame, frame_t toFrame) const
{
  FramesSequence f;

  if (fromFrame > toFrame)
    std::swap(fromFrame, toFrame);

  for (const auto& range : m_ranges) {
    FrameRange r(range);
    const bool isForward = (r.fromFrame <= r.toFrame);

    if (isForward) {
      if (r.fromFrame < fromFrame)
        r.fromFrame = fromFrame;
      if (r.toFrame > toFrame)
        r.toFrame = toFrame;
    }
    else {
      if (r.fromFrame > toFrame)
        r.fromFrame = toFrame;
      if (r.toFrame < fromFrame)
        r.toFrame = fromFrame;
    }

    if ((isForward && r.fromFrame <= r.toFrame) || (!isForward && r.fromFrame >= r.toFrame))
      f.m_ranges.push_back(r);
  }

  return f;
}

bool FramesSequence::contains(frame_t frame) const
{
  return std::find_if(m_ranges.begin(), m_ranges.end(), [frame](const FrameRange& r) -> bool {
           return (r.fromFrame <= frame && frame <= r.toFrame) ||
                  (r.fromFrame >= frame && frame >= r.toFrame);
         }) != m_ranges.end();
}

frame_t FramesSequence::lowestFrame() const
{
  frame_t lof = this->firstFrame();
  for (auto range : m_ranges) {
    lof = std::min<frame_t>(range.fromFrame, std::min<frame_t>(range.toFrame, lof));
  }
  return lof;
}

void FramesSequence::displace(frame_t frameDelta)
{
  // Avoid setting negative numbers in frame ranges
  auto lof = lowestFrame();
  if (lof + frameDelta < 0)
    frameDelta = -lof;

  for (auto& range : m_ranges) {
    range.fromFrame += frameDelta;
    range.toFrame += frameDelta;

    ASSERT(range.fromFrame >= 0);
    ASSERT(range.toFrame >= 0);
  }
}

FramesSequence FramesSequence::makeReverse() const
{
  FramesSequence newFrames;
  for (const FrameRange& range : m_ranges)
    newFrames.m_ranges.insert(newFrames.m_ranges.begin(),
                              FrameRange(range.toFrame, range.fromFrame));
  return newFrames;
}

FramesSequence FramesSequence::makePingPong() const
{
  FramesSequence newFrames = *this;
  const int n = m_ranges.size();
  int i = 0;
  int j = m_ranges.size() - 1;

  for (const FrameRange& range : m_ranges) {
    // Discard first or last range if it contains just one frame.
    if (range.toFrame == range.fromFrame && (i == 0 || j == 0)) {
      ++i;
      --j;
      continue;
    }

    FrameRange reversedRange(range.toFrame, range.fromFrame);
    int step = (range.fromFrame < range.toFrame) - (range.fromFrame > range.toFrame);

    if (i == 0)
      reversedRange.toFrame += step;
    if (j == 0)
      reversedRange.fromFrame -= step;

    if (reversedRange != range)
      newFrames.m_ranges.insert(newFrames.m_ranges.begin() + n, reversedRange);

    ++i;
    --j;
  }

  return newFrames;
}

bool FramesSequence::write(std::ostream& os) const
{
  write32(os, size());
  for (const frame_t frame : *this) {
    write32(os, frame);
  }
  return os.good();
}

bool FramesSequence::read(std::istream& is)
{
  clear();

  int nframes = read32(is);
  for (int i = 0; i < nframes && is; ++i) {
    frame_t frame = read32(is);
    insert(frame);
  }
  return is.good();
}

} // namespace doc
