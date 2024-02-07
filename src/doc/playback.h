// Aseprite Document Library
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PLAYBACK_H_INCLUDED
#define DOC_PLAYBACK_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/tag.h"
#include "doc/tags.h"

#include <memory>
#include <set>
#include <vector>

namespace doc {

  class Sprite;
  class Tag;

  class Playback {
  public:
    enum Mode {
      // Play all the animation until it ends (infinite loops just
      // played once). Useful to export GIF files with tags+loops.
      PlayAll,
      // Regular playback mode for a sprite editor when we start
      // playing in no tag (infinite loops are just played once, tags
      // with repeat are respected). We start from the given frame on
      // Playback() ctor.
      PlayInLoop,
      // Play all frames ignoring tags in loop from the first frame to
      // the last one.
      PlayWithoutTagsInLoop,
      // Play once the full sprite or the given tag from beginning to
      // end (ignoring starting frame, but we back to the starting
      // frame when the animation stops). Useful to play the full
      // sprite/current tag in one snapshot.
      PlayOnce,
      // We reached the end of the playback (generally we reach this
      // state after a full PlayAll or PlayOnce, and never in a
      // PlayInLoop).
      Stopped,
    };

    Playback(const Sprite* sprite,
             const TagsList& tagsList,
             const frame_t frame,
             const Mode playMode,
             const Tag* tag,
             const int forward = 1);

    Playback(const Sprite* sprite = nullptr,
             const frame_t frame = 0,
             const Mode playMode = PlayAll,
             const Tag* tag = nullptr);

    frame_t initialFrame() const { return m_initialFrame; }
    frame_t frame() const { return m_frame; }

    bool isStopped() const { return m_playMode == Mode::Stopped; }
    void stop();

    // Sets the list of possible tags to iterate/enter when we are
    // playing. By default are all the available sprite tags.
    void setTags(const TagsList& tags) {
      m_tags = tags;
    }

    // If "delta" is +1, it's the next frame, if it's -1, it's the
    // previous frame, etc.
    frame_t nextFrame(frame_t frameDelta = frame_t(+1));

    // The tag that is being played right now (can be nullptr).
    Tag* tag() const;

    // Should be called when a specific tag is going to be deleted so
    // we remove all references to this tag.
    void removeReferencesToTag(Tag* tag);

  private:
    // Information about playing tags (and inner tags)
    struct PlayTag {
      const Tag* tag = nullptr;
      int forward = 1;
      int repeat = 0;
      // True if we have to go to the first tag frame when we enter to
      // this PlayTag. Used for overlapped tags, e.g.
      //  A
      // ---> B
      //   ---->
      // 0 1 2 3
      //
      // The tag "B" will have rewind=true, because when we finish
      // "A", we should start from the beginning of "B".
      bool rewind = false;

      // Indicates what PlayTag deletes this PlayTag.
      PlayTag* delayedDelete = nullptr;

      // This PlayTag will remove the following tags from m_played.
      std::vector<const Tag*> removeThese;

      PlayTag(const Tag* tag, int parentForward);
      void invertForward() { forward = -forward; }
    };

    void handleEnterFrame(const frame_t frameDelta, const bool firstTime);
    bool handleExitFrame(const frame_t frameDelta);
    void handleMoveFrame(const frame_t frameDelta);
    void addTag(const Tag* tag,
                const bool rewind,
                const int forward);
    void removeLastTagFromPlayed();
    bool decrementRepeat(const frame_t frameDelta);
    frame_t firstTagFrame(const Tag* tag);
    frame_t lastTagFrame(const Tag* tag);
    void goToFirstTagFrame(const Tag* tag);
    int getParentForward() const;

    const Sprite* m_sprite;

    // List of possible tags that can be played/iterated.
    TagsList m_tags;

    frame_t m_initialFrame;
    frame_t m_frame;
    Mode m_playMode;
    int m_forward;

    // Queue of tags to play and tags that are being played
    std::vector<std::unique_ptr<PlayTag>> m_playing;
    std::set<const Tag*> m_played;
  };

} // namespace doc

#endif
