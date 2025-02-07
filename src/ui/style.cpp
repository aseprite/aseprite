// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/style.h"

#include "os/font.h"

namespace ui {

// static
gfx::Border Style::UndefinedBorder()
{
  return gfx::Border(-1, -1, -1, -1);
}

// static
gfx::Size Style::MinSize()
{
  return gfx::Size(0, 0);
}

// static
gfx::Size Style::MaxSize()
{
  return gfx::Size(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
}

Style::Style(const Style* base)
  : m_insertionPoint(0)
  , m_margin(base ? base->margin() : Style::UndefinedBorder())
  , m_border(base ? base->border() : Style::UndefinedBorder())
  , m_padding(base ? base->padding() : Style::UndefinedBorder())
  , m_minSize(base ? base->minSize() : Style::MinSize())
  , m_maxSize(base ? base->maxSize() : Style::MaxSize())
  , m_gap(base ? base->gap() : gfx::Size(0, 0))
  , m_font(nullptr)
  , m_mnemonics(base ? base->mnemonics() : true)
{
  if (base)
    m_layers = base->layers();
}

void Style::setMinSize(const gfx::Size& sz)
{
  ASSERT(sz.w <= m_maxSize.w);
  ASSERT(sz.h <= m_maxSize.h);
  m_minSize = sz;
}

void Style::setMaxSize(const gfx::Size& sz)
{
  ASSERT(sz.w >= m_minSize.w);
  ASSERT(sz.h >= m_minSize.h);
  m_maxSize = sz;
}

void Style::setFont(const os::Ref<os::Font>& font)
{
  m_font = font;
}

void Style::addLayer(const Layer& layer)
{
  int i, j = int(m_layers.size());

  for (i = m_insertionPoint; i < int(m_layers.size()); ++i) {
    if (layer.type() == m_layers[i].type()) {
      for (j = i + 1; j < int(m_layers.size()); ++j) {
        if (layer.type() != m_layers[j].type())
          break;
      }
      break;
    }
  }

  if (i < int(m_layers.size())) {
    if (layer.type() == Style::Layer::Type::kNewLayer)
      m_insertionPoint = i + 1;
    else
      m_layers.insert(m_layers.begin() + j, layer);
  }
  else {
    m_layers.push_back(layer);
    if (layer.type() == Style::Layer::Type::kNewLayer)
      m_insertionPoint = int(m_layers.size());
  }
}

} // namespace ui
