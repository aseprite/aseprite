// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/layer_utils.h"

#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "fmt/format.h"

namespace app {

using namespace doc;

Layer* candidate_if_layer_is_deleted(
  const Layer* selectedLayer,
  const Layer* layerToDelete)
{
  const Layer* layerToSelect = selectedLayer;

  if ((selectedLayer == layerToDelete) ||
      (selectedLayer &&
       selectedLayer->hasAncestor(layerToDelete))) {
    Sprite* sprite = selectedLayer->sprite();
    LayerGroup* parent = layerToDelete->parent();

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layerToDelete->getPrevious())
      layerToSelect = layerToDelete->getPrevious();
    else if (layerToDelete->getNext())
      layerToSelect = layerToDelete->getNext();
    else if (parent != sprite->root())
      layerToSelect = parent;
  }

  return const_cast<Layer*>(layerToSelect);
}

bool layer_is_locked(Editor* editor)
{
  Layer* layer = editor->layer();
  if (!layer)
    return false;

  auto* statusBar = StatusBar::instance();

  if (!layer->isVisibleHierarchy()) {
    if (statusBar) {
      statusBar->showTip(
        1000, Strings::statusbar_tips_layer_x_is_hidden(layer->name()));
    }
    return true;
  }

  if (!layer->isEditableHierarchy()) {
    if (statusBar) {
      statusBar->showTip(
        1000, Strings::statusbar_tips_layer_locked(layer->name()));
    }
    return true;
  }

  return false;
}

std::string get_layer_path(const Layer* layer)
{
  std::string path;
  for (; layer != layer->sprite()->root(); layer=layer->parent()) {
    if (!path.empty())
      path.insert(0, "/");
    path.insert(0, layer->name());
  }
  return path;
}

Layer* copy_layer(doc::Layer* layer)
{
  return copy_layer_with_sprite(layer, layer->sprite());
}

Layer* copy_layer_with_sprite(doc::Layer* layer, doc::Sprite* sprite)
{
  std::unique_ptr<doc::Layer> clone;
  if (layer->isTilemap()) {
    auto* srcTilemap = static_cast<LayerTilemap*>(layer);
    tileset_index tilesetIndex = srcTilemap->tilesetIndex();
    // If the caller is trying to make a copy of a tilemap layer specifying a
    // different sprite as its owner, then we must copy the tilesets of the
    // given tilemap layer into the new owner.
    if (sprite != srcTilemap->sprite()) {
      auto* srcTilesetCopy = Tileset::MakeCopyCopyingImages(srcTilemap->tileset());
      tilesetIndex = sprite->tilesets()->add(srcTilesetCopy);
    }

    clone.reset(new LayerTilemap(sprite, tilesetIndex));
  }
  else if (layer->isImage())
    clone.reset(new LayerImage(sprite));
  else if (layer->isGroup())
    clone.reset(new LayerGroup(sprite));
  else
    throw std::runtime_error("Invalid layer type");

  if (auto* doc = dynamic_cast<app::Doc*>(sprite->document())) {
    doc->copyLayerContent(layer, doc, clone.get());
  }

  return clone.release();
}

} // namespace app
