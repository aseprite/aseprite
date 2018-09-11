// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_SLICE_KEY_H_INCLUDED
#define APP_CMD_SET_SLICE_KEY_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_slice.h"
#include "doc/frame.h"
#include "doc/slice.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetSliceKey : public Cmd
                    , public WithSlice {
  public:
    SetSliceKey(Slice* slice,
                const doc::frame_t frame,
                const doc::SliceKey& sliceKey);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    doc::frame_t m_frame;
    doc::SliceKey m_oldSliceKey;
    doc::SliceKey m_newSliceKey;
  };

} // namespace cmd
} // namespace app

#endif
