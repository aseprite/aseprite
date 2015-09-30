// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/unlink_cel.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class UnlinkCelCommand : public Command {
public:
  UnlinkCelCommand();
  Command* clone() const override { return new UnlinkCelCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

UnlinkCelCommand::UnlinkCelCommand()
  : Command("UnlinkCel",
            "Unlink Cel",
            CmdRecordableFlag)
{
}

bool UnlinkCelCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void UnlinkCelCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  bool nonEditableLayers = false;
  {
    Transaction transaction(writer.context(), "Unlink Cel");

    // TODO the range of selected frames should be in doc::Site.
    Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
    if (range.enabled()) {
      Sprite* sprite = writer.sprite();

      for (LayerIndex layerIdx = range.layerBegin(); layerIdx <= range.layerEnd(); ++layerIdx) {
        Layer* layer = sprite->indexToLayer(layerIdx);
        if (!layer->isImage())
          continue;

        LayerImage* layerImage = static_cast<LayerImage*>(layer);

        for (frame_t frame = range.frameEnd(),
               begin = range.frameBegin()-1;
             frame != begin;
             --frame) {
          Cel* cel = layerImage->cel(frame);
          if (cel && cel->links()) {
            if (layerImage->isEditable())
              transaction.execute(new cmd::UnlinkCel(cel));
            else
              nonEditableLayers = true;
          }
        }
      }
    }
    else {
      Cel* cel = writer.cel();
      if (cel && cel->links()) {
        if (cel->layer()->isEditable())
          transaction.execute(new cmd::UnlinkCel(writer.cel()));
        else
          nonEditableLayers = true;
      }
    }

    transaction.commit();
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
