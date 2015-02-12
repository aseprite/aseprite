// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_CLEAR_CEL_H_INCLUDED
#define APP_CMD_CLEAR_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "app/cmd_sequence.h"

namespace app {
namespace cmd {
  using namespace doc;

  class ClearCel : public Cmd
                 , public WithCel {
  public:
    ClearCel(Cel* cel);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_seq.memSize();
    }

  private:
    CmdSequence m_seq;
  };

} // namespace cmd
} // namespace app

#endif
