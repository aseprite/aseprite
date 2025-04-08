// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/doc_api.h"

#include "app/cmd/copy_cel.h"
#include "app/cmd/set_pixel_format.h"
#include "app/cmd/set_total_frames.h"
#include "app/file/file.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "render/dithering.h"

namespace app {

using namespace docapi;

// Returns true if the document srcDoc has a cel that can be moved to the destDoc.
// The cel from srcDoc can be moved only when all of the following conditions
// are met:
// * Drop took place in a cel.
// * Source doc has only one layer with just one frame.
// * The layer from the source doc and the destination cel's layer are both
// Image layers.
// Otherwise this function returns false.
bool canMoveCel(app::Doc* srcDoc, app::Doc* destDoc, DroppedOn droppedOn, doc::layer_t layerIndex)
{
  auto* srcLayer = srcDoc->sprite()->firstLayer();
  auto* destLayer = destDoc->sprite()->allLayers()[layerIndex];
  return droppedOn == DroppedOn::Cel && srcDoc->sprite()->allLayersCount() == 1 &&
         srcDoc->sprite()->totalFrames() == 1 && srcLayer->isImage() && destLayer->isImage();
}

void setupInsertionLayer(app::Doc* destDoc,
                         doc::layer_t layerIndex,
                         InsertionPoint& insert,
                         Layer*& layer,
                         LayerGroup*& group)
{
  const LayerList& allLayers = destDoc->sprite()->allLayers();
  layer = allLayers[layerIndex];
  if (insert == InsertionPoint::BeforeLayer && layer->isGroup()) {
    group = static_cast<LayerGroup*>(layer);
    // The user is trying to drop layers into an empty group, so there is no after
    // nor before layer...
    if (group->layersCount() == 0) {
      layer = nullptr;
      return;
    }
    layer = group->lastLayer();
    insert = InsertionPoint::AfterLayer;
  }

  group = layer->parent();
}

void DocApi::dropDocumentsOnTimeline(app::Doc* destDoc,
                                     doc::frame_t frame,
                                     doc::layer_t layerIndex,
                                     InsertionPoint insert,
                                     DroppedOn droppedOn,
                                     DocProvider& docProvider)
{
  // Layer used as a reference to determine if the dropped layers will be
  // inserted after or before it.
  Layer* refLayer = nullptr;
  // Parent group of the reference layer layer.
  LayerGroup* group = nullptr;
  // Keep track of the current insertion point.
  setupInsertionLayer(destDoc, layerIndex, insert, refLayer, group);

  int docsProcessed = 0;
  while (docProvider.pendingDocs() > 0) {
    std::unique_ptr<Doc> srcDoc = docProvider.nextDoc();
    docsProcessed++;

    // If the provider returned a null document then there was some problem with
    // that specific doc but it can continue providing more documents
    if (!srcDoc)
      continue;

    // If source document doesn't match the destination document's color
    // mode, change it.
    if (srcDoc->colorMode() != destDoc->colorMode()) {
      // We can change the source doc pixel format out of any transaction because
      // we don't need undo/redo it.
      cmd::SetPixelFormat cmd(srcDoc->sprite(),
                              destDoc->sprite()->pixelFormat(),
                              render::Dithering(),
                              Preferences::instance().quantization.rgbmapAlgorithm(),
                              nullptr,
                              nullptr,
                              FitCriteria::DEFAULT);
      cmd.execute(destDoc->context());
    }

    // If there is only one source document to process and it has a cel that
    // can be moved, then copy the cel from the source doc into the
    // destination doc's selected frame.
    const bool isJustOneDoc = (docsProcessed == 1 && docProvider.pendingDocs() == 0);
    if (isJustOneDoc && canMoveCel(srcDoc.get(), destDoc, droppedOn, layerIndex)) {
      auto* srcLayer = static_cast<LayerImage*>(srcDoc->sprite()->firstLayer());
      auto* destLayer = static_cast<LayerImage*>(destDoc->sprite()->allLayers()[layerIndex]);
      m_transaction.execute(new cmd::CopyCel(srcLayer, 0, destLayer, frame, false));
      break;
    }

    // If there is no room for the source frames, add frames to the
    // destination sprite.
    if (frame + srcDoc->sprite()->totalFrames() > destDoc->sprite()->totalFrames()) {
      m_transaction.execute(
        new cmd::SetTotalFrames(destDoc->sprite(), frame + srcDoc->sprite()->totalFrames()));
    }

    for (auto* srcLayer : srcDoc->sprite()->root()->layers()) {
      if (insert == InsertionPoint::AfterLayer) {
        refLayer = duplicateLayerAfter(srcLayer, group, refLayer);
      }
      else {
        refLayer = duplicateLayerBefore(srcLayer, group, refLayer);
        insert = InsertionPoint::AfterLayer;
      }
    }
  }
}

} // namespace app
