// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/util/msk_file.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/alert.h"

namespace app {

class SaveMaskCommand : public Command {
public:
  SaveMaskCommand();
  Command* clone() const override { return new SaveMaskCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SaveMaskCommand::SaveMaskCommand()
  : Command("SaveMask",
            "Save Mask",
            CmdUIOnlyFlag)
{
}

bool SaveMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void SaveMaskCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document(reader.document());
  std::string filename = "default.msk";

  filename = app::show_file_selector(
    "Save .msk File", filename, "msk", FileSelectorType::Save);
  if (filename.empty())
    return;

  if (save_msk_file(document->mask(), filename.c_str()) != 0)
    ui::Alert::show("Error<<Error saving .msk file<<%s||&Close", filename.c_str());
}

Command* CommandFactory::createSaveMaskCommand()
{
  return new SaveMaskCommand;
}

} // namespace app
