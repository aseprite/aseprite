// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/layer_frame_comboboxes.h"

#include "app/restore_visible_layers.h"
#include "app/site.h"
#include "doc/anidir.h"
#include "doc/layer.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "ui/combobox.h"

namespace app {

const char* kAllLayers = "";
const char* kAllFrames = "";
const char* kSelectedLayers = "**selected-layers**";
const char* kSelectedFrames = "**selected-frames**";

LayerListItem::LayerListItem(doc::Layer* layer)
  : ListItem(buildName(layer))
  , m_layer(layer)
{
  setValue(layer->name());
}

// static
std::string LayerListItem::buildName(const doc::Layer* layer)
{
  bool isGroup = layer->isGroup();
  std::string name;
  while (layer != layer->sprite()->root()) {
    if (!name.empty())
      name.insert(0, " > ");
    name.insert(0, layer->name());
    layer = layer->parent();
  }
  name.insert(0, isGroup ? "Group: ": "Layer: ");
  return name;
}

FrameListItem::FrameListItem(doc::Tag* tag)
  : ListItem("Tag: " + tag->name())
  , m_tag(tag)
{
  setValue(m_tag->name());
}

void fill_layers_combobox(const doc::Sprite* sprite, ui::ComboBox* layers, const std::string& defLayer)
{
  int i = layers->addItem("Visible layers");
  dynamic_cast<ui::ListItem*>(layers->getItem(i))->setValue(kAllLayers);

  i = layers->addItem("Selected layers");
  dynamic_cast<ui::ListItem*>(layers->getItem(i))->setValue(kSelectedLayers);
  if (defLayer == kSelectedLayers)
    layers->setSelectedItemIndex(i);

  doc::LayerList layersList = sprite->allLayers();
  for (auto it=layersList.rbegin(), end=layersList.rend(); it!=end; ++it) {
    doc::Layer* layer = *it;
    i = layers->addItem(new LayerListItem(layer));
    if (defLayer == layer->name())
      layers->setSelectedItemIndex(i);
  }
}

void fill_frames_combobox(const doc::Sprite* sprite, ui::ComboBox* frames, const std::string& defFrame)
{
  int i = frames->addItem("All frames");
  dynamic_cast<ui::ListItem*>(frames->getItem(i))->setValue(kAllFrames);

  i = frames->addItem("Selected frames");
  dynamic_cast<ui::ListItem*>(frames->getItem(i))->setValue(kSelectedFrames);
  if (defFrame == kSelectedFrames)
    frames->setSelectedItemIndex(i);

  for (auto tag : sprite->tags()) {
    // Don't allow to select empty frame tags
    if (tag->name().empty())
      continue;

    i = frames->addItem(new FrameListItem(tag));
    if (defFrame == tag->name())
      frames->setSelectedItemIndex(i);
  }
}

void fill_anidir_combobox(ui::ComboBox* anidir, doc::AniDir defAnidir)
{
  static_assert(
    int(doc::AniDir::FORWARD) == 0 &&
    int(doc::AniDir::REVERSE) == 1 &&
    int(doc::AniDir::PING_PONG) == 2, "doc::AniDir has changed");

  anidir->addItem("Forward");
  anidir->addItem("Reverse");
  anidir->addItem("Ping-pong");
  anidir->setSelectedItemIndex(int(defAnidir));
}

void calculate_visible_layers(const Site& site,
                              const std::string& layersValue,
                              RestoreVisibleLayers& layersVisibility)
{
  if (layersValue == kSelectedLayers) {
    if (!site.selectedLayers().empty()) {
      layersVisibility.showSelectedLayers(
        const_cast<Sprite*>(site.sprite()),
        site.selectedLayers());
    }
    else {
      layersVisibility.showLayer(const_cast<Layer*>(site.layer()));
    }
  }
  else if (layersValue != kAllFrames) {
    // TODO add a getLayerByName
    for (doc::Layer* layer : site.sprite()->allLayers()) {
      if (layer->name() == layersValue) {
        layersVisibility.showLayer(layer);
        break;
      }
    }
  }
}

doc::Tag* calculate_selected_frames(const Site& site,
                                    const std::string& framesValue,
                                    doc::SelectedFrames& selFrames)
{
  doc::Tag* tag = nullptr;

  if (framesValue == kSelectedFrames) {
    if (!site.selectedFrames().empty()) {
      selFrames = site.selectedFrames();
    }
    else {
      selFrames.insert(site.frame(), site.frame());
    }
  }
  else if (framesValue != kAllFrames) {
    tag = site.sprite()->tags().getByName(framesValue);
    if (tag)
      selFrames.insert(tag->fromFrame(),
                       tag->toFrame());
    else
      selFrames.insert(0, site.sprite()->lastFrame());
  }
  else
    selFrames.insert(0, site.sprite()->lastFrame());

  return tag;
}

} // namespace app
