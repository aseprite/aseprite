// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/pref/preferences.h"
#include "app/tools/tool.h"

namespace app {

class ContiguousFillCommand : public Command {
public:
  ContiguousFillCommand();

protected:
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

ContiguousFillCommand::ContiguousFillCommand()
  : Command(CommandId::ContiguousFill(), CmdUIOnlyFlag)
{
}

bool ContiguousFillCommand::onChecked(Context* ctx)
{
  tools::Tool* tool = App::instance()->activeTool();
  if (!tool)
    return false;

  auto& toolPref = Preferences::instance().tool(tool);
  return toolPref.contiguous();
}

void ContiguousFillCommand::onExecute(Context* ctx)
{
  tools::Tool* tool = App::instance()->activeTool();
  if (!tool)
    return;

  auto& toolPref = Preferences::instance().tool(tool);
  toolPref.contiguous(!toolPref.contiguous());
}

Command* CommandFactory::createContiguousFillCommand()
{
  return new ContiguousFillCommand;
}

} // namespace app
