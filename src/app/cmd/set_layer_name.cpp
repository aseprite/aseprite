// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_name.h"

#include "doc/layer.h"

namespace app {
namespace cmd {

SetLayerName::SetLayerName(Layer* layer, const std::string& name)
  : WithLayer(layer)
  , m_oldName(layer->name())
  , m_newName(name)
{
}

void SetLayerName::onExecute()
{
  layer()->setName(m_newName);
}

void SetLayerName::onUndo()
{
  layer()->setName(m_oldName);
}

} // namespace cmd
} // namespace app
