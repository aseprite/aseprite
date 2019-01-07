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
#include "app/context.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"

namespace app {

using namespace ui;

//////////////////////////////////////////////////////////////////////

class PlayAnimationCommand : public Command {
public:
  PlayAnimationCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

PlayAnimationCommand::PlayAnimationCommand()
  : Command(CommandId::PlayAnimation(), CmdUIOnlyFlag)
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
    current_editor->play(Preferences::instance().editor.playOnce(),
                         Preferences::instance().editor.playAll());
}

//////////////////////////////////////////////////////////////////////

class PlayPreviewAnimationCommand : public Command {
public:
  PlayPreviewAnimationCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

PlayPreviewAnimationCommand::PlayPreviewAnimationCommand()
  : Command(CommandId::PlayPreviewAnimation(), CmdUIOnlyFlag)
{
}

bool PlayPreviewAnimationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void PlayPreviewAnimationCommand::onExecute(Context* context)
{
  PreviewEditorWindow* preview = App::instance()->mainWindow()->getPreviewEditor();
  if (!preview->isPreviewEnabled())
    preview->setPreviewEnabled(true);
  preview->pressPlayButton();
}

//////////////////////////////////////////////////////////////////////

Command* CommandFactory::createPlayAnimationCommand()
{
  return new PlayAnimationCommand;
}

Command* CommandFactory::createPlayPreviewAnimationCommand()
{
  return new PlayPreviewAnimationCommand;
}

} // namespace app
