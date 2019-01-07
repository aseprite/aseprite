// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/pref/preferences.h"

namespace app {

using namespace gfx;

class ShowOnionSkinCommand : public Command {
public:
  ShowOnionSkinCommand()
    : Command(CommandId::ShowOnionSkin(), CmdUIOnlyFlag)
  {
  }

protected:
  bool onChecked(Context* context) override {
    DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
    return docPref.onionskin.active();
  }

  void onExecute(Context* context) override {
    DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
    docPref.onionskin.active(!docPref.onionskin.active());
  }
};

Command* CommandFactory::createShowOnionSkinCommand()
{
  return new ShowOnionSkinCommand;
}

} // namespace app
