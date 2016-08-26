// Aseprite
// Copyright (C) 2001-2015  David Capello
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
  , m_layerBegin(0)
  , m_layerEnd(-1)
  , m_frameBegin(0)
  , m_frameEnd(-1)
{
}

DocumentRange::DocumentRange(Cel* cel)
  : m_type(kCels)
  , m_layerBegin(cel->sprite()->layerToIndex(cel->layer()))
  , m_layerEnd(m_layerBegin)
  , m_frameBegin(cel->frame())
  , m_frameEnd(m_frameBegin)
{
}

void DocumentRange::startRange(LayerIndex layer, frame_t frame, Type type)
{
  m_type = type;
  m_layerBegin = m_layerEnd = layer;
  m_frameBegin = m_frameEnd = frame;
}

void DocumentRange::endRange(LayerIndex layer, frame_t frame)
{
  ASSERT(enabled());
  m_layerEnd = layer;
  m_frameEnd = frame;
}

void DocumentRange::disableRange()
{
  m_type = kNone;
}

bool DocumentRange::inRange(LayerIndex layer) const
{
  if (enabled())
    return (layer >= layerBegin() && layer <= layerEnd());
  else
    return false;
}

bool DocumentRange::inRange(frame_t frame) const
{
  if (enabled())
    return (frame >= frameBegin() && frame <= frameEnd());
  else
    return false;
}

bool DocumentRange::inRange(LayerIndex layer, frame_t frame) const
{
  return inRange(layer) && inRange(frame);
}

void DocumentRange::setLayers(int layers)
{
  if (m_layerBegin <= m_layerEnd) m_layerEnd = m_layerBegin + LayerIndex(layers - 1);
  else m_layerBegin = m_layerEnd + LayerIndex(layers - 1);
}

void DocumentRange::setFrames(frame_t frames)
{
  if (m_frameBegin <= m_frameEnd)
    m_frameEnd = (m_frameBegin + frames) - 1;
  else
    m_frameBegin = (m_frameEnd + frames) - 1;
}

void DocumentRange::displace(int layerDelta, int frameDelta)
{
  m_layerBegin += LayerIndex(layerDelta);
  m_layerEnd   += LayerIndex(layerDelta);
  m_frameBegin += frame_t(frameDelta);
  m_frameEnd   += frame_t(frameDelta);
}

bool DocumentRange::convertToCels(Sprite* sprite)
{
  switch (m_type) {
    case DocumentRange::kNone:
      return false;
    case DocumentRange::kCels:
      break;
    case DocumentRange::kFrames:
      m_layerBegin = sprite->firstLayer();
      m_layerEnd = sprite->lastLayer();
      m_type = DocumentRange::kCels;
      break;
    case DocumentRange::kLayers:
      m_frameBegin = frame_t(0);
      m_frameEnd = sprite->lastFrame();
      m_type = DocumentRange::kCels;
      break;
  }
  return true;
}

} // namespace app
