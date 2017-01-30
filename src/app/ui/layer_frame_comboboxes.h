// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LAYER_FRAME_COMBOBOXES_H_INCLUDED
#define APP_UI_LAYER_FRAME_COMBOBOXES_H_INCLUDED
#pragma once

#include "ui/listitem.h"

#include <string>

namespace doc {
  class FrameTag;
  class Layer;
  class SelectedFrames;
  class SelectedLayers;
  class Site;
  class Sprite;
}

namespace ui {
  class ComboBox;
}

namespace app {

  class RestoreVisibleLayers;

  extern const char* kAllLayers;
  extern const char* kAllFrames;
  extern const char* kSelectedLayers;
  extern const char* kSelectedFrames;

  class LayerListItem : public ui::ListItem {
  public:
    LayerListItem(doc::Layer* layer);
    doc::Layer* layer() const { return m_layer; }
  private:
    static std::string buildName(const doc::Layer* layer);
    doc::Layer* m_layer;
  };

  class FrameListItem : public ui::ListItem {
  public:
    FrameListItem(doc::FrameTag* tag);
    doc::FrameTag* tag() const { return m_tag; }
  private:
    doc::FrameTag* m_tag;
  };

  void fill_layers_combobox(const doc::Sprite* sprite, ui::ComboBox* layers, const std::string& defLayer);
  void fill_frames_combobox(const doc::Sprite* sprite, ui::ComboBox* frames, const std::string& defFrame);

  void calculate_visible_layers(doc::Site& site,
                                const std::string& layersValue,
                                RestoreVisibleLayers& layersVisibility);

  doc::FrameTag* calculate_selected_frames(const doc::Site& site,
                                           const std::string& framesValue,
                                           doc::SelectedFrames& selFrames);

} // namespace app

#endif
