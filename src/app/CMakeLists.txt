# Aseprite
# Copyright (C) 2001-2015  David Capello

######################################################################
# Generate source files from widget XML files

file(GLOB widget_files ${CMAKE_SOURCE_DIR}/data/widgets/*.xml)
foreach(widget_file ${widget_files})
  get_filename_component(widget_name ${widget_file} NAME_WE)
  set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/generated_${widget_name}.h)

  add_custom_command(
    OUTPUT ${output_fn}
    COMMAND ${CMAKE_BINARY_DIR}/bin/gen --input ${widget_file} --widgetid ${widget_name} > ${output_fn}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    MAIN_DEPENDENCY ${widget_file}
    DEPENDS gen)

  list(APPEND generated_files ${output_fn})
endforeach()

# pref.h and pref.cpp
set(pref_xml ${CMAKE_SOURCE_DIR}/data/pref.xml)

set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/generated_pref_types.h)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${CMAKE_BINARY_DIR}/bin/gen --input ${pref_xml} --pref-h > ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${pref_xml}
  DEPENDS gen)
list(APPEND generated_files ${output_fn})

set(output_fn ${CMAKE_CURRENT_BINARY_DIR}/generated_pref_types.cpp)
add_custom_command(
  OUTPUT ${output_fn}
  COMMAND ${CMAKE_BINARY_DIR}/bin/gen --input ${pref_xml} --pref-cpp > ${output_fn}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  MAIN_DEPENDENCY ${pref_xml}
  DEPENDS gen)
list(APPEND generated_files ${output_fn})

# Directory where generated files by "gen" utility will stay.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

######################################################################
# app-lib target

add_library(app-lib
  app.cpp
  app_menus.cpp
  app_options.cpp
  app_render.cpp
  backup.cpp
  check_update.cpp
  cmd.cpp
  cmd/add_cel.cpp
  cmd/add_frame.cpp
  cmd/add_layer.cpp
  cmd/add_palette.cpp
  cmd/background_from_layer.cpp
  cmd/clear_cel.cpp
  cmd/clear_image.cpp
  cmd/clear_mask.cpp
  cmd/configure_background.cpp
  cmd/copy_cel.cpp
  cmd/copy_frame.cpp
  cmd/copy_rect.cpp
  cmd/copy_region.cpp
  cmd/deselect_mask.cpp
  cmd/flatten_layers.cpp
  cmd/flip_image.cpp
  cmd/flip_mask.cpp
  cmd/flip_masked_cel.cpp
  cmd/layer_from_background.cpp
  cmd/move_cel.cpp
  cmd/move_layer.cpp
  cmd/object_io.cpp
  cmd/remove_cel.cpp
  cmd/remove_frame.cpp
  cmd/remove_layer.cpp
  cmd/remove_palette.cpp
  cmd/replace_image.cpp
  cmd/reselect_mask.cpp
  cmd/set_cel_frame.cpp
  cmd/set_cel_opacity.cpp
  cmd/set_cel_position.cpp
  cmd/set_frame_duration.cpp
  cmd/set_layer_flags.cpp
  cmd/set_layer_name.cpp
  cmd/set_mask.cpp
  cmd/set_mask_position.cpp
  cmd/set_palette.cpp
  cmd/set_pixel_format.cpp
  cmd/set_sprite_size.cpp
  cmd/set_total_frames.cpp
  cmd/set_transparent_color.cpp
  cmd/with_cel.cpp
  cmd/with_document.cpp
  cmd/with_image.cpp
  cmd/with_layer.cpp
  cmd/with_palette.cpp
  cmd/with_sprite.cpp
  cmd_sequence.cpp
  cmd_transaction.cpp
  color.cpp
  color_picker.cpp
  color_swatches.cpp
  color_utils.cpp
  commands/cmd_about.cpp
  commands/cmd_advanced_mode.cpp
  commands/cmd_background_from_layer.cpp
  commands/cmd_cancel.cpp
  commands/cmd_canvas_size.cpp
  commands/cmd_cel_properties.cpp
  commands/cmd_change_brush.cpp
  commands/cmd_change_color.cpp
  commands/cmd_change_pixel_format.cpp
  commands/cmd_clear.cpp
  commands/cmd_clear_cel.cpp
  commands/cmd_close_file.cpp
  commands/cmd_configure_tools.cpp
  commands/cmd_copy.cpp
  commands/cmd_copy_cel.cpp
  commands/cmd_crop.cpp
  commands/cmd_cut.cpp
  commands/cmd_deselect_mask.cpp
  commands/cmd_developer_console.cpp
  commands/cmd_duplicate_layer.cpp
  commands/cmd_duplicate_sprite.cpp
  commands/cmd_exit.cpp
  commands/cmd_export_sprite_sheet.cpp
  commands/cmd_eyedropper.cpp
  commands/cmd_flatten_layers.cpp
  commands/cmd_flip.cpp
  commands/cmd_frame_properties.cpp
  commands/cmd_goto_frame.cpp
  commands/cmd_goto_layer.cpp
  commands/cmd_goto_tab.cpp
  commands/cmd_grid.cpp
  commands/cmd_import_sprite_sheet.cpp
  commands/cmd_invert_mask.cpp
  commands/cmd_keyboard_shortcuts.cpp
  commands/cmd_launch.cpp
  commands/cmd_layer_from_background.cpp
  commands/cmd_layer_properties.cpp
  commands/cmd_layer_visibility.cpp
  commands/cmd_load_mask.cpp
  commands/cmd_load_palette.cpp
  commands/cmd_mask_all.cpp
  commands/cmd_mask_by_color.cpp
  commands/cmd_mask_content.cpp
  commands/cmd_merge_down_layer.cpp
  commands/cmd_move_cel.cpp
  commands/cmd_move_mask.cpp
  commands/cmd_new_file.cpp
  commands/cmd_new_frame.cpp
  commands/cmd_new_layer.cpp
  commands/cmd_new_layer_set.cpp
  commands/cmd_onionskin.cpp
  commands/cmd_open_file.cpp
  commands/cmd_open_in_folder.cpp
  commands/cmd_open_with_app.cpp
  commands/cmd_options.cpp
  commands/cmd_palette_editor.cpp
  commands/cmd_paste.cpp
  commands/cmd_play_animation.cpp
  commands/cmd_preview.cpp
  commands/cmd_refresh.cpp
  commands/cmd_remove_frame.cpp
  commands/cmd_remove_layer.cpp
  commands/cmd_repeat_last_export.cpp
  commands/cmd_reselect_mask.cpp
  commands/cmd_reverse_frames.cpp
  commands/cmd_rotate.cpp
  commands/cmd_save_file.cpp
  commands/cmd_save_mask.cpp
  commands/cmd_save_palette.cpp
  commands/cmd_scroll.cpp
  commands/cmd_set_loop_section.cpp
  commands/cmd_set_palette.cpp
  commands/cmd_sprite_editor.cpp
  commands/cmd_sprite_properties.cpp
  commands/cmd_sprite_size.cpp
  commands/cmd_switch_colors.cpp
  commands/cmd_timeline.cpp
  commands/cmd_undo.cpp
  commands/cmd_zoom.cpp
  commands/command.cpp
  commands/commands.cpp
  commands/filters/cmd_color_curve.cpp
  commands/filters/cmd_convolution_matrix.cpp
  commands/filters/cmd_despeckle.cpp
  commands/filters/cmd_invert_color.cpp
  commands/filters/cmd_replace_color.cpp
  commands/filters/color_curve_editor.cpp
  commands/filters/convolution_matrix_stock.cpp
  commands/filters/filter_manager_impl.cpp
  commands/filters/filter_preview.cpp
  commands/filters/filter_target_buttons.cpp
  commands/filters/filter_window.cpp
  commands/filters/filter_worker.cpp
  console.cpp
  context.cpp
  context_flags.cpp
  data_recovery.cpp
  document.cpp
  document_api.cpp
  document_exporter.cpp
  document_location.cpp
  document_range.cpp
  document_range_ops.cpp
  document_undo.cpp
  file/ase_format.cpp
  file/bmp_format.cpp
  file/file.cpp
  file/file_format.cpp
  file/file_formats_manager.cpp
  file/fli/fli.cpp
  file/fli_format.cpp
  file/gif_format.cpp
  file/ico_format.cpp
  file/jpeg_format.cpp
  file/palette_file.cpp
  file/pcx_format.cpp
  file/png_format.cpp
  file/split_filename.cpp
  file/tga_format.cpp
  file_selector.cpp
  file_system.cpp
  filename_formatter.cpp
  flatten.cpp
  gui_xml.cpp
  handle_anidir.cpp
  ini_file.cpp
  job.cpp
  launcher.cpp
  log.cpp
  modules.cpp
  modules/editors.cpp
  modules/gfx.cpp
  modules/gui.cpp
  modules/palettes.cpp
  pref/preferences.cpp
  project.cpp
  recent_files.cpp
  res/palettes_loader_delegate.cpp
  res/resources_loader.cpp
  resource_finder.cpp
  send_crash.cpp
  settings/ui_settings_impl.cpp
  shell.cpp
  thumbnail_generator.cpp
  tools/intertwine.cpp
  tools/pick_ink.cpp
  tools/point_shape.cpp
  tools/shade_table.cpp
  tools/tool_box.cpp
  tools/tool_loop_manager.cpp
  transaction.cpp
  ui/app_menuitem.cpp
  ui/button_set.cpp
  ui/color_bar.cpp
  ui/color_button.cpp
  ui/color_selector.cpp
  ui/color_sliders.cpp
  ui/configure_timeline_popup.cpp
  ui/context_bar.cpp
  ui/devconsole_view.cpp
  ui/document_view.cpp
  ui/drop_down_button.cpp
  ui/editor/cursor.cpp
  ui/editor/drawing_state.cpp
  ui/editor/editor.cpp
  ui/editor/editor_observers.cpp
  ui/editor/editor_states_history.cpp
  ui/editor/editor_view.cpp
  ui/editor/moving_cel_state.cpp
  ui/editor/moving_pixels_state.cpp
  ui/editor/pixels_movement.cpp
  ui/editor/scrolling_state.cpp
  ui/editor/select_box_state.cpp
  ui/editor/standby_state.cpp
  ui/editor/tool_loop_impl.cpp
  ui/editor/transform_handles.cpp
  ui/editor/zooming_state.cpp
  ui/file_list.cpp
  ui/file_selector.cpp
  ui/hex_color_entry.cpp
  ui/keyboard_shortcuts.cpp
  ui/main_menu_bar.cpp
  ui/main_window.cpp
  ui/mini_editor.cpp
  ui/notifications.cpp
  ui/palette_popup.cpp
  ui/palette_view.cpp
  ui/palettes_listbox.cpp
  ui/popup_window_pin.cpp
  ui/resources_listbox.cpp
  ui/select_accelerator.cpp
  ui/skin/button_icon_impl.cpp
  ui/skin/skin_part.cpp
  ui/skin/skin_property.cpp
  ui/skin/skin_slider_property.cpp
  ui/skin/skin_theme.cpp
  ui/skin/style.cpp
  ui/skin/style_sheet.cpp
  ui/start_view.cpp
  ui/status_bar.cpp
  ui/styled_button.cpp
  ui/tabs.cpp
  ui/timeline.cpp
  ui/toolbar.cpp
  ui/workspace.cpp
  ui/workspace_part.cpp
  ui_context.cpp
  util/autocrop.cpp
  util/boundary.cpp
  util/clipboard.cpp
  util/expand_cel_canvas.cpp
  util/filetoks.cpp
  util/misc.cpp
  util/msk_file.cpp
  util/pic_file.cpp
  util/range_utils.cpp
  webserver.cpp
  widget_loader.cpp
  xml_document.cpp
  xml_exception.cpp
  ${generated_files})
