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
#include "app/context_access.h"
#include "app/doc_undo.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "base/thread.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/system.h"

#ifdef ENABLE_UI
#include "app/ui/timeline/timeline.h"
#endif

namespace app {

class UndoCommand : public Command {
public:
  enum Type { Undo, Redo };

  UndoCommand(Type type);

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  Type m_type;
};

UndoCommand::UndoCommand(Type type)
  : Command((type == Undo ? CommandId::Undo():
                            CommandId::Redo()), CmdUIOnlyFlag)
  , m_type(type)
{
}

bool UndoCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  return
    document != NULL &&
    ((m_type == Undo ? document->undoHistory()->canUndo():
                       document->undoHistory()->canRedo()));
}

void UndoCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  DocUndo* undo = document->undoHistory();

#ifdef ENABLE_UI
  Sprite* sprite = document->sprite();
  SpritePosition spritePosition;
  const bool gotoModified = Preferences::instance().undo.gotoModified();
  if (gotoModified) {
    SpritePosition currentPosition(writer.site()->layer(),
                                   writer.site()->frame());

    if (m_type == Undo)
      spritePosition = undo->nextUndoSpritePosition();
    else
      spritePosition = undo->nextRedoSpritePosition();

    if (spritePosition != currentPosition) {
      Layer* selectLayer = spritePosition.layer();
      if (selectLayer)
        current_editor->setLayer(selectLayer);
      current_editor->setFrame(spritePosition.frame());

      // Draw the current layer/frame (which is not undone yet) so the
      // user can see the doUndo/doRedo effect.
      current_editor->drawSpriteClipped(
        gfx::Region(gfx::Rect(0, 0, sprite->width(), sprite->height())));

      current_editor->manager()->flipDisplay();
      base::this_thread::sleep_for(0.01);
    }
  }

  // Get the stream to deserialize the document range after executing
  // the undo/redo action. We cannot yet deserialize the document
  // range because there could be inexistent layers.
  std::istream* docRangeStream;
  if (m_type == Undo)
    docRangeStream = undo->nextUndoDocRange();
  else
    docRangeStream = undo->nextRedoDocRange();

  StatusBar* statusbar = StatusBar::instance();
  if (statusbar) {
    std::string msg;
    if (m_type == Undo)
      msg = "Undid " + undo->nextUndoLabel();
    else
      msg = "Redid " + undo->nextRedoLabel();
    if (Preferences::instance().undo.showTooltip())
      statusbar->showTip(1000, msg.c_str());
    else
      statusbar->setStatusText(0, msg.c_str());
  }
#endif // ENABLE_UI

  // Effectively undo/redo.
  if (m_type == Undo)
    undo->undo();
  else
    undo->redo();

#ifdef ENABLE_UI
  // After redo/undo, we retry to change the current SpritePosition
  // (because new frames/layers could be added, positions that we
  // weren't able to reach before the undo).
  if (gotoModified) {
    SpritePosition currentPosition(
      writer.site()->layer(),
      writer.site()->frame());

    if (spritePosition != currentPosition) {
      Layer* selectLayer = spritePosition.layer();
      if (selectLayer)
        current_editor->setLayer(selectLayer);
      current_editor->setFrame(spritePosition.frame());
    }
  }

  // Update timeline range. We've to deserialize the DocRange at
  // this point when objects (possible layers) are re-created after
  // the undo and we can deserialize them.
  if (docRangeStream) {
    Timeline* timeline = App::instance()->timeline();
    if (timeline) {
      DocRange docRange;
      if (docRange.read(*docRangeStream))
        timeline->setRange(docRange);
    }
  }
#endif  // ENABLE_UI

  document->generateMaskBoundaries();
  document->setExtraCel(ExtraCelRef(nullptr));

#ifdef ENABLE_UI
  update_screen_for_document(document);
#endif
  set_current_palette(writer.palette(), false);
}

Command* CommandFactory::createUndoCommand()
{
  return new UndoCommand(UndoCommand::Undo);
}

Command* CommandFactory::createRedoCommand()
{
  return new UndoCommand(UndoCommand::Redo);
}

} // namespace app
