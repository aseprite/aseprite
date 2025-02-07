// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/configure_background.h"

#include "app/cmd/move_layer.h"
#include "app/cmd/set_layer_flags.h"
#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

ConfigureBackground::ConfigureBackground(Layer* layer)
{
  // Add "Background" and "LockMove" flags
  LayerFlags newFlags = LayerFlags(int(layer->flags()) | int(LayerFlags::BackgroundLayerFlags));

  add(new cmd::SetLayerFlags(layer, newFlags));
  add(new cmd::SetLayerName(layer, "Background"));

  if (layer->isImage() && static_cast<LayerImage*>(layer)->opacity() < 255) {
    add(new cmd::SetLayerOpacity(static_cast<LayerImage*>(layer), 255));
  }

  add(new cmd::MoveLayer(layer, layer->sprite()->root(), nullptr));
}

}} // namespace app::cmd
