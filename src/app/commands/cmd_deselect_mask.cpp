// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
#include "app/tx.h"
#include "doc/mask.h"
#include "doc/sprite.h"

namespace app {

class DeselectMaskCommand : public Command {
public:
  DeselectMaskCommand();

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
  Doc* document(writer.document());
  {
    Tx tx(writer.context(), "Deselect", DoesntModifyDocument);
    tx(new cmd::DeselectMask(document));
    tx.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createDeselectMaskCommand()
{
  return new DeselectMaskCommand;
}

} // namespace app
