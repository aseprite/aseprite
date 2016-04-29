// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/document.h"
#include "app/ui/main_window.h"
#include "ui/alert.h"

namespace app {

class ExitCommand : public Command {
public:
  ExitCommand();
  Command* clone() const override { return new ExitCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

ExitCommand::ExitCommand()
  : Command("Exit",
            "Exit",
            CmdUIOnlyFlag)
{
}

void ExitCommand::onExecute(Context* context)
{
  const doc::Documents& docs = context->documents();
  bool modifiedFiles = false;

  for (doc::Documents::const_iterator it=docs.begin(), end=docs.end(); it!=end; ++it) {
    const Document* document = static_cast<Document*>(*it);
    if (document->isModified()) {
      modifiedFiles = true;
      break;
    }
  }

  if (modifiedFiles) {
    if (ui::Alert::show("Warning<<There are sprites with changes.<<Do you want to quit anyway?||&Yes||&No") != 1)
      return; // In this case the user doesn't want to close with modified files
  }

  // Close the window
  App::instance()->mainWindow()->closeWindow(NULL);
}

Command* CommandFactory::createExitCommand()
{
  return new ExitCommand;
}

} // namespace app
