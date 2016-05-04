// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_TRIM_CEL_H_INCLUDED
#define APP_CMD_TRIM_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/remove_cel.h"
#include "app/cmd/with_cel.h"
#include "doc/color.h"
#include "gfx/rect.h"

namespace app {
namespace cmd {

  class TrimCel : public Cmd
                , public WithCel {
  public:
    TrimCel(doc::Cel* cel);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (m_removeCel ? m_removeCel->memSize() : 0);
    }

  private:
    void cropImage(const gfx::Rect& bounds);

    gfx::Rect m_bounds;
    gfx::Rect m_originalBounds;
    doc::color_t m_color;
    RemoveCel* m_removeCel;
  };

} // namespace cmd
} // namespace app

#endif
