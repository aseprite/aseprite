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

#ifndef APP_COMMANDS_CMD_SAVE_FILE_H_INCLUDED
#define APP_COMMANDS_CMD_SAVE_FILE_H_INCLUDED
#pragma once

#include "app/commands/command.h"

#include <string>

namespace app {
  class ContextReader;

  class SaveFileBaseCommand : public Command {
  public:
    SaveFileBaseCommand(const char* shortName, const char* friendlyName, CommandFlags flags);

    std::string selectedFilename() const {
      return m_selectedFilename;
    }

  protected:
    void onLoadParams(Params* params) override;
    bool onEnabled(Context* context) override;

    void saveAsDialog(const ContextReader& reader, const char* dlgTitle, bool markAsSaved);

    static bool confirmReadonly(const std::string& filename);

    std::string m_filename;
    std::string m_selectedFilename;
  };

} // namespace app

#endif
