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

#ifndef APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#define APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "base/override.h"
#include "doc/export_data.h"

#include <string>

namespace app {

  class ExportSpriteSheetCommand : public Command {
  public:
    enum SpriteSheetType { HorizontalStrip, VerticalStrip, Matrix };
    enum ExportAction { SaveCopyAs, SaveAs, Save, DoNotSave };

    ExportSpriteSheetCommand();
    Command* clone() const OVERRIDE { return new ExportSpriteSheetCommand(*this); }

    SpriteSheetType type() const { return m_type; }
    ExportAction action() const { return m_action; }

    void setUseUI(bool useUI) { m_useUI = useUI; }
    void setExportData(doc::ExportDataPtr data);
    void setAction(ExportAction action) { m_action = action; }

  protected:
    virtual bool onEnabled(Context* context) OVERRIDE;
    virtual void onExecute(Context* context) OVERRIDE;

  private:
    bool m_useUI;
    SpriteSheetType m_type;
    ExportAction m_action;
    int m_columns;
    int m_width;
    int m_height;
    bool m_bestFit;
    std::string m_filename;
  };

} // namespace app

#endif
