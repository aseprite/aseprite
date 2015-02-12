// Aseprite
// Copyright (C) 2001-2015  David Capello
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
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"

namespace app {

using namespace gfx;

class ShowOnionSkinCommand : public Command {
public:
  ShowOnionSkinCommand()
    : Command("ShowOnionSkin",
              "Show Onion Skin",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const override { return new ShowOnionSkinCommand(*this); }

protected:
  bool onChecked(Context* context)
  {
    IDocumentSettings* docSettings = context->settings()->getDocumentSettings(context->activeDocument());

    return docSettings->getUseOnionskin();
  }

  void onExecute(Context* context)
  {
    IDocumentSettings* docSettings = context->settings()->getDocumentSettings(context->activeDocument());

    docSettings->setUseOnionskin(docSettings->getUseOnionskin() ? false: true);
  }
};

Command* CommandFactory::createShowOnionSkinCommand()
{
  return new ShowOnionSkinCommand;
}

} // namespace app
