// Aseprite
// Copyright (C) 20016  Carlo "zED" Caputo
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
#include "app/pref/preferences.h"

namespace app {

using namespace gfx;

class ShowCelPreviewThumbCommand : public Command {
public:
  ShowCelPreviewThumbCommand()
    : Command("ShowCelPreviewThumb",
              "Show Cel Preview Thumbnail",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const override { return new ShowCelPreviewThumbCommand(*this); }

protected:
  bool onChecked(Context* context) override {
    DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
    return docPref.celPreview.showThumb();
  }

  void onExecute(Context* context) override {
    DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
    docPref.celPreview.showThumb(!docPref.celPreview.showThumb());
  }
};

Command* CommandFactory::createShowCelPreviewThumbCommand()
{
  return new ShowCelPreviewThumbCommand;
}

} // namespace app
