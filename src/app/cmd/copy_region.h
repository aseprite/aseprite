// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_COPY_REGION_H_INCLUDED
#define APP_CMD_COPY_REGION_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "base/buffer.h"
#include "gfx/point.h"
#include "gfx/region.h"

namespace app {
namespace cmd {
  using namespace doc;

  class CopyRegion : public Cmd
                   , public WithImage {
  public:
    // If alreadyCopied is false, it means that onExecute() will copy
    // pixels from src to dst. If it's true, it means that "onExecute"
    // should do nothing, because modified pixels are alreadt on "dst"
    // (so we use "src" as the original image).
    CopyRegion(Image* dst, const Image* src,
               const gfx::Region& region,
               const gfx::Point& dstPos,
               bool alreadyCopied = false);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_buffer.size();
    }

  private:
    void swap();

    bool m_alreadyCopied;
    gfx::Region m_region;
    base::buffer m_buffer;
  };

} // namespace cmd
} // namespace app

#endif
