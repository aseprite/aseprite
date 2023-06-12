// Aseprite
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/cmd/remove_tileset.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/pref/preferences.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "fmt/format.h"
#include "ui/alert.h"
#include "ui/widget.h"

namespace app {

// If the UI is available this function shows an alert informing the user that they
// cannot remove all the layers from the sprite (this is true when topLevelLayersToDelete
// value is equals to the number of layers in the sprite's root group) and returns true.
// If the UI is not available, this function just returns true if topLevelLayersToDelete is
// equals to the number of layers in the sprite's root group.
static bool deleting_all_layers(Context* ctx, Sprite* sprite, int topLevelLayersToDelete)
{
  const bool deletingAll = (topLevelLayersToDelete == sprite->root()->layersCount());

#ifdef ENABLE_UI
  if (ctx->isUIAvailable() && deletingAll) {
    ui::Alert::show(Strings::alerts_cannot_delete_all_layers());
  }
#endif

  return deletingAll;
}

// Calculates the list of unused tileset indexes (returned in tsiToDelete parameter)
// once the layers specified are removed.
// Also, if the UI is available, shows a warning message about the deletion of unused
// tilesets.
// This function returns true in any of the following:
// - There won't be deletion of tilesets, this means tsiToDelete is empty.
// - The user accepts continuing despite the warning.
// - There is no UI available.
static bool continue_deleting_unused_tilesets(
  Context* ctx, Sprite* sprite, const LayerList layers,
  std::set<tileset_index, std::greater<tileset_index>>& tsiToDelete)
{
  std::vector<LayerTilemap*> tilemaps;
  std::map<doc::tileset_index, int> timesSelected;
  std::string layerNames;
  for (auto layer : layers) {
    if (layer->isTilemap()) {
      auto tilemap = static_cast<LayerTilemap*>(layer);
      timesSelected[tilemap->tilesetIndex()]++;
      tilemaps.push_back(tilemap);
    }
  }
  for (auto tilemap : tilemaps) {
    auto ts = sprite->tilesets()->get(tilemap->tilesetIndex());
    if (ts->tilemapsCount() == timesSelected[tilemap->tilesetIndex()]) {
      tsiToDelete.insert(tilemap->tilesetIndex());
      layerNames += tilemap->name() + ", ";
    }
  }

#ifdef ENABLE_UI
  // Just continue if UI is not available.
  if (!ctx->isUIAvailable())
    return true;

  // Remove last ", "
  if (!layerNames.empty()) {
    layerNames = layerNames.substr(0, layerNames.length() - 2);
  }

  std::string message;
  if (tsiToDelete.size() >= 1)
    message = fmt::format(Strings::alerts_deleting_tilemaps_will_delete_tilesets(), layerNames);

  return tsiToDelete.empty() ||
         app::OptionalAlert::show(
          Preferences::instance().tilemap.showDeleteUnusedTilesetAlert, 1, message) == 1;
#else
  return true;
#endif
}

class RemoveLayerCommand : public Command {
public:
  RemoveLayerCommand();

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
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    Tx tx(writer.context(), "Remove Layer");
    DocApi api = document->getApi(tx);
    // We need to remove all the tilesets after the tilemaps are deleted
    // and in descending tileset index order, otherwise the tileset indexes
    // get mixed up. This is the reason we use a tileset_index set with
    // the std::greater Compare.
    std::set<tileset_index, std::greater<tileset_index>> tsiToDelete;

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

      if (deleting_all_layers(context, sprite, deletedTopLevelLayers)) {
        return;
      }

      if (!continue_deleting_unused_tilesets(context, sprite, selLayers.toAllTilemaps(), tsiToDelete)) {
        return;
      }

      for (Layer* layer : selLayers) {
        api.removeLayer(layer);
      }
    }
    else {
      if (deleting_all_layers(context, sprite, 1)) {
        return;
      }

      Layer* layer = writer.layer();
      if (layer->isTilemap() && !continue_deleting_unused_tilesets(context, sprite, {layer}, tsiToDelete)) {
        return;
      }

      layerName = layer->name();
      api.removeLayer(layer);
    }

    if (!tsiToDelete.empty()) {
      for (tileset_index tsi : tsiToDelete) {
        tx(new cmd::RemoveTileset(sprite, tsi));
      }
    }

    tx.commit();
  }

#ifdef ENABLE_UI
  if (context->isUIAvailable()) {
    update_screen_for_document(document);

    StatusBar::instance()->invalidate();
    if (!layerName.empty())
      StatusBar::instance()->showTip(
        1000, fmt::format(Strings::remove_layer_x_removed(), layerName));
    else
      StatusBar::instance()->showTip(1000,
                                     Strings::remove_layer_layers_removed());
  }
#endif
}

Command* CommandFactory::createRemoveLayerCommand()
{
  return new RemoveLayerCommand;
}

} // namespace app
