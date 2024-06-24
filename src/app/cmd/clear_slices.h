// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CLEAR_SLICES_H_INCLUDED
#define APP_CMD_CLEAR_SLICES_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd_sequence.h"
#include "doc/cel.h"
#include "doc/image_ref.h"
#include "doc/layer_list.h"
#include "doc/mask.h"
#include "doc/slice.h"

#include <vector>

namespace app {
namespace cmd {
  using namespace doc;

  class ClearSlices : public Cmd {
  public:
    ClearSlices(const LayerList& layers, frame_t frame, const std::vector<SliceKey>& slicesKeys);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      size_t sliceContentsSize = 0;
      for (const auto& sc : m_slicesContents) {
        sliceContentsSize += sc.memSize();
      }
      return sizeof(*this) + m_seq.memSize() + sliceContentsSize;
    }

  private:
    struct SlicesContent {
      Cel* cel = nullptr;
      // Image having a copy of the content of each selected slice.
      ImageRef copy = nullptr;
      gfx::Point cropPos;
      color_t bgcolor;
      size_t memSize() const {
        return sizeof(*this) + (copy ? copy->getMemSize(): 0);
      }
    };

    void clear();
    void restore();

    Mask m_mask;
    CmdSequence m_seq;
    // Slices content for each selected layer's cel
    std::vector<SlicesContent> m_slicesContents;
  };

} // namespace cmd
} // namespace app

#endif
