// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/settings/settings.h"
#include "app/tools/tool.h"
#include "app/ui/context_bar.h"
#include "app/ui/main_window.h"
#include "base/convert_to.h"
#include "doc/brush.h"

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
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  Change m_change;
  int m_slot;
};

ChangeBrushCommand::ChangeBrushCommand()
  : Command("ChangeBrush",
            "Change Brush",
            CmdUIOnlyFlag)
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
  tools::Tool* current_tool = context->settings()->getCurrentTool();
  IToolSettings* tool_settings = context->settings()->getToolSettings(current_tool);
  IBrushSettings* brush = tool_settings->getBrush();

  switch (m_change) {
    case None:
      // Do nothing
      break;
    case IncrementSize:
      if (brush->getSize() < doc::Brush::kMaxBrushSize)
        brush->setSize(brush->getSize()+1);
      break;
    case DecrementSize:
      if (brush->getSize() > doc::Brush::kMinBrushSize)
        brush->setSize(brush->getSize()-1);
      break;
    case IncrementAngle:
      if (brush->getAngle() < 180)
        brush->setAngle(brush->getAngle()+1);
      break;
    case DecrementAngle:
      if (brush->getAngle() > 0)
        brush->setAngle(brush->getAngle()-1);
      break;
    case CustomBrush:
      App::instance()->getMainWindow()->getContextBar()
        ->setActiveBrushBySlot(m_slot);
      break;
  }
}

std::string ChangeBrushCommand::onGetFriendlyName() const
{
  std::string text = "Brush";

  switch (m_change) {
    case None:
      break;
    case IncrementSize:
      text += ": Increment Size";
      break;
    case DecrementSize:
      text += ": Decrement Size";
      break;
    case IncrementAngle:
      text += ": Increment Angle";
      break;
    case DecrementAngle:
      text += ": Decrement Angle";
      break;
    case CustomBrush:
      text += ": Custom Brush #";
      text += base::convert_to<std::string>(m_slot);
      break;
  }

  return text;
}

Command* CommandFactory::createChangeBrushCommand()
{
  return new ChangeBrushCommand;
}

} // namespace app
