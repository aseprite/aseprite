// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_ENGINE_FUNCTIONS_H
#define APP_SCRIPT_ENGINE_FUNCTIONS_H

#include "lua.h"

#include "app/color.h"
#include "app/commands/params.h"
#include "app/extensions.h"
#include "base/uuid.h"
#include "doc/brush.h"
#include "doc/frame.h"
#include "doc/object_ids.h"
#include "doc/pixel_format.h"
#include "doc/tile.h"
#include "doc/user_data.h"

struct lua_State;
struct lua_Debug;

namespace base {
class Version;
}

namespace gfx {
class ColorSpace;
}

namespace doc {
class Cel;
class Image;
class Layer;
class Mask;
class Palette;
class Sprite;
class Tag;
class Tileset;
class Tilesets;
class WithUserData;
} // namespace doc

namespace ui {
class Window;
}

namespace app {
class Editor;
class Site;

namespace tools {
class Tool;
}

namespace script {

int ObjectIterator_pairs_next(lua_State* L);
void push_app_events(lua_State* L);
void push_app_theme(lua_State* L, int uiscale = 1);
void push_app_clipboard(lua_State* L);
int push_image_iterator_function(lua_State* L, const doc::Image* image, int extraArgIndex);
void push_brush(lua_State* L, const doc::BrushRef& brush);
void push_cel_image(lua_State* L, doc::Cel* cel);
void push_cel_images(lua_State* L, const doc::ObjectIds& cels);
void push_cels(lua_State* L, const doc::ObjectIds& cels);
void push_cels(lua_State* L, doc::Layer* layer);
void push_cels(lua_State* L, doc::Sprite* sprite);
void push_color_space(lua_State* L, const gfx::ColorSpace& cs);
void push_doc_range(lua_State* L, Site& site);
void push_editor(lua_State* L, Editor* editor);
void push_group_layers(lua_State* L, doc::Layer* group);
void push_image(lua_State* L, doc::Image* image);
void push_layers(lua_State* L, const doc::ObjectIds& layers);
void push_palette(lua_State* L, doc::Palette* palette);
void push_plugin(lua_State* L, Extension* ext);
void push_properties(lua_State* L, doc::WithUserData* userData, const std::string& extID);
void push_sprite_cel(lua_State* L, doc::Cel* cel);
void push_sprite_events(lua_State* L, doc::Sprite* sprite);
void push_sprite_frame(lua_State* L, doc::Sprite* sprite, doc::frame_t frame);
void push_sprite_frames(lua_State* L, doc::Sprite* sprite);
void push_sprite_frames(lua_State* L, doc::Sprite* sprite, const std::vector<doc::frame_t>& frames);
void push_sprite_layers(lua_State* L, doc::Sprite* sprite);
void push_sprite_palette(lua_State* L, doc::Sprite* sprite, doc::Palette* palette);
void push_sprite_palettes(lua_State* L, doc::Sprite* sprite);
void push_sprite_selection(lua_State* L, doc::Sprite* sprite);
void push_sprite_slices(lua_State* L, doc::Sprite* sprite);
void push_sprite_tags(lua_State* L, doc::Sprite* sprite);
void push_sprites(lua_State* L);
void push_standalone_selection(lua_State* L, doc::Mask* mask);
void push_tile(lua_State* L, const doc::Tileset* tileset, doc::tile_index ti);
void push_tile_properties(lua_State* L,
                          const doc::Tileset* tileset,
                          doc::tile_index ti,
                          const std::string& extID);
void push_tileset(lua_State* L, const doc::Tileset* tileset);
void push_tileset_image(lua_State* L, doc::Tileset* tileset, doc::tile_index ti);
void push_tilesets(lua_State* L, doc::Tilesets* tilesets);
void push_tool(lua_State* L, tools::Tool* tool);
void push_version(lua_State* L, const base::Version& ver);
void push_window_events(lua_State* L, ui::Window* window);

gfx::Point convert_args_into_point(lua_State* L, int index);
gfx::Rect convert_args_into_rect(lua_State* L, int index);
gfx::Size convert_args_into_size(lua_State* L, int index);
app::Color convert_args_into_color(lua_State* L, int index);
doc::color_t convert_args_into_pixel_color(lua_State* L, int index, doc::PixelFormat pixelFormat);
base::Uuid convert_args_into_uuid(lua_State* L, int index);
doc::Palette* get_palette_from_arg(lua_State* L, int index);
doc::Image* may_get_image_from_arg(lua_State* L, int index);
doc::Image* get_image_from_arg(lua_State* L, int index);
doc::Cel* get_image_cel_from_arg(lua_State* L, int index);
doc::Tileset* get_image_tileset_from_arg(lua_State* L, int index);
doc::frame_t get_frame_number_from_arg(lua_State* L, int index);
doc::Mask* get_mask_from_arg(lua_State* L, int index);
tools::Tool* get_tool_from_arg(lua_State* L, int index);
doc::BrushRef get_brush_from_arg(lua_State* L, int index);
doc::Tileset* get_tile_index_from_arg(lua_State* L, int index, doc::tile_index& ts);
doc::UserData::Properties* may_get_properties(lua_State* L, int index);

void register_app_object(lua_State* L);
void register_app_pixel_color_object(lua_State* L);
void register_app_fs_object(lua_State* L);
void register_app_os_object(lua_State* L);
void register_app_command_object(lua_State* L);
void register_app_preferences_object(lua_State* L);
void register_json_object(lua_State* L);
void register_iterator_class(lua_State* L);
void register_brush_class(lua_State* L);
void register_cel_class(lua_State* L);
void register_cels_class(lua_State* L);
void register_color_class(lua_State* L);
void register_color_space_class(lua_State* L);
void register_dialog_class(lua_State* L);
void register_editor_class(lua_State* L);
void register_graphics_context_class(lua_State* L);
void register_window_class(lua_State* L);
void register_events_class(lua_State* L);
void register_frame_class(lua_State* L);
void register_frames_class(lua_State* L);
void register_grid_class(lua_State* L);
void register_image_class(lua_State* L);
void register_image_iterator_class(lua_State* L);
void register_image_spec_class(lua_State* L);
void register_images_class(lua_State* L);
void register_layer_class(lua_State* L);
void register_layers_class(lua_State* L);
void register_palette_class(lua_State* L);
void register_palettes_class(lua_State* L);
void register_plugin_class(lua_State* L);
void register_point_class(lua_State* L);
void register_properties_class(lua_State* L);
void register_range_class(lua_State* L);
void register_rect_class(lua_State* L);
void register_selection_class(lua_State* L);
void register_site_class(lua_State* L);
void register_size_class(lua_State* L);
void register_slice_class(lua_State* L);
void register_slices_class(lua_State* L);
void register_sprite_class(lua_State* L);
void register_sprites_class(lua_State* L);
void register_tag_class(lua_State* L);
void register_tags_class(lua_State* L);
void register_theme_classes(lua_State* L);
void register_clipboard_classes(lua_State* L);
void register_tile_class(lua_State* L);
void register_tileset_class(lua_State* L);
void register_tilesets_class(lua_State* L);
void register_timer_class(lua_State* L);
void register_tool_class(lua_State* L);
void register_uuid_class(lua_State* L);
void register_version_class(lua_State* L);
void register_websocket_class(lua_State* L);
void set_app_params(lua_State* L, const Params& params);

constexpr auto engine_registration_functions = {
  register_iterator_class,
  register_app_object,
  register_app_pixel_color_object,
  register_app_fs_object,
  register_app_os_object,
  register_app_command_object,
  register_app_preferences_object,
  register_json_object,
  register_brush_class,
  register_cel_class,
  register_cels_class,
  register_color_class,
  register_color_space_class,
  register_dialog_class,
  register_editor_class,
  register_graphics_context_class,
  register_window_class,
  register_events_class,
  register_frame_class,
  register_frames_class,
  register_grid_class,
  register_image_class,
  register_image_iterator_class,
  register_image_spec_class,
  register_images_class,
  register_layer_class,
  register_layers_class,
  register_palette_class,
  register_palettes_class,
  register_plugin_class,
  register_point_class,
  register_properties_class,
  register_range_class,
  register_rect_class,
  register_selection_class,
  register_site_class,
  register_size_class,
  register_slice_class,
  register_slices_class,
  register_sprite_class,
  register_sprites_class,
  register_tag_class,
  register_tags_class,
  register_theme_classes,
  register_clipboard_classes,
  register_tile_class,
  register_tileset_class,
  register_tilesets_class,
  register_timer_class,
  register_tool_class,
  register_uuid_class,
  register_version_class,
#if ENABLE_WEBSOCKET
  register_websocket_class,
#endif
};
} // namespace script
} // namespace app

#endif // APP_SCRIPT_ENGINE_FUNCTIONS_H
