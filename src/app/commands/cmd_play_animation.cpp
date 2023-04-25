// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
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
  bool onEnabled(Context* ctx) override;
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

PlayAnimationCommand::PlayAnimationCommand()
  : Command(CommandId::PlayAnimation(), CmdUIOnlyFlag)
{
}

bool PlayAnimationCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsReadable |
                         ContextFlags::HasActiveSprite);
}

bool PlayAnimationCommand::onChecked(Context* ctx)
{
  auto editor = Editor::activeEditor();
  return (editor && editor->isPlaying());
}

void PlayAnimationCommand::onExecute(Context* ctx)
{
  // Do not play one-frame images
  {
    ContextReader writer(ctx);
    Sprite* sprite(writer.sprite());
    if (!sprite || sprite->totalFrames() < 2)
      return;
  }

  auto editor = Editor::activeEditor();
  ASSERT(editor);
  if (!editor)
    return;

  if (editor->isPlaying())
    editor->stop();
  else
    editor->play(Preferences::instance().editor.playOnce(),
                 Preferences::instance().editor.playAll(),
                 Preferences::instance().editor.playSubtags());
}

//////////////////////////////////////////////////////////////////////

class PlayPreviewAnimationCommand : public Command {
public:
  PlayPreviewAnimationCommand();

protected:
  bool onEnabled(Context* ctx) override;
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

PlayPreviewAnimationCommand::PlayPreviewAnimationCommand()
  : Command(CommandId::PlayPreviewAnimation(), CmdUIOnlyFlag)
{
}

bool PlayPreviewAnimationCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::HasActiveSprite);
}

bool PlayPreviewAnimationCommand::onChecked(Context* ctx)
{
  PreviewEditorWindow* preview = App::instance()->mainWindow()->getPreviewEditor();
  return (preview &&
          preview->previewEditor() &&
          preview->previewEditor()->isPlaying());
}

void PlayPreviewAnimationCommand::onExecute(Context* ctx)
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
