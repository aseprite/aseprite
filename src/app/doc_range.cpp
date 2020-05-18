// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc_range.h"

#include "app/util/layer_utils.h"
#include "base/serialization.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <iostream>

namespace app {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

DocRange::DocRange()
  : m_type(kNone)
  , m_flags(m_type)
  , m_selectingFromLayer(nullptr)
  , m_selectingFromFrame(-1)
{
}

DocRange::DocRange(Cel* cel)
  : m_type(kCels)
  , m_flags(m_type)
  , m_selectingFromLayer(nullptr)
  , m_selectingFromFrame(-1)
{
  m_selectedLayers.insert(cel->layer());
  m_selectedFrames.insert(cel->frame());
}

void DocRange::clearRange()
{
  m_type = kNone;
  m_flags = kNone;
  m_selectedLayers.clear();
  m_selectedFrames.clear();
}

void DocRange::startRange(Layer* fromLayer, frame_t fromFrame, Type type)
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

void DocRange::endRange(Layer* toLayer, frame_t toFrame)
{
  ASSERT(enabled());

  if (m_selectingFromLayer && toLayer)
    selectLayerRange(m_selectingFromLayer, toLayer);

  if (m_selectingFromFrame >= 0)
    selectFrameRange(m_selectingFromFrame, toFrame);
}

void DocRange::selectLayer(Layer* layer)
{
  if (m_type == kNone)
    m_type = kLayers;
  m_flags |= kLayers;

  m_selectedLayers.insert(layer);
}

void DocRange::selectLayers(const SelectedLayers& selLayers)
{
  if (m_type == kNone)
    m_type = kLayers;
  m_flags |= kLayers;

  for (auto layer : selLayers)
    m_selectedLayers.insert(layer);
}

void DocRange::eraseAndAdjust(const Layer* layer)
{
  if (!enabled())
    return;

  if (m_selectingFromLayer)
    m_selectingFromLayer = candidate_if_layer_is_deleted(m_selectingFromLayer, layer);

  SelectedLayers copy = m_selectedLayers;
  m_selectedLayers.clear();
  for (Layer* selectedLayer : copy) {
    Layer* layerToSelect = candidate_if_layer_is_deleted(selectedLayer, layer);
    if (layerToSelect)
      m_selectedLayers.insert(layerToSelect);
  }
}

bool DocRange::contains(const Layer* layer) const
{
  if (enabled())
    return m_selectedLayers.contains(const_cast<Layer*>(layer));
  else
    return false;
}

bool DocRange::contains(const Layer* layer,
                        const frame_t frame) const
{
  switch (m_type) {
    case DocRange::kNone:
      return false;
    case DocRange::kCels:
      return contains(layer) && contains(frame);
    case DocRange::kFrames:
      if (contains(frame)) {
        if ((m_flags & (kCels | kLayers)) != 0)
          return contains(layer);
        else
          return true;
      }
      break;
    case DocRange::kLayers:
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

void DocRange::displace(layer_t layerDelta, frame_t frameDelta)
{
  m_selectedLayers.displace(layerDelta);
  m_selectedFrames.displace(frameDelta);
}

bool DocRange::convertToCels(const Sprite* sprite)
{
  switch (m_type) {
    case DocRange::kNone:
      return false;
    case DocRange::kCels:
      break;
    case DocRange::kFrames: {
      if ((m_flags & (kCels | kLayers)) == 0) {
        for (auto layer : sprite->allBrowsableLayers())
          m_selectedLayers.insert(layer);
      }
      m_type = DocRange::kCels;
      break;
    }
    case DocRange::kLayers:
      if ((m_flags & (kCels | kFrames)) == 0)
        selectFrameRange(0, sprite->lastFrame());
      m_type = DocRange::kCels;
      break;
  }
  return true;
}

bool DocRange::write(std::ostream& os) const
{
  write32(os, m_type);
  write32(os, m_flags);

  if (!m_selectedLayers.write(os)) return false;
  if (!m_selectedFrames.write(os)) return false;

  write32(os, m_selectingFromLayer ? m_selectingFromLayer->id(): 0);
  write32(os, m_selectingFromFrame);
  return os.good();
}

bool DocRange::read(std::istream& is)
{
  clearRange();

  m_type = (Type)read32(is);
  m_flags = read32(is);

  if (!m_selectedLayers.read(is)) return false;
  if (!m_selectedFrames.read(is)) return false;

  ObjectId id = read32(is);
  m_selectingFromLayer = doc::get<Layer>(id);
  m_selectingFromFrame = read32(is);
  return is.good();
}

void DocRange::selectLayerRange(Layer* fromLayer, Layer* toLayer)
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

void DocRange::selectFrameRange(frame_t fromFrame, frame_t toFrame)
{
  m_selectedFrames.insert(fromFrame, toFrame);
}

void DocRange::setType(const Type type)
{
  if (type != kNone) {
    m_type = type;
    m_flags |= type;
  }
  else  {
    m_type = kNone;
    m_flags = kNone;
  }
}

void DocRange::setSelectedLayers(const SelectedLayers& layers)
{
  if (layers.empty()) {
    m_type = kNone;
    m_selectedLayers.clear();
    return;
  }

  m_type = kLayers;
  m_flags |= kLayers;
  m_selectedLayers = layers;
}

void DocRange::setSelectedFrames(const SelectedFrames& frames)
{
  if (frames.empty()) {
    m_type = kNone;
    m_selectedFrames.clear();
    return;
  }

  m_type = kFrames;
  m_flags |= kFrames;
  m_selectedFrames = frames;
}

} // namespace app
