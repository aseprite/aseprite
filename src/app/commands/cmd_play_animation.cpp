// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"

namespace app {

using namespace ui;

class PlayAnimationCommand : public Command {
public:
  PlayAnimationCommand();
  Command* clone() const override { return new PlayAnimationCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

PlayAnimationCommand::PlayAnimationCommand()
  : Command("PlayAnimation",
            "Play Animation",
            CmdUIOnlyFlag)
{
}

bool PlayAnimationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void PlayAnimationCommand::onExecute(Context* context)
{
  // Do not play one-frame images
  {
    ContextReader writer(context);
    Sprite* sprite(writer.sprite());
    if (!sprite || sprite->totalFrames() < 2)
      return;
  }

  ASSERT(current_editor);
  if (!current_editor)
    return;

  if (current_editor->isPlaying())
    current_editor->stop();
  else
    current_editor->play(Preferences::instance().editor.playOnce());
}

Command* CommandFactory::createPlayAnimationCommand()
{
  return new PlayAnimationCommand;
}

} // namespace app
