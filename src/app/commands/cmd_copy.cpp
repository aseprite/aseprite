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
#include "app/context_access.h"
#include "app/document.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/util/clipboard.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "ui/base.h"

namespace app {

class CopyCommand : public Command {
public:
  CopyCommand();
  Command* clone() const override { return new CopyCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CopyCommand::CopyCommand()
  : Command("Copy",
            "Copy",
            CmdUIOnlyFlag)
{
}

bool CopyCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::HasActiveDocument);
}

void CopyCommand::onExecute(Context* context)
{
  const ContextReader reader(context);

  // Copy a range from the timeline.
  DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
  if (range.enabled()) {
    clipboard::copy_range(reader, range);
  }
  else if (reader.site()->document() &&
           static_cast<const app::Document*>(reader.site()->document())->isMaskVisible() &&
           reader.site()->image()) {
    clipboard::copy(reader);
  }
}

Command* CommandFactory::createCopyCommand()
{
  return new CopyCommand;
}

} // namespace app
