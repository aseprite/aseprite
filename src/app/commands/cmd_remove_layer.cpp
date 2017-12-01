// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/status_bar.h"
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
  : Command(CommandId::RemoveLayer(), CmdRecordableFlag)
{
}

bool RemoveLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer);
}

void RemoveLayerCommand::onExecute(Context* context)
{
  std::string layerName;
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Transaction transaction(writer.context(), "Remove Layer");
    DocumentApi api = document->getApi(transaction);

    const Site* site = writer.site();
    if (site->inTimeline() &&
        !site->selectedLayers().empty()) {
      SelectedLayers selLayers = site->selectedLayers();
      selLayers.removeChildrenIfParentIsSelected();

      layer_t deletedTopLevelLayers = 0;
      for (Layer* layer : selLayers) {
        if (layer->parent() == sprite->root())
          ++deletedTopLevelLayers;
      }

      if (deletedTopLevelLayers == sprite->root()->layersCount()) {
        ui::Alert::show(Strings::alerts_cannot_delete_all_layers());
        return;
      }

      for (Layer* layer : selLayers) {
        api.removeLayer(layer);
      }
    }
    else {
      if (sprite->allLayersCount() == 1) {
        ui::Alert::show(Strings::alerts_cannot_delete_all_layers());
        return;
      }

      Layer* layer = writer.layer();
      layerName = layer->name();
      api.removeLayer(layer);
    }

    transaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  if (!layerName.empty())
    StatusBar::instance()->showTip(1000, "Layer '%s' removed", layerName.c_str());
  else
    StatusBar::instance()->showTip(1000, "Layers removed");
}

Command* CommandFactory::createRemoveLayerCommand()
{
  return new RemoveLayerCommand;
}

} // namespace app
