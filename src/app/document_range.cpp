// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_range.h"

#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

using namespace doc;

DocumentRange::DocumentRange()
  : m_type(kNone)
  , m_selectingFromLayer(nullptr)
  , m_selectingFromFrame(-1)
{
}

DocumentRange::DocumentRange(Cel* cel)
  : m_type(kCels)
  , m_selectingFromLayer(nullptr)
  , m_selectingFromFrame(-1)
{
  m_selectedLayers.insert(cel->layer());
  m_selectedFrames.insert(cel->frame());
}

void DocumentRange::clearRange()
{
  m_type = kNone;
  m_selectedLayers.clear();
  m_selectedFrames.clear();
}

void DocumentRange::startRange(Layer* fromLayer, frame_t fromFrame, Type type)
{
  m_type = type;
  m_selectingFromLayer = fromLayer;
  m_selectingFromFrame = fromFrame;

  if (fromLayer)
    m_selectedLayers.insert(fromLayer);
  if (fromFrame >= 0)
    m_selectedFrames.insert(fromFrame);
}

void DocumentRange::endRange(Layer* toLayer, frame_t toFrame)
{
  ASSERT(enabled());

  if (m_selectingFromLayer && toLayer)
    selectLayerRange(m_selectingFromLayer, toLayer);

  if (m_selectingFromFrame >= 0)
    selectFrameRange(m_selectingFromFrame, toFrame);
}

bool DocumentRange::contains(Layer* layer) const
{
  if (enabled())
    return m_selectedLayers.contains(layer);
  else
    return false;
}

void DocumentRange::displace(layer_t layerDelta, frame_t frameDelta)
{
  m_selectedLayers.displace(layerDelta);
  m_selectedFrames.displace(frameDelta);
}

bool DocumentRange::convertToCels(const Sprite* sprite)
{
  switch (m_type) {
    case DocumentRange::kNone:
      return false;
    case DocumentRange::kCels:
      break;
    case DocumentRange::kFrames: {
      LayerList layers = sprite->allBrowsableLayers();
      ASSERT(layers.empty());
      if (!layers.empty()) {
        selectLayerRange(layers.front(), layers.back());
        m_type = DocumentRange::kCels;
      }
      else
        return false;
      break;
    }
    case DocumentRange::kLayers:
      selectFrameRange(0, sprite->lastFrame());
      m_type = DocumentRange::kCels;
      break;
  }
  return true;
}

void DocumentRange::selectLayerRange(Layer* fromLayer, Layer* toLayer)
{
  ASSERT(fromLayer);
  ASSERT(toLayer);

  bool goNext = false;
  bool goPrev = false;
  Layer* it;

  if (fromLayer != toLayer) {
    it = m_selectingFromLayer;
    while (it) {
      if (it == toLayer) {
        goNext = true;
        break;
      }
      it = it->getNextInWholeHierarchy();
    }

    if (!goNext) {
      it = m_selectingFromLayer;
      while (it) {
        if (it == toLayer) {
          goPrev = true;
          break;
        }
        it = it->getPreviousInWholeHierarchy();
      }
    }
  }

  it = m_selectingFromLayer;
  do {
    m_selectedLayers.insert(it);
    if (it == toLayer)
      break;

    if (goNext)
      it = it->getNextInWholeHierarchy();
    else if (goPrev)
      it = it->getPreviousInWholeHierarchy();
    else
      break;
  } while (it);
}

void DocumentRange::selectFrameRange(frame_t fromFrame, frame_t toFrame)
{
  m_selectedFrames.insert(fromFrame, toFrame);
}

} // namespace app
