// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CLEAR_IMAGE_H_INCLUDED
#define APP_CMD_CLEAR_IMAGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "doc/color.h"
#include "doc/cel.h"
#include "doc/image_ref.h"

namespace app {
namespace cmd {
  using namespace doc;

  class ClearImage : public Cmd
                   , public WithCel {
  public:
    ClearImage(Cel* cel, color_t color);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + (m_copy ? m_copy->getMemSize(): 0);
    }

  private:
    ImageRef m_copy;
    color_t m_color;
    frame_t m_frame_copy;
  };

} // namespace cmd
} // namespace app

#endif
