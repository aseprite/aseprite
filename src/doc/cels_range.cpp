// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
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

CelsRange::CelsRange(const Sprite* sprite, const SelectedFrames& selFrames, const Flags flags)
  : m_selFrames(selFrames)
  , m_begin(sprite, m_selFrames, flags)
  , m_end(m_selFrames)
{
}

CelsRange::iterator::iterator(const SelectedFrames& selFrames)
  : m_cel(nullptr)
  , m_selFrames(selFrames)
  , m_frameIterator(selFrames.begin())
{
}

CelsRange::iterator::iterator(const Sprite* sprite,
                              const SelectedFrames& selFrames,
                              const CelsRange::Flags flags)
  : m_cel(nullptr)
  , m_selFrames(selFrames)
  , m_frameIterator(selFrames.begin())
  , m_flags(flags)
{
  // Get first cel
  Layer* layer = sprite->root()->firstLayerInWholeHierarchy();
  while (layer && !m_cel) {
    if (layer->isImage()) {
      m_frameIterator = m_selFrames.begin();
      auto endFrame = m_selFrames.end();
      for (; m_frameIterator != endFrame; ++m_frameIterator) {
        m_cel = layer->cel(*m_frameIterator);
        if (m_cel)
          break;
      }
    }

    if (!m_cel)
      layer = layer->getNextInWholeHierarchy();
  }

  if (m_cel && flags == CelsRange::UNIQUE)
    m_visited.insert(m_cel->data()->id());
}

CelsRange::iterator& CelsRange::iterator::operator++()
{
  if (!m_cel)
    return *this;

  auto endFrame = m_selFrames.end();
  if (m_frameIterator != endFrame)
    ++m_frameIterator;

  Layer* layer = m_cel->layer();
  m_cel = nullptr;
  while (layer && !m_cel) {
    if (layer->isImage()) {
      for (; m_frameIterator != endFrame; ++m_frameIterator) {
        m_cel = layer->cel(*m_frameIterator);
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
      layer = layer->getNextInWholeHierarchy();
      m_frameIterator = m_selFrames.begin();
    }
  }
  return *this;
}

} // namespace doc
