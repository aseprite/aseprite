// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
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
  class FramesSequence;
  class SelectedLayers;
  class Slice;
  class Sprite;
  class Tag;
}

namespace ui {
  class ComboBox;
}

namespace app {
  class RestoreVisibleLayers;
  class Site;

  extern const char* kWholeCanvas;
  extern const char* kAllLayers;
  extern const char* kAllFrames;
  extern const char* kSelectedCanvas;
  extern const char* kSelectedLayers;
  extern const char* kSelectedFrames;

  class SliceListItem : public ui::ListItem {
  public:
    SliceListItem(doc::Slice* slice);
    doc::Slice* slice() const { return m_slice; }
  private:
    doc::Slice* m_slice;
  };

  constexpr const int kLayersComboboxExtraInitialItems = 2;

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

  void fill_area_combobox(const doc::Sprite* sprite, ui::ComboBox* area, const std::string& defArea);
  void fill_layers_combobox(const doc::Sprite* sprite, ui::ComboBox* layers, const std::string& defLayer, const int defLayerIndex);
  void fill_frames_combobox(const doc::Sprite* sprite, ui::ComboBox* frames, const std::string& defFrame);
  void fill_anidir_combobox(ui::ComboBox* anidir, doc::AniDir defAnidir);

  void calculate_visible_layers(const Site& site,
                                const std::string& layersValue,
                                const int layersIndex,
                                RestoreVisibleLayers& layersVisibility);

  doc::Tag* calculate_frames_sequence(const Site& site,
                                      const std::string& framesValue,
                                      doc::FramesSequence& selFrames,
                                      bool playSubtags,
                                      doc::AniDir aniDir);
  doc::Tag* calculate_selected_frames(const Site& site,
                                      const std::string& framesValue,
                                      doc::SelectedFrames& selFrames);

} // namespace app

#endif
