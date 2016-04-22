// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"

namespace app {

class TogglePreviewCommand : public Command {
public:
  TogglePreviewCommand();
  Command* clone() const override { return new TogglePreviewCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

TogglePreviewCommand::TogglePreviewCommand()
  : Command("TogglePreview",
            "Toggle Preview",
            CmdUIOnlyFlag)
{
}

bool TogglePreviewCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

bool TogglePreviewCommand::onChecked(Context* context)
{
  MainWindow* mainWin = App::instance()->mainWindow();
  if (!mainWin)
    return false;

  PreviewEditorWindow* previewWin = mainWin->getPreviewEditor();
  return (previewWin && previewWin->isVisible());
}

void TogglePreviewCommand::onExecute(Context* context)
{
  PreviewEditorWindow* previewWin =
    App::instance()->mainWindow()->getPreviewEditor();

  bool state = previewWin->isPreviewEnabled();
  previewWin->setPreviewEnabled(!state);
}

Command* CommandFactory::createTogglePreviewCommand()
{
  return new TogglePreviewCommand;
}

} // namespace app
