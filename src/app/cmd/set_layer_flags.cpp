// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_layer_flags.h"

namespace app { namespace cmd {

SetLayerFlags::SetLayerFlags(Layer* layer, LayerFlags flags)
  : WithLayer(layer)
  , m_oldFlags(layer->flags())
  , m_newFlags(flags)
{
}

void SetLayerFlags::onExecute()
{
  layer()->setFlags(m_newFlags);
  layer()->incrementVersion();
}

void SetLayerFlags::onUndo()
{
  layer()->setFlags(m_oldFlags);
  layer()->incrementVersion();
}

}} // namespace app::cmd
