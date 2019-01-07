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
#include "app/commands/params.h"
#include "app/context.h"
#include "app/pref/preferences.h"
#include "app/tools/freehand_algorithm.h"
#include "app/tools/tool.h"

namespace app {

class PixelPerfectModeCommand : public Command {
public:
  PixelPerfectModeCommand();

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

PixelPerfectModeCommand::PixelPerfectModeCommand()
  : Command(CommandId::PixelPerfectMode(), CmdUIOnlyFlag)
{
}

bool PixelPerfectModeCommand::onEnabled(Context* ctx)
{
  return true;
}

bool PixelPerfectModeCommand::onChecked(Context* ctx)
{
  tools::Tool* tool = App::instance()->activeTool();
  if (!tool)
    return false;

  auto& toolPref = Preferences::instance().tool(tool);
  return (toolPref.freehandAlgorithm() == tools::FreehandAlgorithm::PIXEL_PERFECT);
}

void PixelPerfectModeCommand::onExecute(Context* ctx)
{
  tools::Tool* tool = App::instance()->activeTool();
  if (!tool)
    return;

  auto& toolPref = Preferences::instance().tool(tool);
  toolPref.freehandAlgorithm(
    toolPref.freehandAlgorithm() == tools::FreehandAlgorithm::DEFAULT ?
    tools::FreehandAlgorithm::PIXEL_PERFECT:
    tools::FreehandAlgorithm::DEFAULT);
}

Command* CommandFactory::createPixelPerfectModeCommand()
{
  return new PixelPerfectModeCommand;
}

} // namespace app
