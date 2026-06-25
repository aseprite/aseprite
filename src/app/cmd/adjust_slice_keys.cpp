// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/adjust_slice_keys.h"

#include "doc/slice.h"
#include "doc/sprite.h"

#include <vector>

namespace app { namespace cmd {

using namespace doc;

AdjustSliceKeys::AdjustSliceKeys(Sprite* sprite, int frame) : WithSprite(sprite), m_frame(frame)
{
}

DeleteSliceKeys::DeleteSliceKeys(Sprite* sprite, int frame) : AdjustSliceKeys(sprite, frame)
{
}

InsertSliceKeys::InsertSliceKeys(Sprite* sprite, int frame) : AdjustSliceKeys(sprite, frame)
{
}

MoveSliceKeys::MoveSliceKeys(Sprite* sprite, int frame, int targetFrame)
  : AdjustSliceKeys(sprite, frame)
  , m_targetFrame(targetFrame)
{
}

void DeleteSliceKeys::onExecute()
{
  Sprite* sprite = this->sprite();

  m_keys.clear();
  moveSliceKeysBack(sprite, m_frame, true);

  sprite->incrementVersion();
}

void InsertSliceKeys::onExecute()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysForward(sprite, m_frame);

  sprite->incrementVersion();
}

void MoveSliceKeys::onExecute()
{
  Sprite* sprite = this->sprite();

  m_keys.clear();
  moveSliceKeysBack(sprite, m_frame, true);
  moveSliceKeysForward(sprite, m_targetFrame);
  restoreKeys(sprite, m_targetFrame);

  sprite->incrementVersion();
}

void DeleteSliceKeys::onUndo()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysForward(sprite, m_frame);
  restoreKeys(sprite, m_frame);

  sprite->incrementVersion();
}

void InsertSliceKeys::onUndo()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysBack(sprite, m_frame, false);

  sprite->incrementVersion();
}

void MoveSliceKeys::onUndo()
{
  Sprite* sprite = this->sprite();

  moveSliceKeysBack(sprite, m_targetFrame, false);
  moveSliceKeysForward(sprite, m_frame);
  restoreKeys(sprite, m_frame);

  sprite->incrementVersion();
}

void AdjustSliceKeys::moveSliceKeysBack(Sprite* sprite,
                                        const frame_t from,
                                        const bool storeRemovedKeys)
{
  for (Slice* slice : sprite->slices()) {
    auto it = slice->getIteratorByFrame(from);
    if (it == slice->end())
      continue;

    if (it->frame() == from) {
      if (storeRemovedKeys)
        m_keys.push_back(StoredKey{ slice->id(), *it->value() });
      slice->remove(from);
    }

    for (it = slice->begin(); it != slice->end(); ++it) {
      if (it->frame() > from)
        it->setFrame(it->frame() - 1);
    }

    slice->incrementVersion();
  }
}

void AdjustSliceKeys::moveSliceKeysForward(Sprite* sprite, const frame_t from)
{
  for (Slice* slice : sprite->slices()) {
    std::vector<frame_t> frames;
    for (auto it = slice->begin(); it != slice->end(); ++it) {
      if (it->frame() >= from)
        frames.push_back(it->frame());
    }

    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
      auto keyIt = slice->getIteratorByFrame(*it);
      if (keyIt != slice->end() && keyIt->frame() == *it)
        keyIt->setFrame(*it + 1);
    }

    slice->incrementVersion();
  }
}

void AdjustSliceKeys::restoreKeys(Sprite* sprite, const frame_t frame)
{
  for (const auto& key : m_keys) {
    for (Slice* slice : sprite->slices()) {
      if (slice->id() == key.sliceId) {
        slice->insert(frame, key.key);
        slice->incrementVersion();
        break;
      }
    }
  }
}

}} // namespace app::cmd
