// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_SLICE_H_INCLUDED
#define APP_CMD_ADD_SLICE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_slice.h"
#include "app/cmd/with_sprite.h"

#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class AddSlice : public Cmd
                 , public WithSprite
                 , public WithSlice {
  public:
    AddSlice(Sprite* sprite, Slice* slice);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_size;
    }

  private:
    void addSlice(Sprite* sprite, Slice* slice);
    void removeSlice(Sprite* sprite, Slice* slice);

    size_t m_size;
    std::stringstream m_stream;
  };

} // namespace cmd
} // namespace app

#endif
