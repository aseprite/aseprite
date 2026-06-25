// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADJUST_SLICE_KEYS_H_INCLUDED
#define APP_CMD_ADJUST_SLICE_KEYS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"
#include "doc/object_id.h"
#include "doc/slice.h"

#include <vector>

namespace app { namespace cmd {
using namespace doc;

class AdjustSliceKeys : public Cmd,
                        public WithSprite {
public:
  AdjustSliceKeys(Sprite* sprite, int frame);

protected:
  struct StoredKey {
    ObjectId sliceId;
    SliceKey key;
  };

  void moveSliceKeysBack(Sprite* sprite, frame_t frame, bool storeRemovedKeys);
  void moveSliceKeysForward(Sprite* sprite, frame_t frame);
  void restoreKeys(Sprite* sprite, frame_t frame);

  int m_frame;
  std::vector<StoredKey> m_keys;
};

class DeleteSliceKeys : public AdjustSliceKeys {
public:
  DeleteSliceKeys(Sprite* sprite, int frame);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_keys.size() * sizeof(StoredKey); }
};

class InsertSliceKeys : public AdjustSliceKeys {
public:
  InsertSliceKeys(Sprite* sprite, int frame);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this); }
};

class MoveSliceKeys : public AdjustSliceKeys {
public:
  MoveSliceKeys(Sprite* sprite, int frame, int targetFrame);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_keys.size() * sizeof(StoredKey); }

  int m_targetFrame;
};

}} // namespace app::cmd

#endif
