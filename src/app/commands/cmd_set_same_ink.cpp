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
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/pref/preferences.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"

namespace app {

class SetSameInkCommand : public Command {
public:
  SetSameInkCommand();

protected:
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

SetSameInkCommand::SetSameInkCommand()
  : Command(CommandId::SetSameInk(), CmdUIOnlyFlag)
{
}

bool SetSameInkCommand::onChecked(Context* context)
{
  return Preferences::instance().shared.shareInk();
}

void SetSameInkCommand::onExecute(Context* context)
{
  Preferences& pref = Preferences::instance();
  bool newState = !pref.shared.shareInk();
  pref.shared.shareInk(newState);

  if (newState) {
    tools::Tool* activeTool = App::instance()->activeTool();
    tools::InkType inkType = pref.tool(activeTool).ink();
    int opacity = pref.tool(activeTool).opacity();

    for (tools::Tool* tool : *App::instance()->toolBox()) {
      if (tool != activeTool) {
        pref.tool(tool).ink(inkType);
        pref.tool(tool).opacity(opacity);
      }
    }
  }
}

Command* CommandFactory::createSetSameInkCommand()
{
  return new SetSameInkCommand;
}

} // namespace app
