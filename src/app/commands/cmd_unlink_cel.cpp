// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/unlink_cel.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class UnlinkCelCommand : public Command {
public:
  UnlinkCelCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

UnlinkCelCommand::UnlinkCelCommand()
  : Command(CommandId::UnlinkCel(), CmdRecordableFlag)
{
}

bool UnlinkCelCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void UnlinkCelCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  bool nonEditableLayers = false;
  {
    Tx tx(writer.context(), "Unlink Cel");

    const Site* site = writer.site();
    if (site->inTimeline() &&
        !site->selectedLayers().empty()) {
      for (Layer* layer : site->selectedLayers()) {
        if (!layer->isImage())
          continue;

        if (!layer->isEditableHierarchy()) {
          nonEditableLayers = true;
          continue;
        }

        LayerImage* layerImage = static_cast<LayerImage*>(layer);

        for (frame_t frame : site->selectedFrames().reversed()) {
          Cel* cel = layerImage->cel(frame);
          if (cel && cel->links())
            tx(new cmd::UnlinkCel(cel));
        }
      }
    }
    else {
      Cel* cel = writer.cel();
      if (cel && cel->links()) {
        if (cel->layer()->isEditableHierarchy())
          tx(new cmd::UnlinkCel(writer.cel()));
        else
          nonEditableLayers = true;
      }
    }

    tx.commit();
  }

  if (nonEditableLayers)
    StatusBar::instance()->showTip(1000,
      "There are locked layers");

  update_screen_for_document(document);
}

Command* CommandFactory::createUnlinkCelCommand()
{
  return new UnlinkCelCommand;
}

} // namespace app
