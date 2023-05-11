# Aseprite
# Copyright (C) 2018-2023  Igara Studio S.A.
# Copyright (C) 2001-2018  David Capello

# Generate a ui::Widget for each widget in a XML file
file(GLOB widget_files ${SOURCE_DATA_DIR}/widgets/*.xml)
foreach(widget_file ${widget_files})
  get_filename_component(widget_name ${widget_file} NAME_WE)
  set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/${widget_name}.xml.h)

  add_custom_command(
    OUTPUT ${output_fn}
    COMMAND ${GEN_EXE} --input ${widget_file} --widgetid ${widget_name} > ${output_fn}.tmp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_fn}.tmp ${output_fn}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    MAIN_DEPENDENCY ${widget_file}
    DEPENDS ${GEN_DEP})

  list(APPEND generated_files ${output_fn})
endforeach()

# Generate preference types from data/pref.xml
set(pref_xml ${SOURCE_DATA_DIR}/pref.xml)

set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/pref.xml.h)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${GEN_EXE} --input ${pref_xml} --pref-h > ${output_fn}.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_fn}.tmp ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${pref_xml}
  DEPENDS ${GEN_DEP})
list(APPEND generated_files ${output_fn})

set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/pref.xml.cpp)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${GEN_EXE} --input ${pref_xml} --pref-cpp > ${output_fn}.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_fn}.tmp ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${pref_xml}
  DEPENDS ${GEN_DEP})
list(APPEND generated_files ${output_fn})

# Generate theme.xml.h from data/extensions/aseprite-theme/theme.xml
set(theme_xml ${SOURCE_DATA_DIR}/extensions/aseprite-theme/theme.xml)
set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/theme.xml.h)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${GEN_EXE} --input ${theme_xml} --theme > ${output_fn}.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_fn}.tmp ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${theme_xml}
  DEPENDS ${GEN_DEP})
list(APPEND generated_files ${output_fn})

# Generate strings.ini.h from data/strings/en.ini
set(strings_en_ini ${SOURCE_DATA_DIR}/strings/en.ini)
set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/strings.ini.h)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${GEN_EXE} --input ${strings_en_ini} --strings > ${output_fn}.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_fn}.tmp ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${strings_en_ini}
  DEPENDS ${GEN_DEP})
list(APPEND generated_files ${output_fn})

# Generate command_ids.ini.h from data/strings/en.ini
set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/command_ids.ini.h)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${GEN_EXE} --input ${strings_en_ini} --command-ids > ${output_fn}.tmp
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_fn}.tmp ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${strings_en_ini}
  DEPENDS ${GEN_DEP})
list(APPEND generated_files ${output_fn})

# Check translations
file(GLOB string_files ${SOURCE_DATA_DIR}/strings/*.ini)
set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/check-translations.txt)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${GEN_EXE} --widgets-dir "${SOURCE_DATA_DIR}/widgets/" --strings-dir "${SOURCE_DATA_DIR}/strings/" --gui-file "${SOURCE_DATA_DIR}/gui.xml" >${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${GEN_DEP} ${widget_files} ${string_files} "${SOURCE_DATA_DIR}/gui.xml")
list(APPEND generated_files ${output_fn})

if(ENABLE_WEBP)
  add_definitions(-DENABLE_WEBP)
endif()

if(ENABLE_PSD)
  add_definitions(-DENABLE_PSD)
endif()

# libarchive definitions
add_definitions(-DLIBARCHIVE_STATIC)

######################################################################
# app-lib target

# These specific-platform files should be in an external library
# (e.g. "base" or "os").
set(app_platform_files)
if(WIN32)
  set(app_platform_files
    font_path_win.cpp)
elseif(APPLE)
  set(app_platform_files
    font_path_osx.mm)
else()
  set(app_platform_files
    font_path_unix.cpp)
endif()

set(data_recovery_files)
if(ENABLE_DATA_RECOVERY)
  set(data_recovery_files
    crash/backup_observer.cpp
    crash/data_recovery.cpp
    crash/read_document.cpp
    crash/session.cpp
    crash/write_document.cpp
    ui/data_recovery_view.cpp)
endif()

set(file_formats
  file/ase_format.cpp
  file/bmp_format.cpp
  file/css_format.cpp
  file/fli_format.cpp
  file/gif_format.cpp
  file/ico_format.cpp
  file/jpeg_format.cpp
  file/pcx_format.cpp
  file/png_format.cpp
  file/qoi_format.cpp
  file/svg_format.cpp
  file/tga_format.cpp)
if(ENABLE_WEBP)
  list(APPEND file_formats file/webp_format.cpp)
endif()
if(ENABLE_PSD)
  list(APPEND file_formats file/psd_format.cpp)
endif()

set(scripting_files)
if(ENABLE_SCRIPTING)
  set(scripting_files_ui)
  if(ENABLE_UI)
    set(scripting_files_ui
      commands/cmd_developer_console.cpp
      commands/cmd_open_script_folder.cpp
      commands/debugger.cpp
      ui/devconsole_view.cpp)
  endif()
  if(ENABLE_WEBSOCKET)
    set(scripting_files_ws
      script/websocket_class.cpp)
  endif()
  set(scripting_files
    commands/cmd_run_script.cpp
    script/app_command_object.cpp
    script/app_fs_object.cpp
    script/app_object.cpp
    script/app_theme_object.cpp
    script/brush_class.cpp
    script/canvas_widget.cpp
    script/cel_class.cpp
    script/cels_class.cpp
    script/color_class.cpp
    script/color_space_class.cpp
    script/dialog_class.cpp
    script/editor_class.cpp
    script/engine.cpp
    script/events_class.cpp
    script/frame_class.cpp
    script/frames_class.cpp
    script/graphics_context.cpp
    script/grid_class.cpp
    script/image_class.cpp
    script/image_iterator_class.cpp
    script/image_spec_class.cpp
    script/images_class.cpp
    script/keys.cpp
    script/layer_class.cpp
    script/layers_class.cpp
    script/luacpp.cpp
    script/palette_class.cpp
    script/palettes_class.cpp
    script/pixel_color_object.cpp
    script/plugin_class.cpp
    script/point_class.cpp
    script/preferences_object.cpp
    script/properties_class.cpp
    script/range_class.cpp
    script/rectangle_class.cpp
    script/require.cpp
    script/security.cpp
    script/selection_class.cpp
    script/site_class.cpp
    script/size_class.cpp
    script/slice_class.cpp
    script/slices_class.cpp
    script/sprite_class.cpp
    script/sprites_class.cpp
    script/tag_class.cpp
    script/tags_class.cpp
    script/tile_class.cpp
    script/tileset_class.cpp
    script/tilesets_class.cpp
    script/timer_class.cpp
    script/tool_class.cpp
    script/uuid_class.cpp
    script/values.cpp
    script/version_class.cpp
    shell.cpp
    ${scripting_files_ws}
    ${scripting_files_ui})
endif()

set(ui_app_files)
if(ENABLE_UI)
  set(ui_app_files
    app_brushes.cpp
    app_menus.cpp
    closed_docs.cpp
    commands/cmd_about.cpp
    commands/cmd_advanced_mode.cpp
    commands/cmd_cancel.cpp
    commands/cmd_cel_properties.cpp
    commands/cmd_change_brush.cpp
    commands/cmd_change_color.cpp
    commands/cmd_clear.cpp
    commands/cmd_clear_cel.cpp
    commands/cmd_clear_recent_files.cpp
    commands/cmd_close_file.cpp
    commands/cmd_contiguous_fill.cpp
    commands/cmd_copy.cpp
    commands/cmd_copy_cel.cpp
    commands/cmd_copy_merged.cpp
    commands/cmd_cut.cpp
    commands/cmd_discard_brush.cpp
    commands/cmd_duplicate_layer.cpp
    commands/cmd_duplicate_sprite.cpp
    commands/cmd_duplicate_view.cpp
    commands/cmd_exit.cpp
    commands/cmd_eyedropper.cpp
    commands/cmd_fit_screen.cpp
    commands/cmd_frame_properties.cpp
    commands/cmd_frame_tag_properties.cpp
    commands/cmd_fullscreen_mode.cpp
    commands/cmd_fullscreen_preview.cpp
    commands/cmd_goto_frame.cpp
    commands/cmd_goto_layer.cpp
    commands/cmd_goto_tab.cpp
    commands/cmd_grid.cpp
    commands/cmd_home.cpp
    commands/cmd_invert_mask.cpp
    commands/cmd_keyboard_shortcuts.cpp
    commands/cmd_launch.cpp
    commands/cmd_layer_lock.cpp
    commands/cmd_layer_opacity.cpp
    commands/cmd_layer_properties.cpp
    commands/cmd_layer_visibility.cpp
    commands/cmd_link_cels.cpp
    commands/cmd_load_mask.cpp
    commands/cmd_mask_by_color.cpp
    commands/cmd_mask_content.cpp
    commands/cmd_modify_selection.cpp
    commands/cmd_move_cel.cpp
    commands/cmd_move_mask.cpp
    commands/cmd_new_brush.cpp
    commands/cmd_new_frame_tag.cpp
    commands/cmd_new_sprite_from_selection.cpp
    commands/cmd_onionskin.cpp
    commands/cmd_open_browser.cpp
    commands/cmd_open_group.cpp
    commands/cmd_open_in_folder.cpp
    commands/cmd_open_with_app.cpp
    commands/cmd_options.cpp
    commands/cmd_palette_editor.cpp
    commands/cmd_paste.cpp
    commands/cmd_paste_text.cpp
    commands/cmd_pixel_perfect_mode.cpp
    commands/cmd_play_animation.cpp
    commands/cmd_refresh.cpp
    commands/cmd_enter_license.cpp
    commands/cmd_remove_frame.cpp
    commands/cmd_remove_frame_tag.cpp
    commands/cmd_remove_slice.cpp
    commands/cmd_reopen_closed_file.cpp
    commands/cmd_repeat_last_export.cpp
    commands/cmd_reselect_mask.cpp
    commands/cmd_reverse_frames.cpp
    commands/cmd_rotate.cpp
    commands/cmd_save_mask.cpp
    commands/cmd_save_palette.cpp
    commands/cmd_select_palette.cpp
    commands/cmd_scroll.cpp
    commands/cmd_scroll_center.cpp
    commands/cmd_select_tile.cpp
    commands/cmd_set_color_selector.cpp
    commands/cmd_set_ink_type.cpp
    commands/cmd_set_loop_section.cpp
    commands/cmd_set_palette.cpp
    commands/cmd_set_palette_entry_size.cpp
    commands/cmd_set_same_ink.cpp
    commands/cmd_slice_properties.cpp
    commands/cmd_sprite_properties.cpp
    commands/cmd_swap_checkerboard_colors.cpp
    commands/cmd_switch_colors.cpp
    commands/cmd_symmetry_mode.cpp
    commands/cmd_tiled_mode.cpp
    commands/cmd_timeline.cpp
    commands/cmd_toggle_preview.cpp
    commands/cmd_toggle_tiles_mode.cpp
    commands/cmd_toggle_timeline_thumbnails.cpp
    commands/cmd_undo_history.cpp
    commands/cmd_unlink_cel.cpp
    commands/cmd_zoom.cpp
    commands/filters/filter_preview.cpp
    commands/filters/filter_target_buttons.cpp
    commands/filters/filter_window.cpp
    commands/screenshot.cpp
    commands/set_playback_speed.cpp
    commands/show_menu.cpp
    commands/tileset_mode.cpp
    commands/toggle_play_option.cpp
    file_selector.cpp
    modules/gfx.cpp
    modules/gui.cpp
    ui/app_menuitem.cpp
    ui/backup_indicator.cpp
    ui/browser_view.cpp
    ui/brush_popup.cpp
    ui/button_set.cpp
    ui/color_bar.cpp
    ui/color_button.cpp
    ui/color_popup.cpp
    ui/color_selector.cpp
    ui/color_shades.cpp
    ui/color_sliders.cpp
    ui/color_spectrum.cpp
    ui/color_tint_shade_tone.cpp
    ui/color_wheel.cpp
    ui/configure_timeline_popup.cpp
    ui/context_bar.cpp
    ui/dithering_selector.cpp
    ui/doc_view.cpp
    ui/drop_down_button.cpp
    ui/dynamics_popup.cpp
    ui/editor/brush_preview.cpp
    ui/editor/delayed_mouse_move.cpp
    ui/editor/dragging_value_state.cpp
    ui/editor/drawing_state.cpp
    ui/editor/editor.cpp
    ui/editor/editor_observers.cpp
    ui/editor/editor_render.cpp
    ui/editor/editor_states_history.cpp
    ui/editor/editor_view.cpp
    ui/editor/moving_cel_state.cpp
    ui/editor/moving_pixels_state.cpp
    ui/editor/moving_selection_state.cpp
    ui/editor/moving_slice_state.cpp
    ui/editor/moving_symmetry_state.cpp
    ui/editor/navigate_state.cpp
    ui/editor/pivot_helpers.cpp
    ui/editor/pixels_movement.cpp
    ui/editor/play_state.cpp
    ui/editor/scrolling_state.cpp
    ui/editor/select_box_state.cpp
    ui/editor/standby_state.cpp
    ui/editor/state_with_wheel_behavior.cpp
    ui/editor/transform_handles.cpp
    ui/editor/zooming_state.cpp
    ui/export_file_window.cpp
    ui/expr_entry.cpp
    ui/file_list.cpp
    ui/file_list_view.cpp
    ui/file_selector.cpp
    ui/filename_field.cpp
    ui/font_popup.cpp
    ui/hex_color_entry.cpp
    ui/home_view.cpp
    ui/icon_button.cpp
    ui/incompat_file_window.cpp
    ui/input_chain.cpp
    ui/keyboard_shortcuts.cpp
    ui/main_menu_bar.cpp
    ui/main_window.cpp
    ui/notifications.cpp
    ui/optional_alert.cpp
    ui/palette_popup.cpp
    ui/palette_view.cpp
    ui/palettes_listbox.cpp
    ui/popup_window_pin.cpp
    ui/pref_widget.cpp
    ui/preview_editor.cpp
    ui/recent_listbox.cpp
    ui/resources_listbox.cpp
    ui/rgbmap_algorithm_selector.cpp
    ui/sampling_selector.cpp
    ui/search_entry.cpp
    ui/select_accelerator.cpp
    ui/selection_mode_field.cpp
    ui/skin/font_data.cpp
    ui/skin/skin_part.cpp
    ui/skin/skin_property.cpp
    ui/skin/skin_slider_property.cpp
    ui/skin/skin_theme.cpp
    ui/slice_window.cpp
    ui/slider2.cpp
    ui/status_bar.cpp
    ui/tabs.cpp
    ui/tag_window.cpp
    ui/task_widget.cpp
    ui/tile_button.cpp
    ui/tileset_selector.cpp
    ui/timeline/ani_controls.cpp
    ui/timeline/timeline.cpp
    ui/toolbar.cpp
    ui/user_data_view.cpp
    ui/workspace.cpp
    ui/workspace_panel.cpp
    ui/workspace_tabs.cpp
    ui/zoom_entry.cpp
    ui_context.cpp
    widget_loader.cpp)
  if(ENABLE_NEWS)
    set(ui_app_files
      res/http_loader.cpp
      ui/news_listbox.cpp
      ${ui_app_files})
  endif()
  if(ENABLE_DRM)
    set(ui_app_files
      ui/enter_license.cpp
      ui/aseprite_update.cpp
    ${ui_app_files})
  endif()
endif()

set(send_crash_files)
if(ENABLE_SENTRY)
  set(send_crash_files sentry_wrapper.cpp)
else()
  set(send_crash_files send_crash.cpp)
endif()

add_library(app-lib
  active_site_handler.cpp
  app.cpp
  check_update.cpp
  cli/app_options.cpp
  cli/cli_open_file.cpp
  cli/cli_processor.cpp
  ${file_formats}
  cli/default_cli_delegate.cpp
  cli/preview_cli_delegate.cpp
  cmd.cpp
  cmd/add_cel.cpp
  cmd/add_frame.cpp
  cmd/add_layer.cpp
  cmd/add_palette.cpp
  cmd/add_slice.cpp
  cmd/add_tag.cpp
  cmd/add_tile.cpp
  cmd/add_tileset.cpp
  cmd/assign_color_profile.cpp
  cmd/background_from_layer.cpp
  cmd/clear_cel.cpp
  cmd/clear_image.cpp
  cmd/clear_mask.cpp
  cmd/clear_rect.cpp
  cmd/configure_background.cpp
  cmd/convert_color_profile.cpp
  cmd/copy_cel.cpp
  cmd/copy_frame.cpp
  cmd/copy_rect.cpp
  cmd/copy_region.cpp
  cmd/crop_cel.cpp
  cmd/deselect_mask.cpp
  cmd/flatten_layers.cpp
  cmd/flip_image.cpp
  cmd/flip_mask.cpp
  cmd/flip_masked_cel.cpp
  cmd/layer_from_background.cpp
  cmd/move_cel.cpp
  cmd/move_layer.cpp
  cmd/patch_cel.cpp
  cmd/remap_colors.cpp
  cmd/remap_tilemaps.cpp
  cmd/remap_tileset.cpp
  cmd/remove_cel.cpp
  cmd/remove_frame.cpp
  cmd/remove_layer.cpp
  cmd/remove_palette.cpp
  cmd/remove_slice.cpp
  cmd/remove_tag.cpp
  cmd/remove_tile.cpp
  cmd/remove_tileset.cpp
  cmd/replace_image.cpp
  cmd/replace_tileset.cpp
  cmd/reselect_mask.cpp
  cmd/set_cel_bounds.cpp
  cmd/set_cel_data.cpp
  cmd/set_cel_frame.cpp
  cmd/set_cel_opacity.cpp
  cmd/set_cel_position.cpp
  cmd/set_cel_zindex.cpp
  cmd/set_frame_duration.cpp
  cmd/set_grid_bounds.cpp
  cmd/set_last_point.cpp
  cmd/set_layer_blend_mode.cpp
  cmd/set_layer_flags.cpp
  cmd/set_layer_name.cpp
  cmd/set_layer_opacity.cpp
  cmd/set_layer_tileset.cpp
  cmd/set_mask.cpp
  cmd/set_mask_position.cpp
  cmd/set_palette.cpp
  cmd/set_pixel_format.cpp
  cmd/set_pixel_ratio.cpp
  cmd/set_slice_key.cpp
  cmd/set_slice_name.cpp
  cmd/set_sprite_size.cpp
  cmd/set_sprite_tile_management_plugin.cpp
  cmd/set_tag_anidir.cpp
  cmd/set_tag_color.cpp
  cmd/set_tag_name.cpp
  cmd/set_tag_range.cpp
  cmd/set_tag_repeat.cpp
  cmd/set_tile_data.cpp
  cmd/set_tile_data_properties.cpp
  cmd/set_tile_data_property.cpp
  cmd/set_tileset_base_index.cpp
  cmd/set_tileset_name.cpp
  cmd/set_total_frames.cpp
  cmd/set_transparent_color.cpp
  cmd/set_user_data.cpp
  cmd/set_user_data_properties.cpp
  cmd/set_user_data_property.cpp
  cmd/shift_masked_cel.cpp
  cmd/trim_cel.cpp
  cmd/unlink_cel.cpp
  cmd/with_cel.cpp
  cmd/with_document.cpp
  cmd/with_image.cpp
  cmd/with_layer.cpp
  cmd/with_slice.cpp
  cmd/with_sprite.cpp
  cmd/with_tag.cpp
  cmd/with_tileset.cpp
  cmd_sequence.cpp
  cmd_transaction.cpp
  color.cpp
  color_picker.cpp
  color_spaces.cpp
  color_utils.cpp
  commands/cmd_add_color.cpp
  commands/cmd_background_from_layer.cpp
  commands/cmd_canvas_size.cpp
  commands/cmd_cel_opacity.cpp
  commands/cmd_change_pixel_format.cpp
  commands/cmd_color_quantization.cpp
  commands/cmd_crop.cpp
  commands/cmd_deselect_mask.cpp
  commands/cmd_export_sprite_sheet.cpp
  commands/cmd_fill_and_stroke.cpp
  commands/cmd_flatten_layers.cpp
  commands/cmd_flip.cpp
  commands/cmd_import_sprite_sheet.cpp
  commands/cmd_layer_from_background.cpp
  commands/cmd_load_palette.cpp
  commands/cmd_mask_all.cpp
  commands/cmd_merge_down_layer.cpp
  commands/cmd_new_file.cpp
  commands/cmd_new_frame.cpp
  commands/cmd_new_layer.cpp
  commands/cmd_open_file.cpp
  commands/cmd_palette_size.cpp
  commands/cmd_remove_layer.cpp
  commands/cmd_save_file.cpp
  commands/cmd_show.cpp
  commands/cmd_sprite_size.cpp
  commands/cmd_undo.cpp
  commands/command.cpp
  commands/commands.cpp
  commands/convert_layer.cpp
  commands/copy_path.cpp
  commands/export_tileset.cpp
  commands/filters/cmd_brightness_contrast.cpp
  commands/filters/cmd_color_curve.cpp
  commands/filters/cmd_convolution_matrix.cpp
  commands/filters/cmd_despeckle.cpp
  commands/filters/cmd_hue_saturation.cpp
  commands/filters/cmd_invert_color.cpp
  commands/filters/cmd_outline.cpp
  commands/filters/cmd_replace_color.cpp
  commands/filters/color_curve_editor.cpp
  commands/filters/convolution_matrix_stock.cpp
  commands/filters/filter_manager_impl.cpp
  commands/filters/filter_worker.cpp
  commands/move_colors_command.cpp
  commands/move_thing.cpp
  commands/move_tiles_command.cpp
  commands/new_params.cpp
  commands/quick_command.cpp
  console.cpp
  context.cpp
  context_flags.cpp
  doc.cpp
  doc_api.cpp
  doc_diff.cpp
  doc_exporter.cpp
  doc_range.cpp
  doc_range_ops.cpp
  doc_undo.cpp
  docs.cpp
  extensions.cpp
  extra_cel.cpp
  file/file.cpp
  file/file_data.cpp
  file/file_format.cpp
  file/file_formats_manager.cpp
  file/file_op_config.cpp
  file/palette_file.cpp
  file/split_filename.cpp
  file_system.cpp
  filename_formatter.cpp
  flatten.cpp
  font_path.cpp
  gui_xml.cpp
  i18n/strings.cpp
  i18n/xml_translator.cpp
  ini_file.cpp
  job.cpp
  launcher.cpp
  load_matrix.cpp
  log.cpp
  loop_tag.cpp
  modules.cpp
  modules/palettes.cpp
  pref/preferences.cpp
  recent_files.cpp
  render/shader_renderer.cpp
  render/simple_renderer.cpp
  res/palettes_loader_delegate.cpp
  res/resources_loader.cpp
  resource_finder.cpp
  restore_visible_layers.cpp
  shade.cpp
  site.cpp
  snap_to_grid.cpp
  sprite_job.cpp
  task.cpp
  thumbnail_generator.cpp
  thumbnails.cpp
  tools/active_tool.cpp
  tools/ink_type.cpp
  tools/intertwine.cpp
  tools/pick_ink.cpp
  tools/point_shape.cpp
  tools/stroke.cpp
  tools/symmetry.cpp
  tools/tool_box.cpp
  tools/tool_loop_manager.cpp
  tools/velocity.cpp
  transaction.cpp
  transformation.cpp
  ui/editor/tool_loop_impl.cpp
  ui/layer_frame_comboboxes.cpp
  util/autocrop.cpp
  util/buffer_region.cpp
  util/cel_ops.cpp
  util/clipboard.cpp
  util/clipboard_native.cpp
  util/conversion_to_surface.cpp
  util/expand_cel_canvas.cpp
  util/filetoks.cpp
  util/freetype_utils.cpp
  util/layer_boundaries.cpp
  util/layer_utils.cpp
  util/msk_file.cpp
  util/new_image_from_mask.cpp
  util/pal_ops.cpp
  util/pic_file.cpp
  util/pixel_ratio.cpp
  util/range_utils.cpp
  util/readable_time.cpp
  util/resize_image.cpp
  util/wrap_point.cpp
  xml_document.cpp
  xml_exception.cpp
  ${send_crash_files}
  ${ui_app_files}
  ${app_platform_files}
  ${data_recovery_files}
  ${scripting_files}
  ${generated_files})

if(TARGET generated_version)
  add_dependencies(app-lib generated_version)
endif()

target_link_libraries(app-lib
  laf-base
  cfg-lib
  clip
  doc-lib
  dio-lib
  filters-lib
  fixmath-lib
  flic-lib
  tga-lib
  laf-gfx
  render-lib
  laf-ft
  laf-os
  ui-lib
  ver-lib
  undo
  ${CMARK_LIBRARIES}
  ${TINYXML_LIBRARY}
  ${JPEG_LIBRARIES}
  ${GIF_LIBRARIES}
  ${PNG_LIBRARIES}
  ${WEBP_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${HARFBUZZ_LIBRARIES}
  json11
  archive_static
  fmt
  tinyexpr
  qoi)

if(ENABLE_PSD)
  target_link_libraries(app-lib psd)
endif()

if(ENABLE_DRM)
  target_link_libraries(app-lib drm-lib)
endif()

# Directory where generated files by "gen" utility will stay.
target_include_directories(app-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if(REQUIRE_CURL)
  target_link_libraries(app-lib net-lib)
endif()

if(ENABLE_SCRIPTING)
  target_link_libraries(app-lib lua lauxlib lualib)

  if(ENABLE_WEBSOCKET)
    target_link_libraries(app-lib ixwebsocket)
  endif()
endif()

if(ENABLE_UPDATER)
  target_link_libraries(app-lib updater-lib)
endif()

if(ENABLE_STEAM)
  add_definitions(-DENABLE_STEAM)
  target_link_libraries(app-lib steam-lib)
endif()

if(ENABLE_SENTRY)
  target_compile_definitions(app-lib PUBLIC
    -DENABLE_SENTRY
    -DSENTRY_BUILD_STATIC=1
    -DSENTRY_DSN="${SENTRY_DSN}"
    -DSENTRY_ENV="${SENTRY_ENV}")
  add_subdirectory(${SENTRY_DIR} sentry)
  target_link_libraries(app-lib sentry)
endif()
