// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd.h"

#include "app/cmd/add_cel.h"
#include "app/cmd/add_frame.h"
#include "app/cmd/add_layer.h"
#include "app/cmd/add_palette.h"
#include "app/cmd/add_slice.h"
#include "app/cmd/add_tag.h"
#include "app/cmd/add_tile.h"
#include "app/cmd/add_tileset.h"
#include "app/cmd/assign_color_profile.h"
#include "app/cmd/background_from_layer.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/clear_image.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/clear_rect.h"
#include "app/cmd/clear_slices.h"
#include "app/cmd/configure_background.h"
#include "app/cmd/convert_color_profile.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/copy_frame.h"
#include "app/cmd/copy_rect.h"
#include "app/cmd/copy_region.h"
#include "app/cmd/crop_cel.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/flip_image.h"
#include "app/cmd/flip_mask.h"
#include "app/cmd/layer_from_background.h"
#include "app/cmd/move_layer.h"
#include "app/cmd/patch_cel.h"
#include "app/cmd/remap_colors.h"
#include "app/cmd/remap_tilemaps.h"
#include "app/cmd/remap_tileset.h"
#include "app/cmd/remove_cel.h"
#include "app/cmd/remove_frame.h"
#include "app/cmd/remove_layer.h"
#include "app/cmd/remove_palette.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/replace_tileset.h"
#include "app/cmd/reselect_mask.h"
#include "app/cmd/sequence.h"
#include "app/cmd/set_cel_bounds.h"
#include "app/cmd/set_cel_data.h"
#include "app/cmd/set_cel_frame.h"
#include "app/cmd/set_cel_image.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_cel_position.h"
#include "app/cmd/set_cel_zindex.h"
#include "app/cmd/set_frame_duration.h"
#include "app/cmd/set_grid_bounds.h"
#include "app/cmd/set_last_point.h"
#include "app/cmd/set_layer_blend_mode.h"
#include "app/cmd/set_layer_flags.h"
#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/cmd/set_layer_tileset.h"
#include "app/cmd/set_mask.h"
#include "app/cmd/set_mask_position.h"
#include "app/cmd/set_palette.h"
#include "app/cmd/set_pixel_format.h"
#include "app/cmd/set_pixel_ratio.h"
#include "app/cmd/set_slice_key.h"
#include "app/cmd/set_slice_name.h"
#include "app/cmd/set_sprite_size.h"
#include "app/cmd/set_sprite_tile_management_plugin.h"
#include "app/cmd/set_tag_anidir.h"
#include "app/cmd/set_tag_name.h"
#include "app/cmd/set_tag_range.h"
#include "app/cmd/set_tag_repeat.h"
#include "app/cmd/set_tile_data.h"
#include "app/cmd/set_tile_data_properties.h"
#include "app/cmd/set_tile_data_property.h"
#include "app/cmd/set_tileset_base_index.h"
#include "app/cmd/set_tileset_match_flags.h"
#include "app/cmd/set_tileset_name.h"
#include "app/cmd/set_total_frames.h"
#include "app/cmd/set_transparent_color.h"
#include "app/cmd/set_user_data.h"
#include "app/cmd/set_user_data_properties.h"
#include "app/cmd/set_user_data_property.h"
#include "app/cmd/transaction.h"
#include "app/cmd/unlink_cel.h"

#include <algorithm>

namespace app {

// static
Cmd* Cmd::Decode(CmdSerial& s)
{
  cmdtype_t t = 0;
  s.cmdtype(t);
  s.unused(); // Mark as "unused token", as it's going to be read again in onSerialize()

#define CMDCASE(n)                                                                                 \
  case cmd::n::kType:                                                                              \
    PRINTARGS("Cmd::Decode " #n);                                                                  \
    cmd = new cmd::n(s);                                                                           \
    break;

  Cmd* cmd = nullptr;
  switch (t) {
    CMDCASE(AddCel);
    CMDCASE(AddFrame);
    CMDCASE(AddLayer);
    CMDCASE(ClearCel);
    CMDCASE(ClearImage);
    CMDCASE(ClearMask);
    CMDCASE(CmdSequence);
    CMDCASE(CmdTransaction);
    CMDCASE(CopyRegion);
    CMDCASE(CropCel);
    CMDCASE(RemoveCel);
    CMDCASE(RemoveFrame);
    CMDCASE(RemoveLayer);

    // TODO implement all cases
  }

  return cmd;
}

} // namespace app
