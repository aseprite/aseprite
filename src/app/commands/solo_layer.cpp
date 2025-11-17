// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "doc/layer.h"

#include <algorithm>

namespace app {

struct SoloLayerParams : public NewParams {
  Param<int> layerId{ this, 0, "layerId" };
};

class SoloLayerCommand : public CommandWithNewParams<SoloLayerParams> {
public:
  SoloLayerCommand();

private:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

SoloLayerCommand::SoloLayerCommand() : CommandWithNewParams<SoloLayerParams>(CommandId::SoloLayer())
{
}

bool SoloLayerCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

static void show_children_layers(const LayerGroup* layerGroup, Doc* doc)
{
  for (Layer* child : layerGroup->layers()) {
    if (child->isGroup() && static_cast<const LayerGroup*>(child)->layersCount() > 0)
      show_children_layers(static_cast<const LayerGroup*>(child), doc);
    doc->setLayerVisibilityWithNotifications(child, true);
  }
}

void SoloLayerCommand::onExecute(Context* ctx)
{
  ContextReader reader(ctx);
  ASSERT(reader.document() && reader.document()->sprite());
  if (!reader.document() || !reader.document()->sprite())
    return;

  Doc* doc = reader.document();
  const Sprite* sprite = doc->sprite();
  auto allLayers = sprite->allLayers();
  SelectedLayers selLayers;
  auto range = ctx->range();

  // Case of Alt+Click eye pick. In this case just one layer is referred by object id via
  // command parmameter.
  int layerId = params().layerId();
  if (layerId > 0) {
    for (Layer* layer : allLayers)
      if (layer->id() == layerId) {
        selLayers.insert(layer);
        break;
      }
  }
  // No parameters means 'command via keyboard shortcut' (i.e. no eye pick cases)
  else {
    if (range.enabled())
      selLayers = range.selectedLayers();
    else
      selLayers.insert(reader.layer());
  }

  // Hide everything or restore alternative state
  bool oneWithInternalState = std::any_of(
    allLayers.begin(),
    allLayers.end(),
    [](const Layer* layer) -> bool { return layer->hasFlags(LayerFlags::Internal_WasVisible); });

  // If there is one layer with the internal state, restore the previous visible state
  if (oneWithInternalState) {
    for (Layer* layer : allLayers) {
      if (layer->hasFlags(LayerFlags::Internal_WasVisible)) {
        doc->setLayerVisibilityWithNotifications(layer, true);
        layer->switchFlags(LayerFlags::Internal_WasVisible, false);
      }
      else
        doc->setLayerVisibilityWithNotifications(layer, false);
    }
    // Keep the layer clicked with Alt+click, or the selected layers visible.
    for (Layer* layer : selLayers)
      doc->setLayerVisibilityWithNotifications(layer, true);
  }
  // In other case, hide everything
  else {
    for (Layer* layer : allLayers) {
      layer->switchFlags(LayerFlags::Internal_WasVisible, layer->isVisible());
      doc->setLayerVisibilityWithNotifications(layer, false);
    }
    // Keep the selected layers visible.
    for (Layer* layer : selLayers) {
      doc->setLayerVisibilityWithNotifications(layer, true);

      // Show children layers if it's a Group
      if (layer->isGroup() && static_cast<const LayerGroup*>(layer)->layersCount() > 0)
        show_children_layers(static_cast<const LayerGroup*>(layer), doc);

      // Show parents too
      if (layer->parent() && !layer->parent()->isVisible()) {
        Layer* parentLayer = layer->parent();
        while (parentLayer) {
          if (!parentLayer->isVisible())
            doc->setLayerVisibilityWithNotifications(parentLayer, true);
          parentLayer = parentLayer->parent();
        }
      }
    }
  }
}

std::string SoloLayerCommand::onGetFriendlyName() const
{
  return Strings::commands_SoloLayer();
}

Command* CommandFactory::createSoloLayerCommand()
{
  return new SoloLayerCommand;
}

} // namespace app
