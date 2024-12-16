// Aseprite
// Copyright (C) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/i18n/strings.h"

namespace app {

struct ShowMenuParams : public NewParams {
  Param<std::string> menu{ this, "", "menu" };
};

class ShowMenuCommand : public CommandWithNewParams<ShowMenuParams> {
public:
  ShowMenuCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;

  MenuItem* findMenuItem() const;
  void openSubmenuById(Menu* menu, const std::string& id);
};

ShowMenuCommand::ShowMenuCommand()
  : CommandWithNewParams<ShowMenuParams>(CommandId::ShowMenu(), CmdUIOnlyFlag)
{
}

void ShowMenuCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  if (MenuItem* menuitem = findMenuItem())
    menuitem->openSubmenu();
}

std::string ShowMenuCommand::onGetFriendlyName() const
{
  std::string name;
  if (MenuItem* menuitem = findMenuItem())
    name = menuitem->text();
  else
    name = params().menu();
  return Strings::commands_ShowMenu(name);
}

MenuItem* ShowMenuCommand::findMenuItem() const
{
  std::string id = params().menu();
  if (id.empty())
    return nullptr;

  if (AppMenus* menus = AppMenus::instance()) {
    if (Menu* root = menus->getRootMenu()) {
      if (auto menuitem = root->findItemById(id.c_str())) {
        if (menuitem->type() == ui::kMenuItemWidget)
          return static_cast<MenuItem*>(menuitem);
      }
    }
  }
  return nullptr;
}

Command* CommandFactory::createShowMenuCommand()
{
  return new ShowMenuCommand;
}

} // namespace app
