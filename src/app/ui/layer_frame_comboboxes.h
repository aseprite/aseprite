// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LAYER_FRAME_COMBOBOXES_H_INCLUDED
#define APP_UI_LAYER_FRAME_COMBOBOXES_H_INCLUDED
#pragma once

#include "doc/anidir.h"
#include "ui/listitem.h"

#include <string>

namespace doc {
  class Layer;
  class SelectedFrames;
  class SelectedLayers;
  class Sprite;
  class Tag;
}

namespace ui {
  class ComboBox;
}

namespace app {
  class RestoreVisibleLayers;
  class Site;

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
    FrameListItem(doc::Tag* tag);
    doc::Tag* tag() const { return m_tag; }
  private:
    doc::Tag* m_tag;
  };

  void fill_layers_combobox(const doc::Sprite* sprite, ui::ComboBox* layers, const std::string& defLayer);
  void fill_frames_combobox(const doc::Sprite* sprite, ui::ComboBox* frames, const std::string& defFrame);
  void fill_anidir_combobox(ui::ComboBox* anidir, doc::AniDir defAnidir);

  void calculate_visible_layers(const Site& site,
                                const std::string& layersValue,
                                RestoreVisibleLayers& layersVisibility);

  doc::Tag* calculate_selected_frames(const Site& site,
                                      const std::string& framesValue,
                                      doc::SelectedFrames& selFrames);

} // namespace app

#endif
