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

namespace doc {
  class Cel;
}

namespace app {
namespace cmd {

  class TrimCel : public Cmd {
  public:
    TrimCel(doc::Cel* cel);
    ~TrimCel();

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_subCmd->memSize();
    }

  private:
    Cmd* m_subCmd;
  };

} // namespace cmd
} // namespace app

#endif
