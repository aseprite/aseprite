// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "app/pref/preferences.h"

namespace app {

class MaskAllCommand : public Command {
public:
  MaskAllCommand();
  Command* clone() const override { return new MaskAllCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

MaskAllCommand::MaskAllCommand()
  : Command(CommandId::MaskAll(), CmdRecordableFlag)
{
}

bool MaskAllCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void MaskAllCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());

  Mask newMask;
  newMask.replace(sprite->bounds());

  Transaction transaction(writer.context(), "Select All", DoesntModifyDocument);
  transaction.execute(new cmd::SetMask(document, &newMask));
  transaction.commit();

  document->resetTransformation();
  document->generateMaskBoundaries();

  if (Preferences::instance().selection.autoShowSelectionEdges()) {
    DocumentPreferences& docPref = Preferences::instance().document(document);
    docPref.show.selectionEdges(true);
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createMaskAllCommand()
{
  return new MaskAllCommand;
}

} // namespace app
