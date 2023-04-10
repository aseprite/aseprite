// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_ZINDEX_H_INCLUDED
#define APP_CMD_SET_CEL_ZINDEX_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetCelZIndex : public Cmd
                     , public WithCel {
  public:
    SetCelZIndex(Cel* cel, int zindex);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    int m_oldZIndex;
    int m_newZIndex;
  };

} // namespace cmd
} // namespace app

#endif
