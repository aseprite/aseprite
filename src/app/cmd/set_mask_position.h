// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_MASK_POSITION_H_INCLUDED
#define APP_CMD_SET_MASK_POSITION_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "gfx/point.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetMaskPosition : public Cmd
                        , public WithDocument {
  public:
    SetMaskPosition(Doc* doc, const gfx::Point& pos);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void setMaskPosition(const gfx::Point& pos);

    gfx::Point m_oldPosition;
    gfx::Point m_newPosition;
  };

} // namespace cmd
} // namespace app

#endif
