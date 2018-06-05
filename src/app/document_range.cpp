// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
  , m_flags(m_type)
  , m_selectingFromLayer(nullptr)
  , m_selectingFromFrame(-1)
{
}

DocumentRange::DocumentRange(Cel* cel)
  : m_type(kCels)
  , m_flags(m_type)
  , m_selectingFromLayer(nullptr)
  , m_selectingFromFrame(-1)
{
  m_selectedLayers.insert(cel->layer());
  m_selectedFrames.insert(cel->frame());
}

void DocumentRange::clearRange()
{
  m_type = kNone;
  m_flags = kNone;
  m_selectedLayers.clear();
  m_selectedFrames.clear();
}

void DocumentRange::startRange(Layer* fromLayer, frame_t fromFrame, Type type)
{
  m_type = type;
  m_flags |= type;
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

void DocumentRange::selectLayer(Layer* layer)
{
  if (m_type == kNone)
    m_type = kLayers;
  m_flags |= kLayers;

  m_selectedLayers.insert(layer);
}

void DocumentRange::selectLayers(const SelectedLayers& selLayers)
{
  if (m_type == kNone)
    m_type = kLayers;
  m_flags |= kLayers;

  for (auto layer : selLayers)
    m_selectedLayers.insert(layer);
}

bool DocumentRange::contains(const Layer* layer) const
{
  if (enabled())
    return m_selectedLayers.contains(const_cast<Layer*>(layer));
  else
    return false;
}

bool DocumentRange::contains(const Layer* layer,
                             const frame_t frame) const
{
  switch (m_type) {
    case DocumentRange::kNone:
      return false;
    case DocumentRange::kCels:
      return contains(layer) && contains(frame);
    case DocumentRange::kFrames:
      if (contains(frame)) {
        if ((m_flags & (kCels | kLayers)) != 0)
          return contains(layer);
        else
          return true;
      }
      break;
    case DocumentRange::kLayers:
      if (contains(layer)) {
        if ((m_flags & (kCels | kFrames)) != 0)
          return contains(frame);
        else
          return true;
      }
      break;
  }
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
      if ((m_flags & (kCels | kLayers)) == 0) {
        for (auto layer : sprite->allBrowsableLayers())
          m_selectedLayers.insert(layer);
      }
      m_type = DocumentRange::kCels;
      break;
    }
    case DocumentRange::kLayers:
      if ((m_flags & (kCels | kFrames)) == 0)
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
      it = it->getNextBrowsable();
    }

    if (!goNext) {
      it = m_selectingFromLayer;
      while (it) {
        if (it == toLayer) {
          goPrev = true;
          break;
        }
        it = it->getPreviousBrowsable();
      }
    }
  }

  it = m_selectingFromLayer;
  do {
    m_selectedLayers.insert(it);
    if (it == toLayer)
      break;

    if (goNext)
      it = it->getNextBrowsable();
    else if (goPrev)
      it = it->getPreviousBrowsable();
    else
      break;
  } while (it);
}

void DocumentRange::selectFrameRange(frame_t fromFrame, frame_t toFrame)
{
  m_selectedFrames.insert(fromFrame, toFrame);
}

} // namespace app
