// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/alert.h"
#include "ui/widget.h"

namespace app {

class RemoveLayerCommand : public Command {
public:
  RemoveLayerCommand();
  Command* clone() const override { return new RemoveLayerCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

RemoveLayerCommand::RemoveLayerCommand()
  : Command("RemoveLayer",
            "Remove Layer",
            CmdRecordableFlag)
{
}

bool RemoveLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsVisible |
                             ContextFlags::ActiveLayerIsEditable);
}

void RemoveLayerCommand::onExecute(Context* context)
{
  std::string layer_name;
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  Layer* layer(writer.layer());
  {
    Transaction transaction(writer.context(), "Remove Layer");
    DocumentApi api = document->getApi(transaction);

    // TODO the range of selected layer should be in doc::Site.
    auto range = App::instance()->timeline()->range();
    if (range.enabled()) {
      if (range.layers() == sprite->countLayers()) {
        ui::Alert::show("Error<<You cannot delete all layers.||&OK");
        return;
      }

      for (LayerIndex layer = range.layerEnd(); layer >= range.layerBegin(); --layer) {
        api.removeLayer(sprite->indexToLayer(layer));
      }
    }
    else {
      if (sprite->countLayers() == 1) {
        ui::Alert::show("Error<<You cannot delete the last layer.||&OK");
        return;
      }

      layer_name = layer->name();
      api.removeLayer(layer);
    }

    transaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  if (!layer_name.empty())
    StatusBar::instance()->showTip(1000, "Layer `%s' removed", layer_name.c_str());
  else
    StatusBar::instance()->showTip(1000, "Layers removed");
}

Command* CommandFactory::createRemoveLayerCommand()
{
  return new RemoveLayerCommand;
}

} // namespace app
