// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "ver/info.h"

#ifdef ENABLE_DRM
#include "drm/license_manager.h"
#else
#include "app/i18n/strings.h"
#include "ui/alert.h"
#endif

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
#ifdef ENABLE_DRM
  // Load the window widget
  app::gen::EnterLicense window;

  window.setSizeable(false);
  window.licenseKey()->Change.connect([&window]() {
    window.licenseKey()->setText(base::string_to_upper(window.licenseKey()->text()));
    window.okButton()->setEnabled(window.licenseKey()->text().size() > 0);
  });
  window.okButton()->setEnabled(false);
  window.okButton()->Click.connect([&window](ui::Event&) {
    drm::LicenseManager::instance()->activate(window.licenseKey()->text(), get_app_name(), get_app_version());
    window.closeWindow(nullptr);
  });

  // Open the window
  window.openWindowInForeground();
#else
  ui::Alert::show(Strings::alerts_enter_license_disabled());
#endif
}

Command* CommandFactory::createRegisterCommand()
{
  return new EnterLicenseCommand;
}

} // namespace app
