// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/reselect_mask.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class ReselectMaskCommand : public Command {
public:
  ReselectMaskCommand();
  Command* clone() const override { return new ReselectMaskCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ReselectMaskCommand::ReselectMaskCommand()
  : Command(CommandId::ReselectMask(), CmdRecordableFlag)
{
}

bool ReselectMaskCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  return
     document &&                      // The document does exist
    !document->isMaskVisible() &&     // The mask is hidden
     document->mask() &&           // The mask does exist
    !document->mask()->isEmpty();  // But it is not empty
}

void ReselectMaskCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  {
    Transaction transaction(writer.context(), "Reselect", DoesntModifyDocument);
    transaction.execute(new cmd::ReselectMask(document));
    transaction.commit();
  }

  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

Command* CommandFactory::createReselectMaskCommand()
{
  return new ReselectMaskCommand;
}

} // namespace app
