// Aseprite
// Copyright (C) 2024 Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_drop_on_timeline_H_INCLUDED
#define APP_CMD_drop_on_timeline_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "app/doc_observer.h"
#include "base/paths.h"
#include "doc/frame.h"
#include "doc/layer.h"
#include "doc/layer_list.h"

#include <vector>

namespace app {
namespace cmd {

  class DropOnTimeline : public Cmd
                       , public WithDocument {
  public:
    enum class LayerInsertion {
      Before,
      After,
    };
    // Inserts the layers and frames of the documents pointed by the specified
    // paths, at the specified frame and before or after the specified layer index.
    DropOnTimeline(app::Doc* doc, doc::frame_t frame, doc::layer_t layerIndex,
                   LayerInsertion insert, const base::paths& paths);
  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_size;
    }

  private:
    void setupInsertionLayers(doc::Layer** before, doc::Layer** after, doc::LayerGroup** group);
    void notifyAddLayer(doc::Layer* layer);
    void notifyDocObservers(doc::Layer* layer);

    size_t m_size;
    base::paths m_paths;
    doc::frame_t m_frame;
    doc::layer_t m_layerIndex;
    LayerInsertion m_insert;
    // Holds the list of layers dropped into the document.
    doc::LayerList m_droppedLayers;
    // Number of frames the doc had before dropping.
    doc::frame_t m_previousTotalFrames;
  };

} // namespace cmd
} // namespace app

#endif
