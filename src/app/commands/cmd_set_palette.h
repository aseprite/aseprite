// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_CMD_SET_PALETTE_H_INCLUDED
#define APP_COMMANDS_CMD_SET_PALETTE_H_INCLUDED
#pragma once

#include "app/commands/command.h"

namespace doc {
  class Palette;
}

namespace app {

  class SetPaletteCommand : public Command {
  public:
    enum class Target { Document, App };

    SetPaletteCommand();
    Command* clone() const override { return new SetPaletteCommand(*this); }

    void setPalette(const doc::Palette* palette) { m_palette = palette; }

  protected:
    virtual void onExecute(Context* context) override;

  private:
    const doc::Palette* m_palette;
  };

} // namespace app

#endif
