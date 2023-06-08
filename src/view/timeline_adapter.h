// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef VIEW_TIMELINE_ADAPTER_H_INCLUDED
#define VIEW_TIMELINE_ADAPTER_H_INCLUDED
#pragma once

#include "doc/sprite.h"
#include "doc/tag.h"
#include "view/frames.h"

namespace view {

// Tries to create an abstract way to access timeline frames (or
// layers) so then we can change the kind of timeline view depending
// on some kind of configuration (e.g. collapsing a tag/hiding frames
// needs some kind of adapter between columns <-> frames, or "virtual
// frames/columns" <-> "real frames").
class TimelineAdapter {
public:
  virtual ~TimelineAdapter() { }

  // Total number of columns in the timeline.
  virtual col_t totalFrames() const = 0;

  // Frame duration of the specific column.
  virtual int frameDuration(col_t frame) const = 0;

  // Returns the real frame that the given column is showing.
  virtual fr_t toRealFrame(col_t frame) const = 0;

  // Returns the column in the timeline that represents the given real
  // frame of the sprite. Returns -1 if the frame is not visible given
  // the current timeline filters/hidden frames.
  virtual col_t toColFrame(fr_t frame) const = 0;

  // Returns true if this tag is being used for this specific timeline
  // adapter/view. E.g. if a tag is going to be removed and this
  // timeline view is using that tag, we must switch to other kind of
  // timeline view that doesn't use this tag anymore.
  virtual bool isViewingTag(doc::Tag* tag) const = 0;
};

// Represents the default timeline view where the whole sprite is
// visible (all frames/all layers/open groups).
class FullSpriteTimelineAdapter : public TimelineAdapter {
public:
  FullSpriteTimelineAdapter(doc::Sprite* sprite) : m_sprite(sprite) { }
  col_t totalFrames() const override { return col_t(m_sprite->totalFrames()); }
  int frameDuration(col_t frame) const override { return m_sprite->frameDuration(frame); }
  fr_t toRealFrame(col_t frame) const override { return fr_t(frame); }
  col_t toColFrame(fr_t frame) const override { return col_t(frame); }
  bool isViewingTag(doc::Tag*) const override { return false; }
private:
  doc::Sprite* m_sprite = nullptr;
};

// Represents an alternative timeline to show only the specified tag.
class ShowTagTimelineAdapter : public TimelineAdapter {
public:
  ShowTagTimelineAdapter(doc::Sprite* sprite,
                         doc::Tag* tag) : m_sprite(sprite)
                                        , m_tag(tag) { }
  col_t totalFrames() const override { return col_t(m_tag->frames()); }
  int frameDuration(col_t frame) const override {
    return m_sprite->frameDuration(doc::frame_t(toRealFrame(frame)));
  }
  fr_t toRealFrame(col_t frame) const override {
    return fr_t(frame + m_tag->fromFrame());
  }
  col_t toColFrame(fr_t frame) const override {
    if (m_tag->contains(frame))
      return col_t(frame - m_tag->fromFrame());
    else
      return kNoCol;
  }
  bool isViewingTag(doc::Tag* tag) const override {
    return m_tag == tag;
  }
private:
  doc::Sprite* m_sprite = nullptr;
  doc::Tag* m_tag = nullptr;
};

} // namespace view

#endif  // VIEW_TIMELINE_ADAPTER_H_INCLUDED
