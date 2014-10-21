/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

    void setPalette(doc::Palette* palette) { m_palette = palette; }
    void setTarget(Target target) { m_target = target; }

  protected:
    virtual void onExecute(Context* context) override;

  private:
    doc::Palette* m_palette;
    Target m_target;
  };

} // namespace app

#endif
