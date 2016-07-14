// Aseprite
// Copyright (C) 2016  Carlo Caputo
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

class ToggleTimelineThumbnailsCommand : public Command {
public:
  ToggleTimelineThumbnailsCommand()
    : Command("ToggleTimelineThumbnails",
              "Toggle Timeline Thumbnails",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const override { return new ToggleTimelineThumbnailsCommand(*this); }

protected:
  bool onChecked(Context* context) override {
    DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
    return docPref.thumbnails.enabled();
  }

  void onExecute(Context* context) override {
    DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
    docPref.thumbnails.enabled(!docPref.thumbnails.enabled());
  }
};

Command* CommandFactory::createToggleTimelineThumbnailsCommand()
{
  return new ToggleTimelineThumbnailsCommand;
}

} // namespace app
