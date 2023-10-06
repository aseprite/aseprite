// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/layer_from_background.h"
#include "app/cmd/set_layer_flags.h"
#include "app/cmd/set_layer_name.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

LayerFromBackground::LayerFromBackground(Layer* layer)
{
  ASSERT(layer != NULL);
  ASSERT(layer->isVisible());
  ASSERT(layer->isEditable());
  ASSERT(layer->isBackground());
  ASSERT(!layer->isReference());
  ASSERT(layer->sprite() != NULL);
  ASSERT(layer->sprite()->backgroundLayer() != NULL);

  // Remove "Background" and "LockMove" flags
  LayerFlags newFlags = LayerFlags(int(layer->flags())
    & ~int(LayerFlags::BackgroundLayerFlags));

  layer->setBgPrevName(layer->name());

  add(new cmd::SetLayerFlags(layer, newFlags));

  // set previous set layer name before layer got changed to background
  add(new cmd::SetLayerName(layer, layer->prevName()));
}

} // namespace cmd
} // namespace app
