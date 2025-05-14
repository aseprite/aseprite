// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/ui_context.h"
#include "base/fs.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "duplicate_sprite.xml.h"

#include <cstdio>

namespace app {

using namespace ui;

struct DuplicateSpriteParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<std::string> filename{ this, std::string(), "filename" };
  Param<bool> flatten{ this, false, "flatten" };
};

class DuplicateSpriteCommand : public CommandWithNewParams<DuplicateSpriteParams> {
public:
  DuplicateSpriteCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DuplicateSpriteCommand::DuplicateSpriteCommand()
  : CommandWithNewParams<DuplicateSpriteParams>(CommandId::DuplicateSprite(), CmdRecordableFlag)
{
}

bool DuplicateSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void DuplicateSpriteCommand::onExecute(Context* context)
{
  const bool ui = (params().ui() && context->isUIAvailable());

  const ContextReader reader(context);
  const Doc* document = reader.document();

  const std::string fn = document->filename();
  const std::string ext = base::get_file_extension(fn);

  std::string duplicateFn = params().filename.isSet() ?
                              params().filename() :
                              Strings::general_copy_of(base::get_file_title(fn)) +
                                (!ext.empty() ? "." + ext : "");

  bool flatten = params().flatten.isSet() ? params().flatten() :
                                            get_config_bool("DuplicateSprite", "Flatten", false);

  if (ui) {
    // Load the window widget
    app::gen::DuplicateSprite window;
    window.srcName()->setText(base::get_file_name(fn));
    window.dstName()->setText(duplicateFn);
    window.flatten()->setSelected(flatten);

    // Open the window
    window.openWindowInForeground();

    if (window.closer() == window.ok()) {
      flatten = window.flatten()->isSelected();
      duplicateFn = window.dstName()->text();

      // Only set the config when we do it from the UI, to avoid automation messing with user
      // expectations.
      set_config_bool("DuplicateSprite", "Flatten", flatten);
    }
    else // Abort if we close/cancel the window
      return;
  }

  // Make a copy of the document
  Doc* docCopy;
  if (flatten)
    docCopy = document->duplicate(DuplicateWithFlattenLayers);
  else
    docCopy = document->duplicate(DuplicateExactCopy);

  docCopy->setFilename(duplicateFn);
  docCopy->setContext(context);
}

Command* CommandFactory::createDuplicateSpriteCommand()
{
  return new DuplicateSpriteCommand;
}

} // namespace app
