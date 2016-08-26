// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/copy_cel.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class LinkCelsCommand : public Command {
public:
  LinkCelsCommand();
  Command* clone() const override { return new LinkCelsCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

LinkCelsCommand::LinkCelsCommand()
  : Command("LinkCels",
            "Links Cels",
            CmdRecordableFlag)
{
}

bool LinkCelsCommand::onEnabled(Context* context)
{
  if (context->checkFlags(ContextFlags::ActiveDocumentIsWritable)) {
    // TODO the range of selected frames should be in doc::Site.
    auto range = App::instance()->timeline()->range();
    return (range.enabled() && range.frames() > 1);
  }
  else
    return false;
}

void LinkCelsCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  bool nonEditableLayers = false;
  {
    // TODO the range of selected frames should be in doc::Site.
    auto range = App::instance()->timeline()->range();
    if (!range.enabled())
      return;

    Transaction transaction(writer.context(), friendlyName());
    Sprite* sprite = writer.sprite();
    frame_t begin = range.frameBegin();
    frame_t end = range.frameEnd();

    for (LayerIndex layerIdx = range.layerBegin(); layerIdx <= range.layerEnd(); ++layerIdx) {
      Layer* layer = sprite->indexToLayer(layerIdx);
      if (!layer->isImage())
        continue;

      if (!layer->isEditable()) {
        nonEditableLayers = true;
        continue;
      }

      LayerImage* layerImage = static_cast<LayerImage*>(layer);
      for (frame_t frame=begin; frame < end+1; ++frame) {
        Cel* cel = layerImage->cel(frame);
        if (cel) {
          for (frame = cel->frame()+1;
               frame < end+1; ++frame) {
            transaction.execute(
              new cmd::CopyCel(
                layerImage, cel->frame(),
                layerImage, frame,
                true));         // true = force links
          }
          break;
        }
      }
    }

    transaction.commit();
  }

  if (nonEditableLayers)
    StatusBar::instance()->showTip(1000,
      "There are locked layers");

  update_screen_for_document(document);
}

Command* CommandFactory::createLinkCelsCommand()
{
  return new LinkCelsCommand;
}

} // namespace app
