// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_PATCH_CEL_H_INCLUDED
#define APP_CMD_PATCH_CEL_H_INCLUDED
#pragma once

#include "app/cmd/with_cel.h"
#include "app/cmd_sequence.h"
#include "gfx/fwd.h"
#include "gfx/point.h"

namespace doc {
  class Cel;
  class Image;
}

namespace app {
namespace cmd {

  class PatchCel : public CmdSequence
                 , public WithCel {
  public:
    PatchCel(doc::Cel* dstCel,
             const doc::Image* patch,
             const gfx::Region& patchedRegion,
             const gfx::Point& patchPos);

  protected:
    void onExecute() override;

    const doc::Image* m_patch;
    const gfx::Region& m_region;
    gfx::Point m_pos;
  };

} // namespace cmd
} // namespace app

#endif
