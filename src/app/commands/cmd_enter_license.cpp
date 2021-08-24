// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"

#include "enter_license.xml.h"

namespace app {

class EnterLicenseCommand : public Command {
public:
  EnterLicenseCommand();
protected:
  void onExecute(Context* context) override;
};

EnterLicenseCommand::EnterLicenseCommand()
  : Command(CommandId::EnterLicense(), CmdUIOnlyFlag)
{
}

void EnterLicenseCommand::onExecute(Context* context)
{
  // Load the window widget
  app::gen::EnterLicense window;

  window.setSizeable(false);
  window.licenseKey()->Change.connect([&window]() {
    window.licenseKey()->setText(base::string_to_upper(window.licenseKey()->text()));
  });

  // Open the window
  window.openWindowInForeground();
}

Command* CommandFactory::createRegisterCommand()
{
  return new EnterLicenseCommand;
}

} // namespace app
