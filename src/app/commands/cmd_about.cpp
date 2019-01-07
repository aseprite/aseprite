// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "base/bind.h"

#include "about.xml.h"

namespace app {

using namespace ui;

class AboutCommand : public Command {
public:
  AboutCommand();

protected:
  void onExecute(Context* context) override;
};

AboutCommand::AboutCommand()
  : Command(CommandId::About(), CmdUIOnlyFlag)
{
}

void AboutCommand::onExecute(Context* context)
{
  gen::About window;
  window.title()->setText(PACKAGE " v" VERSION);
  window.licenses()->Click.connect(
    [&window]{
      window.closeWindow(nullptr);
      App::instance()->mainWindow()->showBrowser("docs/LICENSES.md");
    });
  window.openWindowInForeground();
}

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}

} // namespace app
