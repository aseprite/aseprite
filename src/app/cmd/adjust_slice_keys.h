
#ifndef APP_CMD_MOVE_SLICE_H_INCLUDED
#define APP_CMD_MOVE_SLICE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

#include <sstream>

namespace app { namespace cmd {
using namespace doc;

class AdjustSliceKeys : public Cmd,
                 public WithSprite {
public:
  AdjustSliceKeys(Sprite* sprite, int frame);

protected:
  void moveSliceKeysBack(Sprite* sprite, frame_t frame);
  void moveSliceKeysForward(Sprite* sprite, frame_t frame);

  int frame;
  size_t m_size;
  std::stringstream m_stream;
};

class DeleteSliceKeys : public AdjustSliceKeys {
public:
  DeleteSliceKeys(Sprite* sprite, int frame);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_size; }
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
  size_t onMemSize() const override { return sizeof(*this) + m_size; }

  int targetFrame;
};

}} // namespace app::cmd

#endif
