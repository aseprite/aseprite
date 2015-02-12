// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#define APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "doc/export_data.h"

#include <string>

namespace app {

  class ExportSpriteSheetCommand : public Command {
  public:
    enum SpriteSheetType { HorizontalStrip, VerticalStrip, Matrix };
    enum ExportAction { SaveCopyAs, SaveAs, Save, DoNotSave };

    ExportSpriteSheetCommand();
    Command* clone() const override { return new ExportSpriteSheetCommand(*this); }

    SpriteSheetType type() const { return m_type; }
    ExportAction action() const { return m_action; }

    void setUseUI(bool useUI) { m_useUI = useUI; }
    void setExportData(doc::ExportDataPtr data);
    void setAction(ExportAction action) { m_action = action; }

  protected:
    virtual bool onEnabled(Context* context) override;
    virtual void onExecute(Context* context) override;

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
