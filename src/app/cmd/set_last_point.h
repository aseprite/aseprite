// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_LAST_POINT_H_INCLUDED
#define APP_CMD_SET_LAST_POINT_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "gfx/point.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetLastPoint : public Cmd
                        , public WithDocument {
  public:
    SetLastPoint(Doc* doc, const gfx::Point& pos);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void setLastPoint(const gfx::Point& pos);

    gfx::Point m_oldPoint;
    gfx::Point m_newPoint;
  };

} // namespace cmd
} // namespace app

#endif
