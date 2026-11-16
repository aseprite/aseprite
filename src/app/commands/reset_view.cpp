// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"

namespace app {

struct ResetViewParams : public NewParams {
  Param<bool> preview{ this, false, "preview" };
};

class ResetViewCommand : public CommandWithNewParams<ResetViewParams> {
public:
  ResetViewCommand();

protected:
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

ResetViewCommand::ResetViewCommand() : CommandWithNewParams<ResetViewParams>(CommandId::ResetView())
{
}

void ResetViewCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  Editor* editor;
  if (params().preview())
    editor = App::instance()->mainWindow()->getPreviewEditor()->previewEditor();
  else
    editor = Editor::activeEditor();

  if (editor)
    editor->setViewOptions(ViewOptions());
}

std::string ResetViewCommand::onGetFriendlyName() const
{
  return (params().preview() ? Strings::commands_ResetView_Preview() :
                               Strings::commands_ResetView());
}

Command* CommandFactory::createResetViewCommand()
{
  return new ResetViewCommand;
}

} // namespace app
