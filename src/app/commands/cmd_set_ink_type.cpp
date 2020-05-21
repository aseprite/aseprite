// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
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
#include "app/commands/new_params.h"
#include "app/i18n/strings.h"
#include "app/tools/ink_type.h"
#include "app/ui/context_bar.h"
#include "fmt/format.h"

namespace app {

struct SetInkTypeParams : public NewParams {
  Param<app::tools::InkType> type { this, app::tools::InkType::DEFAULT, "type" };
};

class SetInkTypeCommand : public CommandWithNewParams<SetInkTypeParams> {
public:
  SetInkTypeCommand();

protected:
  bool onNeedsParams() const override { return true; }
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;
};

SetInkTypeCommand::SetInkTypeCommand()
  : CommandWithNewParams(CommandId::SetInkType(), CmdUIOnlyFlag)
{
}

bool SetInkTypeCommand::onChecked(Context* context)
{
  tools::Tool* tool = App::instance()->activeTool();
  return (Preferences::instance().tool(tool).ink() == params().type());
}

void SetInkTypeCommand::onExecute(Context* context)
{
  if (App::instance()->contextBar() != nullptr)
    App::instance()->contextBar()->setInkType(params().type());
}

std::string SetInkTypeCommand::onGetFriendlyName() const
{
  std::string ink;
  switch (params().type()) {
    case tools::InkType::SIMPLE:
      ink = Strings::inks_simple_ink();
      break;
    case tools::InkType::ALPHA_COMPOSITING:
      ink = Strings::inks_alpha_compositing();
      break;
    case tools::InkType::COPY_COLOR:
      ink = Strings::inks_copy_color();
      break;
    case tools::InkType::LOCK_ALPHA:
      ink = Strings::inks_lock_alpha();
      break;
    case tools::InkType::SHADING:
      ink = Strings::inks_shading();
      break;
  }
  return fmt::format(getBaseFriendlyName(), ink);
}

Command* CommandFactory::createSetInkTypeCommand()
{
  return new SetInkTypeCommand;
}

} // namespace app
