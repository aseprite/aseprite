// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cels_range.h"

#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace doc {

CelsRange::CelsRange(const Sprite* sprite,
  frame_t first, frame_t last, Flags flags)
  : m_begin(sprite, first, last, flags)
  , m_end()
{
}

CelsRange::iterator::iterator()
  : m_cel(nullptr)
{
}

CelsRange::iterator::iterator(const Sprite* sprite, frame_t first, frame_t last, CelsRange::Flags flags)
  : m_cel(nullptr)
  , m_first(first)
  , m_last(last)
  , m_flags(flags)
{
  // Get first cel
  Layer* layer = sprite->layer(sprite->firstLayer());
  while (layer && !m_cel) {
    if (layer->isImage()) {
      for (frame_t f=first; f<=last; ++f) {
        m_cel = layer->cel(f);
        if (m_cel)
          break;
      }
    }

    if (!m_cel)
      nextLayer(layer);
  }

  if (m_cel && flags == CelsRange::UNIQUE)
    m_visited.insert(m_cel->data()->id());
}

CelsRange::iterator& CelsRange::iterator::operator++()
{
  if (!m_cel)
    return *this;

  // Get next cel
  Layer* layer = m_cel->layer();
  frame_t first = m_cel->frame()+1;
  m_cel = nullptr;

  while (layer && !m_cel) {
    if (layer->isImage()) {
      for (frame_t f=first; f<=m_last; ++f) {
        m_cel = layer->cel(f);
        if (m_cel) {
          if (m_flags == CelsRange::UNIQUE) {
            if (m_visited.find(m_cel->data()->id()) == m_visited.end()) {
              m_visited.insert(m_cel->data()->id());
              break;
            }
            else
              m_cel = nullptr;
          }
          else
            break;
        }
      }
    }

    if (!m_cel) {
      nextLayer(layer);
      first = m_first;
    }
  }
  return *this;
}

void CelsRange::iterator::nextLayer(Layer*& layer)
{
  // Go to children
  if (layer->isGroup()) {
    layer = static_cast<LayerGroup*>(layer)->firstLayer();
  }
  // Go to next layer in the parent
  else if (!layer->getNext())
    layer = layer->parent()->getNext();
  else
    layer = layer->getNext();
}

} // namespace doc
