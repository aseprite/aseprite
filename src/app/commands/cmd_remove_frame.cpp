// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
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
#include "app/doc_api.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

class RemoveFrameCommand : public Command {
public:
  RemoveFrameCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

RemoveFrameCommand::RemoveFrameCommand()
  : Command(CommandId::RemoveFrame(), CmdRecordableFlag)
{
}

bool RemoveFrameCommand::onEnabled(Context* context)
{
  if (!context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasActiveSprite))
    return false;

  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  return
    sprite &&
    sprite->totalFrames() > 1;
}

void RemoveFrameCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Tx tx(writer.context(), "Remove Frame");
    DocApi api = document->getApi(tx);
    const Site* site = writer.site();
    if (site->inTimeline() &&
        !site->selectedFrames().empty()) {
      for (frame_t frame : site->selectedFrames().reversed()) {
        api.removeFrame(sprite, frame);
      }
    }
    else {
      api.removeFrame(sprite, writer.frame());
    }

    tx.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createRemoveFrameCommand()
{
  return new RemoveFrameCommand;
}

} // namespace app
