// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/util/msk_file.h"
#include "base/fs.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/alert.h"

namespace app {

class SaveMaskCommand : public Command {
public:
  SaveMaskCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SaveMaskCommand::SaveMaskCommand()
  : Command(CommandId::SaveMask(), CmdUIOnlyFlag)
{
}

bool SaveMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void SaveMaskCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Doc* document(reader.document());

  base::paths exts = { "msk" };
  base::paths selFilename;
  if (!app::show_file_selector(
        "Save .msk File", "default.msk", exts,
        FileSelectorType::Save, selFilename))
    return;

  std::string filename = selFilename.front();

  if (save_msk_file(document->mask(), filename.c_str()) != 0)
    ui::Alert::show(fmt::format(Strings::alerts_error_saving_file(), filename));
}

Command* CommandFactory::createSaveMaskCommand()
{
  return new SaveMaskCommand;
}

} // namespace app
