// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/drop_on_timeline.h"

#include "app/cmd/add_layer.h"
#include "app/cmd/set_pixel_format.h"
#include "app/cmd/move_cel.h"
#include "app/context_flags.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/file/file.h"
#include "app/util/open_file_job.h"
#include "app/tx.h"
#include "doc/layer_list.h"
#include "render/dithering.h"

#if ENABLE_WEBP && LAF_WINDOWS
#include "app/util/decode_webp.h"
#endif

#include <algorithm>

namespace app {
namespace cmd {

DropOnTimeline::DropOnTimeline(app::Doc* doc,
                               doc::frame_t frame,
                               doc::layer_t layerIndex,
                               InsertionPoint insert,
                               DroppedOn droppedOn,
                               const base::paths& paths) : WithDocument(doc)
                                                         , m_size(0)
                                                         , m_paths(paths)
                                                         , m_frame(frame)
                                                         , m_layerIndex(layerIndex)
                                                         , m_insert(insert)
                                                         , m_droppedOn(droppedOn)
{
  ASSERT(m_layerIndex >= 0);
  for(const auto& path : m_paths)
    m_size += path.size();
}

void DropOnTimeline::setupInsertionLayers(Layer** before, Layer** after, LayerGroup** group)
{
  const LayerList& allLayers = document()->sprite()->allLayers();
  *after  = (m_insert == InsertionPoint::AfterLayer  ? allLayers[m_layerIndex] : nullptr);
  *before = (m_insert == InsertionPoint::BeforeLayer ? allLayers[m_layerIndex] : nullptr);
  if (*before && (*before)->isGroup()) {
    *group = static_cast<LayerGroup*>(*before);
    // The user is trying to drop layers into an empty group, so there is no after
    // nor before layer...
    if ((*group)->layersCount() == 0) {
      *after = nullptr;
      *before = nullptr;
      return;
    }
    *after = static_cast<LayerGroup*>(*before)->lastLayer();
    *before = nullptr;
  }

  *group  = (*after ? (*after)->parent() : (*before)->parent());
}

void DropOnTimeline::onExecute()
{
  Doc* destDoc = document();
  Console console;
  Context* context = document()->context();

  m_previousTotalFrames = destDoc->sprite()->totalFrames();
  // Layers after/before which the dropped layers will be inserted
  Layer* afterThis = nullptr;
  Layer* beforeThis = nullptr;
  // Parent group of the after/before layers.
  LayerGroup* group = nullptr;

  int flags =
    FILE_LOAD_DATA_FILE | FILE_LOAD_AVOID_BACKGROUND_LAYER |
    FILE_LOAD_CREATE_PALETTE | FILE_LOAD_SEQUENCE_YES;

  int fopCount = 0;
  while(!m_paths.empty()) {
    std::unique_ptr<FileOp> fop(
      FileOp::createLoadDocumentOperation(context, m_paths.front(), flags));

    // Do nothing (the user cancelled or something like that)
    if (!fop)
      return;

    fopCount++;

    base::paths fopFilenames;
    fop->getFilenameList(fopFilenames);
    // Remove paths that will be loaded by the current file operation.
    for (const auto& filename : fopFilenames) {
      auto it = std::find(m_paths.begin(), m_paths.end(), filename);
      if (it != m_paths.end())
        m_paths.erase(it);
    }

    if (fop->hasError()) {
      console.printf(fop->error().c_str());
    }
    else {
      OpenFileJob task(fop.get(), true);
      task.showProgressWindow();

      // Post-load processing, it is called from the GUI because may require user intervention.
      fop->postLoad();

      // Show any error
      if (fop->hasError() && !fop->isStop())
        console.printf(fop->error().c_str());

      Doc* srcDoc = fop->document();
      if (srcDoc) {
        // If source document doesn't match the destination document's color
        // mode, change it.
        if (srcDoc->colorMode() != destDoc->colorMode()) {
          // Execute in a source doc transaction because we don't need undo/redo
          // this.
          Tx tx(srcDoc);
          tx(new cmd::SetPixelFormat(
            srcDoc->sprite(), destDoc->sprite()->pixelFormat(),
            render::Dithering(),
            Preferences::instance().quantization.rgbmapAlgorithm(),
            nullptr,
            nullptr,
            FitCriteria::DEFAULT));
          tx.commit();
        }

        // If there is only one file operation and we can move the cel from the
        // source document then move the cel into the destination frame.
        const bool isOnlyFileOperation = (fopCount == 1 && m_paths.empty());
        if (isOnlyFileOperation && canMoveCelFrom(srcDoc)) {
          auto* srcLayer = static_cast<LayerImage*>(srcDoc->sprite()->firstLayer());
          auto* destLayer = static_cast<LayerImage*>(destDoc->sprite()->allLayers()[m_layerIndex]);
          executeAndAdd(new MoveCel(srcLayer, 0, destLayer, m_frame, false));
          break;
        }

        // If there is no room for the source frames, add frames to the
        // destination sprite.
        if (m_frame+srcDoc->sprite()->totalFrames() > destDoc->sprite()->totalFrames()) {
          destDoc->sprite()->setTotalFrames(m_frame+srcDoc->sprite()->totalFrames());
        }

        setupInsertionLayers(&beforeThis, &afterThis, &group);

        // Insert layers from the source document.
        auto allLayers = srcDoc->sprite()->allLayers();
        for (auto it = allLayers.cbegin(); it != allLayers.cend(); ++it) {
          auto* layer = *it;
          // TODO: If we could "relocate" a layer from the source document to the
          // destination document we could avoid making a copy here.
          auto* layerCopy = Layer::MakeCopyWithSprite(layer, destDoc->sprite());
          destDoc->copyLayerContent(layer, destDoc, layerCopy);
          layerCopy->displaceFrames(0, m_frame);

          if (afterThis) {
            group->insertLayer(layerCopy, afterThis);
            afterThis = layerCopy;
          }
          else if (beforeThis) {
            group->insertLayerBefore(layerCopy, beforeThis);
            beforeThis = nullptr;
            afterThis = layerCopy;
          }
          else {
            group->addLayer(layerCopy);
            afterThis = layerCopy;
          }
          m_droppedLayers.push_back(layerCopy);
          m_size += layerCopy->getMemSize();
        }
        group->incrementVersion();
      }
    }
  }
  destDoc->sprite()->incrementVersion();
  destDoc->incrementVersion();

  notifyDocObservers(afterThis ? afterThis : beforeThis);
}

void DropOnTimeline::onUndo()
{
  CmdSequence::onUndo();

  if (m_droppedLayers.empty()) {
    notifyDocObservers(nullptr);
    return;
  }

  Doc* doc = document();
  frame_t currentTotalFrames = doc->sprite()->totalFrames();
  Layer* layerBefore = nullptr;
  for (auto* layer : m_droppedLayers) {
    layerBefore = layer->getPrevious();
    layer->parent()->removeLayer(layer);
  }
  doc->sprite()->setTotalFrames(m_previousTotalFrames);
  m_previousTotalFrames = currentTotalFrames;

  if (!layerBefore)
    layerBefore = doc->sprite()->firstLayer();

  notifyDocObservers(layerBefore);
}

void DropOnTimeline::onRedo()
{
  CmdSequence::onRedo();

  if (m_droppedLayers.empty()) {
    notifyDocObservers(nullptr);
    return;
  }

  Doc* doc = document();
  frame_t currentTotalFrames = doc->sprite()->totalFrames();
  doc->sprite()->setTotalFrames(m_previousTotalFrames);
  m_previousTotalFrames = currentTotalFrames;

  Layer* afterThis = nullptr;
  Layer* beforeThis = nullptr;
  LayerGroup* group = nullptr;
  setupInsertionLayers(&beforeThis, &afterThis, &group);

  for (auto it = m_droppedLayers.cbegin(); it != m_droppedLayers.cend(); ++it) {
    auto* layer = *it;

    if (afterThis) {
      group->insertLayer(layer, afterThis);
      afterThis = layer;
    }
    else if (beforeThis) {
      group->insertLayerBefore(layer, beforeThis);
      beforeThis = nullptr;
      afterThis = layer;
    }
    else {
      group->addLayer(layer);
      afterThis = layer;
    }
  }
  notifyDocObservers(afterThis ? afterThis : beforeThis);
}

bool DropOnTimeline::canMoveCelFrom(app::Doc* srcDoc)
{
  // The cel from the source doc can be moved only when the all of the following
  // conditions are met:
  // * Drop took place in a cel.
  // * Source doc has only one layer with just one frame.
  // * The layer from the source doc and the destination cel's layer are both
  // Image layers.
  auto* srcLayer = srcDoc->sprite()->firstLayer();
  auto* destLayer = document()->sprite()->allLayers()[m_layerIndex];
  return m_droppedOn == DroppedOn::Cel &&
         srcDoc->sprite()->allLayersCount() == 1 &&
         srcDoc->sprite()->totalFrames() == 1 &&
         srcLayer->isImage() &&
         destLayer->isImage();
}

void DropOnTimeline::notifyDocObservers(Layer* layer)
{
  Doc* doc = document();
  if (!doc)
    return;

  if (!layer) {
    doc->notifyGeneralUpdate();
    return;
  }

  DocEvent ev(doc);
  ev.sprite(doc->sprite());
  ev.layer(layer);
  // TODO: This is a hack, we send this notification because the timeline
  // has the code we need to execute after this command. We tried using
  // DocObserver::onAddLayer but it makes the redo crash.
  doc->notify_observers<DocEvent&>(&DocObserver::onAfterRemoveLayer, ev);
}

} // namespace cmd
} // namespace app
