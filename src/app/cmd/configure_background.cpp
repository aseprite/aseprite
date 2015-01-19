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

#include "app/cmd/configure_background.h"

#include "app/cmd/move_layer.h"
#include "app/cmd/set_layer_flags.h"
#include "app/cmd/set_layer_name.h"

namespace app {
namespace cmd {

ConfigureBackground::ConfigureBackground(Layer* layer)
{
  // Add "Background" and "LockMove" flags
  LayerFlags newFlags = LayerFlags(int(layer->flags())
    | int(LayerFlags::BackgroundLayerFlags));

  add(new cmd::SetLayerFlags(layer, newFlags));
  add(new cmd::SetLayerName(layer, "Background"));
  add(new cmd::MoveLayer(layer, nullptr));
}

} // namespace cmd
} // namespace app
