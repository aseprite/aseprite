// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/util/msk_file.h"
#include "base/fs.h"
#include "ui/alert.h"

namespace app {

struct SaveMaskParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<std::string> filename{ this, "default.msk", "filename" };
};

class SaveMaskCommand : public CommandWithNewParams<SaveMaskParams> {
public:
  SaveMaskCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SaveMaskCommand::SaveMaskCommand() : CommandWithNewParams(CommandId::SaveMask())
{
}

bool SaveMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void SaveMaskCommand::onExecute(Context* context)
{
  const bool ui = (params().ui() && context->isUIAvailable());
  std::string filename = params().filename();

  const ContextReader reader(context);
  const Doc* document(reader.document());

  if (ui) {
    base::paths exts = { "msk" };
    base::paths selFilename;
    if (!app::show_file_selector(Strings::save_selection_title(),
                                 filename,
                                 exts,
                                 FileSelectorType::Save,
                                 selFilename))
      return;

    filename = selFilename.front();
  }

  const bool result = (save_msk_file(document->mask(), filename.c_str()) == 0);
  if (!result && ui)
    ui::Alert::show(Strings::alerts_error_saving_file(filename));
}

Command* CommandFactory::createSaveMaskCommand()
{
  return new SaveMaskCommand;
}

} // namespace app
