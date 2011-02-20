# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2011  David Capello

if(MSVC)
  # Do not link with libcmt.lib (to avoid duplicated symbols with msvcrtd.lib)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif()
endif(MSVC)

# Third-party libraries
set(libs3rdparty freetype libart_lgpl loadpng tinyxml giflib)

if(USE_SHARED_JPEGLIB)
  find_package(JPEG)
  if(JPEG_FOUND)
    set(libs3rdparty ${libs3rdparty} ${JPEG_LIBRARIES})
    include_directories(${JPEG_INCLUDE_DIR})
  endif(JPEG_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} jpeg)
endif()

if(USE_SHARED_ZLIB)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    set(libs3rdparty ${libs3rdparty} ${ZLIB_LIBRARIES})
    include_directories(${ZLIB_INCLUDE_DIR})
  endif(ZLIB_FOUND)
else()
  set(libs3rdparty ${libs3rdparty} zlib)
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

# All libraries for .exe files
set(all_libs aseprite-library gui-lib gfx-lib base-lib ${libs3rdparty} allegro ${sys_libs})

# Directories where .h files can be found
include_directories(
  . .. ../third_party
  ${GIFLIB_DIR}/lib
  ${LIBFREETYPE_DIR}/include
  ${LIBJPEG_DIR}
  ${LIBPNG_DIR}
  ${ZLIB_DIR}
  ${LOADPNG_DIR}
  ${TINYXML_DIR})

######################################################################
# Sub-libraries

add_subdirectory(allegro)
add_subdirectory(base)
add_subdirectory(gfx)
add_subdirectory(gui)

######################################################################
# aseprite library

add_library(aseprite-library
  app.cpp
  check_args.cpp
  console.cpp
  context.cpp
  gfxmode.cpp
  gui_xml.cpp
  job.cpp
  launcher.cpp
  log.cpp
  recent_files.cpp
  resource_finder.cpp
  ui_context.cpp
  undoable.cpp
  xml_exception.cpp
  xml_widgets.cpp
  app/color.cpp
  app/color_utils.cpp
  commands/cmd_about.cpp
  commands/cmd_advanced_mode.cpp
  commands/cmd_background_from_layer.cpp
  commands/cmd_canvas_size.cpp
  commands/cmd_cel_properties.cpp
  commands/cmd_change_color.cpp
  commands/cmd_change_image_type.cpp
  commands/cmd_change_pen.cpp
  commands/cmd_check_updates.cpp
  commands/cmd_clear.cpp
  commands/cmd_close_file.cpp
  commands/cmd_configure_tools.cpp
  commands/cmd_copy.cpp
  commands/cmd_copy_cel.cpp
  commands/cmd_crop.cpp
  commands/cmd_cut.cpp
  commands/cmd_deselect_mask.cpp
  commands/cmd_donate.cpp
  commands/cmd_duplicate_layer.cpp
  commands/cmd_duplicate_sprite.cpp
  commands/cmd_exit.cpp
  commands/cmd_eyedropper.cpp
  commands/cmd_film_editor.cpp
  commands/cmd_flatten_layers.cpp
  commands/cmd_flip.cpp
  commands/cmd_frame_properties.cpp
  commands/cmd_goto_frame.cpp
  commands/cmd_goto_layer.cpp
  commands/cmd_grid.cpp
  commands/cmd_invert_mask.cpp
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
  commands/cmd_options.cpp
  commands/cmd_palette_editor.cpp
  commands/cmd_paste.cpp
  commands/cmd_play_animation.cpp
  commands/cmd_preview.cpp
  commands/cmd_quick_reference.cpp
  commands/cmd_redo.cpp
  commands/cmd_refresh.cpp
  commands/cmd_remove_cel.cpp
  commands/cmd_remove_frame.cpp
  commands/cmd_remove_layer.cpp
  commands/cmd_reselect_mask.cpp
  commands/cmd_rotate_canvas.cpp
  commands/cmd_save_file.cpp
  commands/cmd_save_mask.cpp
  commands/cmd_screen_shot.cpp
  commands/cmd_select_file.cpp
  commands/cmd_sprite_editor.cpp
  commands/cmd_sprite_properties.cpp
  commands/cmd_sprite_size.cpp
  commands/cmd_switch_colors.cpp
  commands/cmd_undo.cpp
  commands/command.cpp
  commands/commands.cpp
  commands/fx/cmd_color_curve.cpp
  commands/fx/cmd_convolution_matrix.cpp
  commands/fx/cmd_despeckle.cpp
  commands/fx/cmd_invert_color.cpp
  commands/fx/cmd_replace_color.cpp
  commands/fx/effectbg.cpp
  core/cfg.cpp
  core/drop_files.cpp
  core/file_system.cpp
  core/modules.cpp
  dialogs/aniedit.cpp
  dialogs/drawtext.cpp
  dialogs/filesel.cpp
  dialogs/maskcol.cpp
  dialogs/playfli.cpp
  dialogs/repo.cpp
  dialogs/vectmap.cpp
  effect/colcurve.cpp
  effect/convmatr.cpp
  effect/effect.cpp
  effect/invrtcol.cpp
  effect/median.cpp
  effect/replcol.cpp
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
  modules/rootmenu.cpp
  raster/algo.cpp
  raster/algofill.cpp
  raster/algo_polygon.cpp
  raster/blend.cpp
  raster/cel.cpp
  raster/dirty.cpp
  raster/gfxobj.cpp
  raster/image.cpp
  raster/images_collector.cpp
  raster/layer.cpp
  raster/mask.cpp
  raster/palette.cpp
  raster/path.cpp
  raster/pen.cpp
  raster/quantization.cpp
  raster/rgbmap.cpp
  raster/rotate.cpp
  raster/sprite.cpp
  raster/stock.cpp
  raster/undo.cpp
  settings/ui_settings_impl.cpp
  skin/skin_theme.cpp
  skin/skin_property.cpp
  skin/skin_slider_property.cpp
  tools/tool.cpp
  tools/toolbox.cpp
  util/autocrop.cpp
  util/boundary.cpp
  util/celmove.cpp
  util/clipboard.cpp
  util/col_file.cpp
  util/filetoks.cpp
  util/misc.cpp
  util/msk_file.cpp
  util/pic_file.cpp
  util/render.cpp
  util/thmbnail.cpp
  widgets/color_bar.cpp
  widgets/color_button.cpp
  widgets/color_selector.cpp
  widgets/curvedit.cpp
  widgets/editor/click.cpp
  widgets/editor/cursor.cpp
  widgets/editor/editor.cpp
  widgets/editor/keys.cpp
  widgets/editor/pixels_movement.cpp
  widgets/fileview.cpp
  widgets/groupbut.cpp
  widgets/menuitem.cpp
  widgets/palette_view.cpp
  widgets/preview.cpp
  widgets/statebar.cpp
  widgets/tabs.cpp
  widgets/target.cpp
  widgets/toolbar.cpp)

######################################################################
# aseprite application

if(WIN32)
  set(win32_resources resources_win32.rc)
endif(WIN32)

add_executable(aseprite WIN32 main.cpp ${win32_resources})

target_link_libraries(aseprite ${all_libs})

INSTALL(TARGETS aseprite
  RUNTIME DESTINATION bin)

INSTALL(DIRECTORY ../data
  DESTINATION share/aseprite)

if(EXISTS ../docs/quickref.pdf)
  INSTALL(FILES ../docs/quickref.pdf
    DESTINATION share/aseprite/docs/quickref.pdf)
endif()

######################################################################
# Unit tests

file(GLOB_RECURSE tests *_unittest.cpp)

# Add gtest include directory so we can #include <gtest/gtest.h> in tests source code
include_directories(${CMAKE_SOURCE_DIR}/third_party/gtest/include)

foreach(testsourcefile ${tests})
  get_filename_component(testname ${testsourcefile} NAME_WE)

  add_executable(${testname} ${testsourcefile})

  target_link_libraries(${testname} gtest ${all_libs})

  add_custom_target(run_${testname}
    COMMAND ${testname})

  set(all_runs ${all_runs} run_${testname})
endforeach()

# To run tests
add_custom_target(run_all_unittests DEPENDS ${all_runs})
