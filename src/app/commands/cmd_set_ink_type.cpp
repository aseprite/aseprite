// Aseprite
// Copyright (C) 2001-2016  David Capello
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
#include "app/tools/ink_type.h"
#include "app/ui/context_bar.h"

namespace app {

class SetInkTypeCommand : public Command {
public:
  SetInkTypeCommand();
  Command* clone() const override { return new SetInkTypeCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  tools::InkType m_type;
};

SetInkTypeCommand::SetInkTypeCommand()
  : Command("SetInkType",
            "Set Ink Type",
            CmdUIOnlyFlag)
  , m_type(tools::InkType::DEFAULT)
{
}

void SetInkTypeCommand::onLoadParams(const Params& params)
{
  std::string typeStr = params.get("type");
  if (typeStr == "simple")
    m_type = tools::InkType::SIMPLE;
  else if (typeStr == "alpha-compositing")
    m_type = tools::InkType::ALPHA_COMPOSITING;
  else if (typeStr == "copy-color")
    m_type = tools::InkType::COPY_COLOR;
  else if (typeStr == "lock-alpha")
    m_type = tools::InkType::LOCK_ALPHA;
  else if (typeStr == "shading")
    m_type = tools::InkType::SHADING;
  else
    m_type = tools::InkType::DEFAULT;
}

bool SetInkTypeCommand::onChecked(Context* context)
{
  tools::Tool* tool = App::instance()->activeTool();
  return (Preferences::instance().tool(tool).ink() == m_type);
}

void SetInkTypeCommand::onExecute(Context* context)
{
  App::instance()
    ->contextBar()
    ->setInkType(m_type);
}

std::string SetInkTypeCommand::onGetFriendlyName() const
{
  return "Set Ink Type: " + tools::ink_type_to_string(m_type);
}

Command* CommandFactory::createSetInkTypeCommand()
{
  return new SetInkTypeCommand;
}

} // namespace app
