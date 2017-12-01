// Aseprite
// Copyright (C) 2001-42017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/deselect_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class DeselectMaskCommand : public Command {
public:
  DeselectMaskCommand();
  Command* clone() const override { return new DeselectMaskCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DeselectMaskCommand::DeselectMaskCommand()
  : Command(CommandId::DeselectMask(), CmdRecordableFlag)
{
}

bool DeselectMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasVisibleMask);
}

void DeselectMaskCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  {
    Transaction transaction(writer.context(), "Deselect", DoesntModifyDocument);
    transaction.execute(new cmd::DeselectMask(document));
    transaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

Command* CommandFactory::createDeselectMaskCommand()
{
  return new DeselectMaskCommand;
}

} // namespace app
