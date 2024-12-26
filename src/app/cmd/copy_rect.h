// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_COPY_RECT_H_INCLUDED
#define APP_CMD_COPY_RECT_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "gfx/clip.h"

#include <vector>

namespace doc {
class Image;
}

namespace app { namespace cmd {
using namespace doc;

class CopyRect : public Cmd,
                 public WithImage {
public:
  CopyRect(Image* dst, const Image* src, const gfx::Clip& clip);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_data.size(); }

private:
  void swap();
  int lineSize();

  gfx::Clip m_clip;
  std::vector<uint8_t> m_data;
};

}} // namespace app::cmd

#endif
