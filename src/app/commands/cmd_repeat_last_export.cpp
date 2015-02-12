// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_export_sprite_sheet.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"

namespace app {

class RepeatLastExportCommand : public Command {
public:
  RepeatLastExportCommand();
  Command* clone() const override { return new RepeatLastExportCommand(*this); }

protected:
  virtual bool onEnabled(Context* context) override;
  virtual void onExecute(Context* context) override;
};

RepeatLastExportCommand::RepeatLastExportCommand()
  : Command("RepeatLastExport",
            "Repeat Last Export",
            CmdRecordableFlag)
{
}

bool RepeatLastExportCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void RepeatLastExportCommand::onExecute(Context* context)
{
  base::UniquePtr<ExportSpriteSheetCommand> command(
    static_cast<ExportSpriteSheetCommand*>(
      CommandsModule::instance()->getCommandByName(CommandId::ExportSpriteSheet)->clone()));

  {
    const ContextReader reader(context);
    const Document* document(reader.document());
    doc::ExportDataPtr data = document->exportData();

    if (data != NULL) {
      if (data->type() == doc::ExportData::None)
        return;                 // Do nothing case

      command->setUseUI(false);
      command->setExportData(data);
    }
  }

  context->executeCommand(command);
}

Command* CommandFactory::createRepeatLastExportCommand()
{
  return new RepeatLastExportCommand;
}

} // namespace app
