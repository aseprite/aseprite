// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool.h"
#include "app/ui/context_bar.h"
#include "base/convert_to.h"
#include "doc/brush.h"
#include "fmt/format.h"

namespace app {

class ChangeBrushCommand : public Command {
  enum Change {
    None,
    IncrementSize,
    DecrementSize,
    IncrementAngle,
    DecrementAngle,
    CustomBrush,
  };

public:
  ChangeBrushCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Change m_change;
  int m_slot;
};

ChangeBrushCommand::ChangeBrushCommand()
  : Command(CommandId::ChangeBrush(), CmdUIOnlyFlag)
{
  m_change = None;
  m_slot = 0;
}

void ChangeBrushCommand::onLoadParams(const Params& params)
{
  std::string change = params.get("change");
  if (change == "increment-size") m_change = IncrementSize;
  else if (change == "decrement-size") m_change = DecrementSize;
  else if (change == "increment-angle") m_change = IncrementAngle;
  else if (change == "decrement-angle") m_change = DecrementAngle;
  else if (change == "custom") m_change = CustomBrush;

  if (m_change == CustomBrush)
    m_slot = params.get_as<int>("slot");
  else
    m_slot = 0;
}

void ChangeBrushCommand::onExecute(Context* context)
{
  // Change the brush of the selected tool in the toolbar (not the
  // active tool which might be different, e.g. the quick tool)
  tools::Tool* tool = App::instance()->activeToolManager()->activeTool();
  ToolPreferences::Brush& brush =
    Preferences::instance().tool(tool).brush;

  switch (m_change) {
    case None:
      // Do nothing
      break;
    case IncrementSize:
      if (brush.size() < doc::Brush::kMaxBrushSize)
        brush.size(brush.size()+1);
      break;
    case DecrementSize:
      if (brush.size() > doc::Brush::kMinBrushSize)
        brush.size(brush.size()-1);
      break;
    case IncrementAngle:
      if (brush.angle() < 180)
        brush.angle(brush.angle()+1);
      break;
    case DecrementAngle:
      if (brush.angle() > 0)
        brush.angle(brush.angle()-1);
      break;
    case CustomBrush:
      App::instance()->contextBar()
        ->setActiveBrushBySlot(tool, m_slot);
      break;
  }
}

std::string ChangeBrushCommand::onGetFriendlyName() const
{
  std::string change;
  switch (m_change) {
    case None: break;
    case IncrementSize: change = Strings::commands_ChangeBrush_IncrementSize(); break;
    case DecrementSize: change = Strings::commands_ChangeBrush_DecrementSize(); break;
    case IncrementAngle: change = Strings::commands_ChangeBrush_IncrementAngle(); break;
    case DecrementAngle: change = Strings::commands_ChangeBrush_DecrementAngle(); break;
    case CustomBrush:
      change = fmt::format(Strings::commands_ChangeBrush_CustomBrush(), m_slot);
      break;
  }
  return fmt::format(getBaseFriendlyName(), change);
}

Command* CommandFactory::createChangeBrushCommand()
{
  return new ChangeBrushCommand;
}

} // namespace app
