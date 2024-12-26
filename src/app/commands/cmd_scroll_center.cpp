// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/ui/editor/editor.h"

namespace app {

class ScrollCenterCommand : public Command {
public:
  ScrollCenterCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ScrollCenterCommand::ScrollCenterCommand() : Command(CommandId::ScrollCenter(), CmdUIOnlyFlag)
{
}

bool ScrollCenterCommand::onEnabled(Context* context)
{
  return (Editor::activeEditor() != nullptr);
}

void ScrollCenterCommand::onExecute(Context* context)
{
  Editor::activeEditor()->setScrollToCenter();
}

Command* CommandFactory::createScrollCenterCommand()
{
  return new ScrollCenterCommand;
}

} // namespace app
