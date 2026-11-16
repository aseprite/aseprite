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

struct FlipViewParams : public NewParams {
  Param<std::string> orientation{ this, "horizontal", "orientation" };
  Param<bool> preview{ this, false, "preview" };
};

class FlipViewCommand : public CommandWithNewParams<FlipViewParams> {
public:
  FlipViewCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

FlipViewCommand::FlipViewCommand() : CommandWithNewParams<FlipViewParams>(CommandId::FlipView())
{
}

void FlipViewCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  Editor* editor;
  if (params().preview())
    editor = App::instance()->mainWindow()->getPreviewEditor()->previewEditor();
  else
    editor = Editor::activeEditor();

  if (editor) {
    auto opts = editor->viewOptions();
    if (params().orientation() == "vertical")
      opts.yflip = !opts.yflip;
    else
      opts.xflip = !opts.xflip;
    editor->setViewOptions(opts);
  }
}

std::string FlipViewCommand::onGetFriendlyName() const
{
  std::string orientation;

  if (params().orientation() == "vertical")
    orientation = Strings::commands_FlipView_Vertically();
  else
    orientation = Strings::commands_FlipView_Horizontally();

  return (params().preview() ? Strings::commands_FlipView_Preview(orientation) :
                               Strings::commands_FlipView(orientation));
}

Command* CommandFactory::createFlipViewCommand()
{
  return new FlipViewCommand;
}

} // namespace app
