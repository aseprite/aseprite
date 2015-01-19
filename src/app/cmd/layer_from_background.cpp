/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
  ASSERT(layer->sprite() != NULL);
  ASSERT(layer->sprite()->backgroundLayer() != NULL);

  // Remove "Background" and "LockMove" flags
  LayerFlags newFlags = LayerFlags(int(layer->flags())
    & ~int(LayerFlags::BackgroundLayerFlags));

  add(new cmd::SetLayerFlags(layer, newFlags));
  add(new cmd::SetLayerName(layer, "Layer 0"));
}

} // namespace cmd
} // namespace app
