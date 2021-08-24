// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"

#include "register.xml.h"

namespace app {

class RegisterCommand : public Command {
public:
  RegisterCommand();
protected:
  void onExecute(Context* context) override;
};

RegisterCommand::RegisterCommand()
  : Command(CommandId::Register(), CmdUIOnlyFlag)
{
}

void RegisterCommand::onExecute(Context* context)
{
  // Load the window widget
  app::gen::Register window;

  window.setSizeable(false);

  auto onChange = [] (ui::Entry *entry) {
    return [entry]() {
      entry->setText(base::string_to_upper(entry->text()));
    };
  };

  window.code1()->Change.connect(onChange(window.code1()));
  window.code2()->Change.connect(onChange(window.code2()));
  window.code3()->Change.connect(onChange(window.code3()));

  // Open the window
  window.openWindowInForeground();
}

Command* CommandFactory::createRegisterCommand()
{
  return new RegisterCommand;
}

} // namespace app
