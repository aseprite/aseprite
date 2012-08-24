# ASEPRITE
# Copyright (C) 2001-2012  David Capello

add_definitions(-DHAVE_CONFIG_H)

if(MSVC)
  # Do not link with libcmt.lib (to avoid duplicated symbols with msvcrtd.lib)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif()
endif(MSVC)

# Libraries in this directory
set(aseprite_libraries aseprite-library undo-lib filters-lib ui-lib she gfx-lib base-lib)

# Directories where .h files can be found
include_directories(. .. ../third_party)

# Third-party libraries

if(USE_SHARED_JPEGLIB)
  find_package(JPEG)
  if(JPEG_FOUND)
    set(libs3rdparty ${libs3rdparty} ${JPEG_LIBRARIES})
    include_directories(${JPEG_INCLUDE_DIR})
  endif(JPEG_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} jpeg)
  include_directories(${LIBJPEG_DIR})
endif()

if(USE_SHARED_ZLIB)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    set(libs3rdparty ${libs3rdparty} ${ZLIB_LIBRARIES})
    include_directories(${ZLIB_INCLUDE_DIR})
  endif(ZLIB_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} zlib)
  include_directories(${ZLIB_DIR})
endif()

if(USE_SHARED_LIBPNG)
  find_package(PNG)
  if(PNG_FOUND)
    set(libs3rdparty ${libs3rdparty} ${PNG_LIBRARIES})
    add_definitions(${PNG_DEFINITIONS})
    include_directories(${PNG_INCLUDE_DIR})
  endif(PNG_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} libpng)
endif()

if(USE_SHARED_GIFLIB)
  find_package(GIF REQUIRED)
  if(GIF_FOUND)
    set(libs3rdparty ${libs3rdparty} ${GIF_LIBRARIES})
    include_directories(${GIF_INCLUDE_DIR})
  endif(GIF_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} giflib)
  include_directories(${GIFLIB_DIR}/lib)
endif()

if(USE_SHARED_TINYXML)
  find_library(LIBTINYXML_LIBRARY NAMES tinyxml)
  find_path(LIBTINYXML_INCLUDE_DIR NAMES tinyxml.h)

  set(libs3rdparty ${libs3rdparty} ${LIBTINYXML_LIBRARY})
  include_directories(${LIBTINYXML_INCLUDE_DIR})
else()
  set(libs3rdparty ${libs3rdparty} tinyxml)
  include_directories(${TINYXML_DIR})
endif()

if(USE_SHARED_LIBLOADPNG)
  find_library(LIBLOADPNG_LIBRARY NAMES loadpng)
  find_path(LIBLOADPNG_INCLUDE_DIR NAMES loadpng.h)

  set(libs3rdparty ${libs3rdparty} ${LIBLOADPNG_LIBRARY})
  include_directories(${LIBLOADPNG_INCLUDE_DIR})
else()
  set(libs3rdparty loadpng ${libs3rdparty})
  include_directories(${LOADPNG_DIR})
endif()

if (CMAKE_USE_PTHREADS_INIT)
  set(sys_libs ${sys_libs} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(ENABLE_UPDATER)
  if(USE_SHARED_CURL)
    find_library(LIBCURL_LIBRARY NAMES curl)
    find_path(LIBCURL_INCLUDE_DIR NAMES curl/curl.h)

    set(libs3rdparty ${libs3rdparty} ${LIBCURL_LIBRARY})
    include_directories(${LIBCURL_INCLUDE_DIR})
  else()
    set(libs3rdparty ${libs3rdparty} libcurl)
    include_directories(${CURL_DIR}/include)
    if(CURL_STATICLIB)
      add_definitions(-DCURL_STATICLIB)
    endif()
  endif()

  set(aseprite_libraries ${aseprite_libraries} updater-lib net-lib)
  add_definitions(-DENABLE_UPDATER)

  add_subdirectory(net)
  add_subdirectory(updater)
endif()

if(USE_SHARED_ALLEGRO4)
  # Find the shared Allegro 4 library
  find_library(LIBALLEGRO4_LIBRARY alleg)
  find_path(LIBALLEGRO4_INCLUDE_DIR allegro.h)

  if(NOT LIBALLEGRO4_LIBRARY)
    message(FATAL_ERROR "Allegro 4 not found")
  endif()

  # Get flags to link programs using allegro-config program
  execute_process(COMMAND allegro-config --libs --shared
    OUTPUT_VARIABLE LIBALLEGRO4_LINK_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  include_directories(${LIBALLEGRO4_INCLUDE_DIR})
else()
  # Use patched version of Allegro 4 (with window resize support).
  add_subdirectory(allegro)
  set(libs3rdparty ${libs3rdparty} allegro)
  add_definitions(-DALLEGRO4_WITH_RESIZE_PATCH)
endif()

# All libraries for .exe files
set(all_libs ${aseprite_libraries} ${libs3rdparty} ${sys_libs})

######################################################################
# ASEPRITE libraries

add_subdirectory(base)
add_subdirectory(filters)
add_subdirectory(gfx)
add_subdirectory(she)
add_subdirectory(ui)
add_subdirectory(undo)

add_library(aseprite-library
  app.cpp
  app_menus.cpp
  check_args.cpp
  console.cpp
  context.cpp
  context_flags.cpp
  context_observer_list.cpp
  document.cpp
  document_undo.cpp
  documents.cpp
  drop_files.cpp
  file_system.cpp
  flatten.cpp
  gfxmode.cpp
  gui_xml.cpp
  ini_file.cpp
  job.cpp
  launcher.cpp
  log.cpp
  modules.cpp
  objects_container_impl.cpp
  recent_files.cpp
  resource_finder.cpp
  thumbnail_generator.cpp
  ui_context.cpp
  undo_transaction.cpp
  xml_exception.cpp
  app/backup.cpp
  app/check_update.cpp
  app/color.cpp
  app/color_utils.cpp
  app/data_recovery.cpp
  app/file_selector.cpp
  app/widget_loader.cpp
  commands/cmd_about.cpp
  commands/cmd_advanced_mode.cpp
  commands/cmd_background_from_layer.cpp
  commands/cmd_cancel.cpp
  commands/cmd_canvas_size.cpp
  commands/cmd_cel_properties.cpp
  commands/cmd_change_color.cpp
  commands/cmd_change_image_type.cpp
  commands/cmd_change_pen.cpp
  commands/cmd_clear.cpp
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
  commands/cmd_film_editor.cpp
  commands/cmd_flatten_layers.cpp
  commands/cmd_flip.cpp
  commands/cmd_frame_properties.cpp
  commands/cmd_goto_frame.cpp
  commands/cmd_goto_layer.cpp
  commands/cmd_goto_tab.cpp
  commands/cmd_grid.cpp
  commands/cmd_import_sprite_sheet.cpp
  commands/cmd_invert_mask.cpp
  commands/cmd_launch.cpp
  commands/cmd_layer_from_background.cpp
  commands/cmd_layer_properties.cpp
  commands/cmd_load_mask.cpp
  commands/cmd_mask_all.cpp
  commands/cmd_mask_by_color.cpp
  commands/cmd_merge_down_layer.cpp
  commands/cmd_move_cel.cpp
  commands/cmd_new_file.cpp
  commands/cmd_new_frame.cpp
  commands/cmd_new_layer.cpp
  commands/cmd_new_layer_set.cpp
  commands/cmd_open_file.cpp
  commands/cmd_open_in_folder.cpp
  commands/cmd_open_with_app.cpp
  commands/cmd_options.cpp
  commands/cmd_palette_editor.cpp
  commands/cmd_paste.cpp
  commands/cmd_play_animation.cpp
  commands/cmd_preview.cpp
  commands/cmd_refresh.cpp
  commands/cmd_remove_cel.cpp
  commands/cmd_remove_frame.cpp
  commands/cmd_remove_layer.cpp
  commands/cmd_reselect_mask.cpp
  commands/cmd_rotate_canvas.cpp
  commands/cmd_save_file.cpp
  commands/cmd_save_mask.cpp
  commands/cmd_scroll.cpp
  commands/cmd_select_file.cpp
  commands/cmd_sprite_editor.cpp
  commands/cmd_sprite_properties.cpp
  commands/cmd_sprite_size.cpp
  commands/cmd_switch_colors.cpp
  commands/cmd_undo.cpp
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
  dialogs/aniedit.cpp
  dialogs/maskcol.cpp
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
  file/pcx_format.cpp
  file/png_format.cpp
  file/tga_format.cpp
  modules/editors.cpp
  modules/gfx.cpp
  modules/gui.cpp
  modules/palettes.cpp
  raster/algo.cpp
  raster/algo_polygon.cpp
  raster/algofill.cpp
  raster/algorithm/flip_image.cpp
  raster/blend.cpp
  raster/cel.cpp
  raster/cel_io.cpp
  raster/dirty.cpp
  raster/dirty_io.cpp
  raster/gfxobj.cpp
  raster/image.cpp
  raster/image_io.cpp
  raster/images_collector.cpp
  raster/layer.cpp
  raster/layer_io.cpp
  raster/mask.cpp
  raster/mask_io.cpp
  raster/palette.cpp
  raster/palette_io.cpp
  raster/pen.cpp
  raster/quantization.cpp
  raster/rgbmap.cpp
  raster/rotate.cpp
  raster/sprite.cpp
  raster/stock.cpp
  settings/ui_settings_impl.cpp
  skin/button_icon_impl.cpp
  skin/skin_theme.cpp
  skin/skin_property.cpp
  skin/skin_slider_property.cpp
  tools/intertwine.cpp
  tools/point_shape.cpp
  tools/tool_box.cpp
  tools/tool_loop_manager.cpp
  undoers/add_cel.cpp
  undoers/add_image.cpp
  undoers/add_layer.cpp
  undoers/add_palette.cpp
  undoers/close_group.cpp
  undoers/dirty_area.cpp
  undoers/flip_image.cpp
  undoers/image_area.cpp
  undoers/move_layer.cpp
  undoers/open_group.cpp
  undoers/remap_palette.cpp
  undoers/remove_cel.cpp
  undoers/remove_image.cpp
  undoers/remove_layer.cpp
  undoers/remove_palette.cpp
  undoers/replace_image.cpp
  undoers/set_cel_frame.cpp
  undoers/set_cel_opacity.cpp
  undoers/set_cel_position.cpp
  undoers/set_current_frame.cpp
  undoers/set_current_layer.cpp
  undoers/set_frame_duration.cpp
  undoers/set_layer_flags.cpp
  undoers/set_layer_name.cpp
  undoers/set_mask.cpp
  undoers/set_mask_position.cpp
  undoers/set_palette_colors.cpp
  undoers/set_sprite_pixel_format.cpp
  undoers/set_sprite_size.cpp
  undoers/set_stock_pixel_format.cpp
  undoers/set_total_frames.cpp
  util/autocrop.cpp
  util/boundary.cpp
  util/celmove.cpp
  util/clipboard.cpp
  util/col_file.cpp
  util/expand_cel_canvas.cpp
  util/filetoks.cpp
  util/gpl_file.cpp
  util/misc.cpp
  util/msk_file.cpp
  util/pic_file.cpp
  util/render.cpp
  util/thmbnail.cpp
  widgets/button_set.cpp
  widgets/color_bar.cpp
  widgets/color_button.cpp
  widgets/color_selector.cpp
  widgets/color_sliders.cpp
  widgets/drop_down_button.cpp
  widgets/editor/cursor.cpp
  widgets/editor/drawing_state.cpp
  widgets/editor/editor.cpp
  widgets/editor/editor_observers.cpp
  widgets/editor/editor_states_history.cpp
  widgets/editor/editor_view.cpp
  widgets/editor/keys.cpp
  widgets/editor/moving_cel_state.cpp
  widgets/editor/moving_pixels_state.cpp
  widgets/editor/pixels_movement.cpp
  widgets/editor/scrolling_state.cpp
  widgets/editor/select_box_state.cpp
  widgets/editor/standby_state.cpp
  widgets/editor/tool_loop_impl.cpp
  widgets/editor/transform_handles.cpp
  widgets/file_list.cpp
  widgets/file_selector.cpp
  widgets/hex_color_entry.cpp
  widgets/main_menu_bar.cpp
  widgets/main_window.cpp
  widgets/menuitem2.cpp
  widgets/palette_view.cpp
  widgets/popup_window_pin.cpp
  widgets/status_bar.cpp
  widgets/tabs.cpp
  widgets/toolbar.cpp)

######################################################################
# ASEPRITE application

if(WIN32)
  set(win32_resources resources_win32.rc)
endif(WIN32)

if(UNIX)
  set(x11_resources xpm_icon.c)
endif(UNIX)

add_executable(aseprite WIN32 main.cpp ${win32_resources} ${x11_resources})
target_link_libraries(aseprite ${all_libs})
if(LIBALLEGRO4_LINK_FLAGS)
  set_target_properties(aseprite
    PROPERTIES LINK_FLAGS ${LIBALLEGRO4_LINK_FLAGS})
endif()

install(TARGETS aseprite
  RUNTIME DESTINATION bin)

install(DIRECTORY ../data
  DESTINATION share/aseprite)

if(EXISTS ../docs/quickref.pdf)
  install(FILES ../docs/quickref.pdf
    DESTINATION share/aseprite/docs/quickref.pdf)
endif()

######################################################################
# Unit tests

function(find_unittests dir dependencies)
  file(GLOB tests ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/*_unittest.cpp)
  list(REMOVE_AT ARGV 0)

  if(NOT USE_SHARED_GTEST)
    # Add gtest include directory so we can #include <gtest/gtest.h> in tests source code
    include_directories(${CMAKE_SOURCE_DIR}/third_party/gtest/include)
  endif()

  foreach(testsourcefile ${tests})
    get_filename_component(testname ${testsourcefile} NAME_WE)

    add_executable(${testname} ${testsourcefile})
    target_link_libraries(${testname} gtest ${ARGV})
    if(LIBALLEGRO4_LINK_FLAGS)
      set_target_properties(${testname}
	PROPERTIES LINK_FLAGS ${LIBALLEGRO4_LINK_FLAGS})
    endif()

    add_custom_target(run_${testname}
      COMMAND ${testname}
      DEPENDS ${testname})

    set(local_runs ${local_runs} run_${testname})

    string(REGEX MATCH "_ui_unittest" test_requires_ui ${testname})
    if (NOT test_requires_ui STREQUAL "_ui_unittest")
      set(local_non_ui_runs ${local_non_ui_runs} run_${testname})
    endif()
  endforeach()
  set(all_runs ${all_runs} ${local_runs} PARENT_SCOPE)
  set(non_ui_runs ${non_ui_runs} ${local_non_ui_runs} PARENT_SCOPE)
endfunction()

find_unittests(base base-lib ${sys_libs})
find_unittests(gfx gfx-lib base-lib ${sys_libs})
find_unittests(ui ui-lib she gfx-lib base-lib ${libs3rdparty} ${sys_libs})
find_unittests(file ${all_libs})
find_unittests(app ${all_libs})
find_unittests(. ${all_libs})

# To run tests
add_custom_target(run_all_unittests DEPENDS ${all_runs})
add_custom_target(run_non_ui_unittests DEPENDS ${non_ui_runs})
