// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/main_window.h"
#include "fmt/format.h"
#include "ver/info.h"

#include "about.xml.h"
#include "app/context.h"

namespace app {

using namespace ui;

class AboutCommand : public Command {
public:
  AboutCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

AboutCommand::AboutCommand() : Command(CommandId::About())
{
}

bool AboutCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

void AboutCommand::onExecute(Context* context)
{
  gen::About window;
  window.title()->setText(fmt::format("{} v{}", get_app_name(), get_app_version()));
  window.licenses()->Click.connect([&window] {
    window.closeWindow(nullptr);
    App::instance()->mainWindow()->showBrowser("docs/LICENSES.md");
  });
  window.credits()->Click.connect([&window] {
    window.closeWindow(nullptr);
    App::instance()->mainWindow()->showBrowser("AUTHORS.md", "Authors");
  });
  window.i18nCredits()->Click.connect([&window] {
    window.closeWindow(nullptr);
    App::instance()->mainWindow()->showBrowser("strings/README.md", "Translators");
  });
  window.openWindowInForeground();
}

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}

} // namespace app
