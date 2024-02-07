// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/layer_frame_comboboxes.h"

#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/restore_visible_layers.h"
#include "app/site.h"
#include "doc/anidir.h"
#include "doc/frames_sequence.h"
#include "doc/layer.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/playback.h"
#include "ui/combobox.h"

namespace app {

const char* kWholeCanvas = "";
const char* kAllLayers = "";
const char* kAllFrames = "";
const char* kSelectedCanvas = "**selected-canvas**";
const char* kSelectedLayers = "**selected-layers**";
const char* kSelectedFrames = "**selected-frames**";

SliceListItem::SliceListItem(doc::Slice* slice)
  : ListItem("Slice: " + slice->name())
  , m_slice(slice)
{
  setValue(m_slice->name());
}

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
  const std::string namePrefix =
    (isGroup ? Strings::layer_combo_group() :
               Strings::layer_combo_layer()) + " ";
  name.insert(0, namePrefix);
  return name;
}

FrameListItem::FrameListItem(doc::Tag* tag)
  : ListItem(Strings::frame_combo_tag() + " " + tag->name())
  , m_tag(tag)
{
  setValue(m_tag->name());
}

void fill_area_combobox(const doc::Sprite* sprite, ui::ComboBox* area, const std::string& defArea)
{
  int i = area->addItem("Canvas");
  dynamic_cast<ui::ListItem*>(area->getItem(i))->setValue(kWholeCanvas);

  i = area->addItem("Selection");
  dynamic_cast<ui::ListItem*>(area->getItem(i))->setValue(kSelectedCanvas);
  if (defArea == kSelectedCanvas)
    area->setSelectedItemIndex(i);

  for (auto slice : sprite->slices()) {
    if (slice->name().empty())
      continue;

    i = area->addItem(new SliceListItem(slice));
    if (defArea == slice->name())
      area->setSelectedItemIndex(i);
  }
}

void fill_layers_combobox(const doc::Sprite* sprite, ui::ComboBox* layers, const std::string& defLayer, const int defLayerIndex)
{
  int i = layers->addItem(Strings::layer_combo_visible_layers());
  dynamic_cast<ui::ListItem*>(layers->getItem(i))->setValue(kAllLayers);

  i = layers->addItem(Strings::layer_combo_selected_layers());
  dynamic_cast<ui::ListItem*>(layers->getItem(i))->setValue(kSelectedLayers);
  if (defLayer == kSelectedLayers)
    layers->setSelectedItemIndex(i);

  assert(layers->getItemCount() == kLayersComboboxExtraInitialItems);
  static_assert(kLayersComboboxExtraInitialItems == 2,
                "Update kLayersComboboxExtraInitialItems value to match the number of initial items in layers combobox");

  doc::LayerList layersList = sprite->allLayers();
  for (auto it=layersList.rbegin(), end=layersList.rend(); it!=end; ++it) {
    doc::Layer* layer = *it;
    i = layers->addItem(new LayerListItem(layer));
    if (defLayer == layer->name() && (defLayerIndex == -1 ||
                                      defLayerIndex == i-kLayersComboboxExtraInitialItems))
      layers->setSelectedItemIndex(i);
  }
}

void fill_frames_combobox(const doc::Sprite* sprite, ui::ComboBox* frames, const std::string& defFrame)
{
  int i = frames->addItem(Strings::frame_combo_all_frames());
  dynamic_cast<ui::ListItem*>(frames->getItem(i))->setValue(kAllFrames);

  i = frames->addItem(Strings::frame_combo_selected_frames());
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
    int(doc::AniDir::PING_PONG) == 2 &&
    int(doc::AniDir::PING_PONG_REVERSE) == 3, "doc::AniDir has changed");

  anidir->addItem(Strings::anidir_combo_forward());
  anidir->addItem(Strings::anidir_combo_reverse());
  anidir->addItem(Strings::anidir_combo_ping_pong());
  anidir->addItem(Strings::anidir_combo_ping_pong_reverse());
  anidir->setSelectedItemIndex(int(defAnidir));
}

void calculate_visible_layers(const Site& site,
                              const std::string& layersValue,
                              const int layersIndex,
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
  else if (layersValue != kAllLayers) {
    int i = site.sprite()->allLayersCount();
    // TODO add a getLayerByName
    for (doc::Layer* layer : site.sprite()->allLayers()) {
      i--;
      if (layer->name() == layersValue && (layersIndex == -1 ||
                                           layersIndex == i)) {
        layersVisibility.showLayer(layer);
        break;
      }
    }
  }
}

doc::Tag* calculate_frames_sequence(const Site& site,
                                    const std::string& framesValue,
                                    doc::FramesSequence& framesSeq,
                                    bool playSubtags,
                                    AniDir aniDir)
{
  doc::Tag* tag = nullptr;

  if (!playSubtags || framesValue == kSelectedFrames) {
    SelectedFrames selFrames;
    tag = calculate_selected_frames(site, framesValue, selFrames);
    framesSeq = FramesSequence(selFrames);
  }
  else {
    frame_t start = 0;
    AniDir origAniDir = aniDir;
    int forward = 1;
    if (framesValue == kAllFrames) {
      tag = nullptr;
      forward = (aniDir == AniDir::FORWARD || aniDir == AniDir::PING_PONG ? 1 : -1);
      if (forward < 0)
        start = site.sprite()->lastFrame();

      // Look for a tag containing the first frame and determine the start frame
      // according to the selected animation direction.
      auto startTag = site.sprite()->tags().innerTag(start);
      if (startTag) {
        int startTagForward = (startTag->aniDir() == AniDir::FORWARD ||
                               startTag->aniDir() == AniDir::PING_PONG
                               ? 1 : -1);
        start = forward * startTagForward > 0
                ? startTag->fromFrame()
                : startTag->toFrame();
      }
    }
    else {
      tag = site.sprite()->tags().getByName(framesValue);
      // User selected a specific tag, then set its anidir to the selected
      // direction. We save the original direction to restore it later.
      if (tag) {
        origAniDir = tag->aniDir();
        tag->setAniDir(aniDir);
        start = (aniDir == AniDir::FORWARD || aniDir == AniDir::PING_PONG
                 ? tag->fromFrame()
                 : tag->toFrame());
      }
    }

    auto playback = doc::Playback(
      site.document()->sprite(),
      site.document()->sprite()->tags().getInternalList(),
      start,
      doc::Playback::PlayAll,
      tag,
      forward);
    framesSeq.insert(playback.frame());
    auto frame = playback.nextFrame();
    while(!playback.isStopped()) {
      framesSeq.insert(frame);
      frame = playback.nextFrame();
    }

    if (framesValue == kAllFrames) {
      // If the user is playing all frames and selected some of the ping-pong
      // animation direction, then modify the animation as needed.
      if (aniDir == AniDir::PING_PONG || aniDir == AniDir::PING_PONG_REVERSE)
        framesSeq = framesSeq.makePingPong();
    }
    else if (tag) {
      // If exported tag is ping-pong, remove last frame of the sequence to
      // avoid playing the first frame twice.
      if (aniDir == AniDir::PING_PONG || aniDir == AniDir::PING_PONG_REVERSE) {
        doc::FramesSequence newSeq;
        int i = 0;
        int frames = framesSeq.size()-1;
        for (auto f : framesSeq) {
          if (i < frames)
            newSeq.insert(f);
          ++i;
        }
        framesSeq = newSeq;
      }
      // Restore tag's original animation direction.
      tag->setAniDir(origAniDir);
    }
  }

  return tag;
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
