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
#include "app/cmd/move_cel.h"
#include "app/cmd/set_pixel_format.h"
#include "app/console.h"
#include "app/context_flags.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/file/file.h"
#include "app/tx.h"
#include "app/util/layer_utils.h"
#include "app/util/open_file_job.h"
#include "base/serialization.h"
#include "doc/layer_io.h"
#include "doc/layer_list.h"
#include "doc/subobjects_io.h"
#include "render/dithering.h"

#include <algorithm>

namespace app { namespace cmd {

using namespace base::serialization::little_endian;

DropOnTimeline::DropOnTimeline(app::Doc* doc,
                               doc::frame_t frame,
                               doc::layer_t layerIndex,
                               InsertionPoint insert,
                               DroppedOn droppedOn,
                               const base::paths& paths)
  : WithDocument(doc)
  , m_size(0)
  , m_paths(paths)
  , m_frame(frame)
  , m_layerIndex(layerIndex)
  , m_insert(insert)
  , m_droppedOn(droppedOn)
{
  ASSERT(m_layerIndex >= 0);
  for (const auto& path : m_paths)
    m_size += path.size();

  // Zero layers stored.
  write32(m_stream, 0);
  m_size += sizeof(uint32_t);
}

DropOnTimeline::DropOnTimeline(app::Doc* doc,
                               doc::frame_t frame,
                               doc::layer_t layerIndex,
                               InsertionPoint insert,
                               DroppedOn droppedOn,
                               const doc::ImageRef& image)
  : WithDocument(doc)
  , m_size(0)
  , m_image(image)
  , m_frame(frame)
  , m_layerIndex(layerIndex)
  , m_insert(insert)
  , m_droppedOn(droppedOn)
{
  ASSERT(m_layerIndex >= 0);

  // Zero layers stored.
  write32(m_stream, 0);
  m_size += sizeof(uint32_t);
}

void DropOnTimeline::onExecute()
{
  Doc* destDoc = document();
  m_previousTotalFrames = destDoc->sprite()->totalFrames();

  int docsProcessed = 0;
  while (hasPendingWork()) {
    Doc* srcDoc;
    if (!getNextDoc(srcDoc))
      return;

    if (srcDoc) {
      docsProcessed++;
      // If source document doesn't match the destination document's color
      // mode, change it.
      if (srcDoc->colorMode() != destDoc->colorMode()) {
        // Execute in a source doc transaction because we don't need undo/redo
        // this.
        Tx tx(srcDoc);
        tx(new cmd::SetPixelFormat(srcDoc->sprite(),
                                   destDoc->sprite()->pixelFormat(),
                                   render::Dithering(),
                                   Preferences::instance().quantization.rgbmapAlgorithm(),
                                   nullptr,
                                   nullptr,
                                   FitCriteria::DEFAULT));
        tx.commit();
      }

      // If there is only one source document to process and it has a cel that
      // can be moved, then move the cel from the source doc into the
      // destination doc's selected frame.
      const bool isJustOneDoc = (docsProcessed == 1 && !hasPendingWork());
      if (isJustOneDoc && canMoveCelFrom(srcDoc)) {
        auto* srcLayer = static_cast<LayerImage*>(srcDoc->sprite()->firstLayer());
        auto* destLayer = static_cast<LayerImage*>(destDoc->sprite()->allLayers()[m_layerIndex]);
        executeAndAdd(new MoveCel(srcLayer, 0, destLayer, m_frame, false));
        break;
      }

      // If there is no room for the source frames, add frames to the
      // destination sprite.
      if (m_frame + srcDoc->sprite()->totalFrames() > destDoc->sprite()->totalFrames()) {
        destDoc->sprite()->setTotalFrames(m_frame + srcDoc->sprite()->totalFrames());
      }

      // Save dropped layers from source document.
      saveDroppedLayers(srcDoc->sprite()->root()->layers(), destDoc->sprite());

      // Source doc is not needed anymore.
      delete srcDoc;
    }
  }

  if (m_droppedLayersIds.empty())
    return;

  destDoc->sprite()->incrementVersion();
  destDoc->incrementVersion();

  insertDroppedLayers();
}

void DropOnTimeline::onUndo()
{
  CmdSequence::onUndo();

  if (m_droppedLayersIds.empty()) {
    notifyGeneralUpdate();
    return;
  }

  Doc* doc = document();
  frame_t currentTotalFrames = doc->sprite()->totalFrames();

  for (auto id : m_droppedLayersIds) {
    auto* layer = doc::get<Layer>(id);
    ASSERT(layer);
    if (layer) {
      DocEvent ev(doc);
      ev.sprite(layer->sprite());
      ev.layer(layer);
      doc->notify_observers<DocEvent&>(&DocObserver::onBeforeRemoveLayer, ev);

      LayerGroup* group = layer->parent();
      group->removeLayer(layer);
      group->incrementVersion();
      group->sprite()->incrementVersion();

      doc->notify_observers<DocEvent&>(&DocObserver::onAfterRemoveLayer, ev);

      delete layer;
    }
  }
  doc->sprite()->setTotalFrames(m_previousTotalFrames);
  doc->sprite()->incrementVersion();
  m_previousTotalFrames = currentTotalFrames;
}

void DropOnTimeline::onRedo()
{
  CmdSequence::onRedo();

  if (m_droppedLayersIds.empty()) {
    notifyGeneralUpdate();
    return;
  }

  Doc* doc = document();
  frame_t currentTotalFrames = doc->sprite()->totalFrames();
  doc->sprite()->setTotalFrames(m_previousTotalFrames);
  doc->sprite()->incrementVersion();
  m_previousTotalFrames = currentTotalFrames;

  insertDroppedLayers();
}

void DropOnTimeline::setupInsertionLayer(Layer*& layer, LayerGroup*& group)
{
  const LayerList& allLayers = document()->sprite()->allLayers();
  layer = allLayers[m_layerIndex];
  if (m_insert == InsertionPoint::BeforeLayer && layer->isGroup()) {
    group = static_cast<LayerGroup*>(layer);
    // The user is trying to drop layers into an empty group, so there is no after
    // nor before layer...
    if (group->layersCount() == 0) {
      layer = nullptr;
      return;
    }
    layer = group->lastLayer();
    m_insert = InsertionPoint::AfterLayer;
  }

  group = layer->parent();
}

bool DropOnTimeline::hasPendingWork()
{
  return m_image || !m_paths.empty();
}

bool DropOnTimeline::getNextDocFromImage(Doc*& srcDoc)
{
  if (!m_image)
    return true;

  Sprite* sprite = new Sprite(m_image->spec(), 256);
  LayerImage* layer = new LayerImage(sprite);
  sprite->root()->addLayer(layer);
  Cel* cel = new Cel(0, m_image);
  layer->addCel(cel);
  srcDoc = new Doc(sprite);
  m_image = nullptr;
  return true;
}

bool DropOnTimeline::getNextDocFromPaths(Doc*& srcDoc)
{
  Console console;
  Context* context = document()->context();
  int flags = FILE_LOAD_DATA_FILE | FILE_LOAD_AVOID_BACKGROUND_LAYER | FILE_LOAD_CREATE_PALETTE |
              FILE_LOAD_SEQUENCE_YES;

  std::unique_ptr<FileOp> fop(FileOp::createLoadDocumentOperation(context, m_paths.front(), flags));
  // Remove the path that is currently being processed
  m_paths.erase(m_paths.begin());

  // Do nothing (the user cancelled or something like that)
  if (!fop)
    return false;

  if (fop->hasError()) {
    console.printf(fop->error().c_str());
    return true;
  }

  base::paths fopFilenames;
  fop->getFilenameList(fopFilenames);
  // Remove paths that will be loaded by the current file operation.
  for (const auto& filename : fopFilenames) {
    auto it = std::find(m_paths.begin(), m_paths.end(), filename);
    if (it != m_paths.end())
      m_paths.erase(it);
  }

  OpenFileJob task(fop.get(), true);
  task.showProgressWindow();

  // Post-load processing, it is called from the GUI because may require user intervention.
  fop->postLoad();

  // Show any error
  if (fop->hasError() && !fop->isStop())
    console.printf(fop->error().c_str());

  srcDoc = fop->releaseDocument();
  return true;
}

bool DropOnTimeline::getNextDoc(Doc*& srcDoc)
{
  srcDoc = nullptr;
  if (m_image == nullptr && !m_paths.empty())
    return getNextDocFromPaths(srcDoc);

  return getNextDocFromImage(srcDoc);
}

void DropOnTimeline::storeDroppedLayerIds(const Layer* layer)
{
  if (layer->isGroup()) {
    const auto* group = static_cast<const LayerGroup*>(layer);
    for (auto* child : group->layers())
      storeDroppedLayerIds(child);

    m_droppedLayersIds.push_back(group->id());
  }
  else {
    m_droppedLayersIds.push_back(layer->id());
  }
}

void DropOnTimeline::saveDroppedLayers(const LayerList& layers, Sprite* sprite)
{
  size_t start = m_stream.tellp();

  // Calculate the new number of layers.
  m_stream.seekg(0);
  uint32_t nLayers = read32(m_stream) + layers.size();

  // Flat list of all the dropped layers.
  LayerList allDroppedLayers;
  // Write number of layers (at the beginning of the stream).
  m_stream.seekp(0);
  write32(m_stream, nLayers);
  // Move to where we must start writing.
  m_stream.seekp(start);
  for (auto it = layers.cbegin(); it != layers.cend(); ++it) {
    auto* layer = *it;
    // TODO: If we could "relocate" a layer from the source document to the
    // destination document we could avoid making a copy here.
    std::unique_ptr<Layer> layerCopy(copy_layer_with_sprite(layer, sprite));
    layerCopy->displaceFrames(0, m_frame);

    write_layer(m_stream, layerCopy.get());

    storeDroppedLayerIds(layerCopy.get());
  }
  size_t end = m_stream.tellp();
  m_size += end - start;
}

void DropOnTimeline::insertDroppedLayers()
{
  // Layer used as a reference to determine if the dropped layers will be
  // inserted after or before it.
  Layer* refLayer = nullptr;
  // Parent group of the reference layer layer.
  LayerGroup* group = nullptr;
  // Keep track of the current insertion point.
  InsertionPoint insert = m_insert;

  setupInsertionLayer(refLayer, group);

  SubObjectsFromSprite io(group->sprite());
  m_stream.seekg(0);
  auto nLayers = read32(m_stream);
  for (int i = 0; i < nLayers; ++i) {
    auto* layer = read_layer(m_stream, &io);

    if (!refLayer) {
      group->addLayer(layer);
      refLayer = layer;
      insert = InsertionPoint::AfterLayer;
    }
    else if (insert == InsertionPoint::AfterLayer) {
      group->insertLayer(layer, refLayer);
      refLayer = layer;
    }
    else if (insert == InsertionPoint::BeforeLayer) {
      group->insertLayerBefore(layer, refLayer);
      refLayer = layer;
      insert = InsertionPoint::AfterLayer;
    }

    group->incrementVersion();
    group->sprite()->incrementVersion();

    Doc* doc = static_cast<Doc*>(group->sprite()->document());
    DocEvent ev(doc);
    ev.sprite(group->sprite());
    ev.layer(layer);
    doc->notify_observers<DocEvent&>(&DocObserver::onAddLayer, ev);
  }
}

// Returns true if the document srcDoc has a cel that can be moved.
// The cel from the srcDoc can be moved only when all of the following
// conditions are met:
// * Drop took place in a cel.
// * Source doc has only one layer with just one frame.
// * The layer from the source doc and the destination cel's layer are both
// Image layers.
// Otherwise this function returns false.
bool DropOnTimeline::canMoveCelFrom(app::Doc* srcDoc)
{
  auto* srcLayer = srcDoc->sprite()->firstLayer();
  auto* destLayer = document()->sprite()->allLayers()[m_layerIndex];
  return m_droppedOn == DroppedOn::Cel && srcDoc->sprite()->allLayersCount() == 1 &&
         srcDoc->sprite()->totalFrames() == 1 && srcLayer->isImage() && destLayer->isImage();
}

void DropOnTimeline::notifyGeneralUpdate()
{
  Doc* doc = document();
  if (!doc)
    return;

  doc->notifyGeneralUpdate();
}

}} // namespace app::cmd
