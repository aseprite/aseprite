// Aseprite UI Library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/style.h"

namespace ui {

// static
gfx::Border Style::UndefinedBorder()
{
  return gfx::Border(-1, -1, -1, -1);
}

Style::Style(const Style* base)
  : m_insertionPoint(0)
  , m_margin(base ? base->margin(): Style::UndefinedBorder())
  , m_border(base ? base->border(): Style::UndefinedBorder())
  , m_padding(base ? base->padding(): Style::UndefinedBorder())
  , m_font(nullptr)
{
  if (base)
    m_layers = base->layers();
}

void Style::addLayer(const Layer& layer)
{
  int i, j = int(m_layers.size());

  for (i=m_insertionPoint; i<int(m_layers.size()); ++i) {
    if (layer.type() == m_layers[i].type()) {
      for (j=i+1; j<int(m_layers.size()); ++j) {
        if (layer.type() != m_layers[j].type())
          break;
      }
      break;
    }
  }

  if (i < int(m_layers.size())) {
    if (layer.type() == Style::Layer::Type::kNewLayer)
      m_insertionPoint = i+1;
    else
      m_layers.insert(m_layers.begin()+j, layer);
  }
  else {
    m_layers.push_back(layer);
    if (layer.type() == Style::Layer::Type::kNewLayer)
      m_insertionPoint = int(m_layers.size());
  }
}

} // namespace ui
