// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_STAMP_IN_CEL_H_INCLUDED
#define APP_CMD_STAMP_IN_CEL_H_INCLUDED
#pragma once

#include "app/cmd/with_cel.h"
#include "app/cmd_sequence.h"
#include "doc/mask.h"
#include "gfx/rect.h"

namespace doc {
  class Cel;
  class Image;
}

namespace app {
namespace cmd {

  class StampInCel : public CmdSequence
                 , public WithCel {
  public:
    // Stamps image into dstCel using the specified mask. The image is
    // positioned and scaled according to the stampBounds rectangle.
    StampInCel(doc::Cel* dstCel,
             const doc::ImageRef& image,
             const doc::MaskRef& mask,
             const gfx::Rect& stampBounds);

  protected:
    void onExecute() override;

  private:
    const doc::ImageRef m_image;
    const doc::MaskRef m_mask;
    gfx::Rect m_stampBounds;
  };

} // namespace cmd
} // namespace app

#endif
