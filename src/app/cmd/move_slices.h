
#ifndef APP_CMD_MOVE_SLICE_H_INCLUDED
#define APP_CMD_MOVE_SLICE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

#include <sstream>

namespace app { namespace cmd {
using namespace doc;

class MoveSlices : public Cmd,
                 public WithSprite {
public:
  MoveSlices(Sprite* sprite, int delta, int frame, int targetFrame);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_size; }

private:
  void moveSlicesBack(Sprite* sprite, frame_t frame);
  void moveSlicesForward(Sprite* sprite, frame_t frame);

  int delta, frame, targetFrame;
  size_t m_size;
  std::stringstream m_stream;
};

}} // namespace app::cmd

#endif
