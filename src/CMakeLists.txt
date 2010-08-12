# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

# System libraries on Windows
if(WIN32)
  set(sys_libs
    User32 Shell32 ComCtl32 ComDlg32 Gdi32 Msimg32
    WinMM AdvAPI32 Ole32 ShLwApi Vfw32 WinInet PsApi
    DDraw DxGuid DSound DInput8)
endif(WIN32)

# Third-party libraries
set(libs freetype jpeg libart_lgpl libpng loadpng tinyxml vaca zlib)

# Directories where .h files can be found
include_directories(
  .. . ../third_party
  ${LIBFREETYPE_DIR}/include
  ${LIBJPEG_DIR}
  ${LIBPNG_DIR}
  ${ZLIB_DIR}
  ${LOADPNG_DIR}
  ${TINYXML_DIR}
  ${VACA_DIR}/include)

######################################################################
# aseprite application

add_executable(aseprite WIN32
  app.cpp
  ase_exception.cpp
  check_args.cpp
  console.cpp
  context.cpp
  gfxmode.cpp
  launcher.cpp
  main.cpp
  mem_utils.cpp
  recent_files.cpp
  resource_finder.cpp
  ui_context.cpp
  undoable.cpp
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
  commands/cmd_configure_screen.cpp
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
  core/color.cpp
  core/config.cpp
  core/core.cpp
  core/drop_files.cpp
  core/file_system.cpp
  core/job.cpp
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
  effect/images_ref.cpp
  effect/invrtcol.cpp
  effect/median.cpp
  effect/replcol.cpp
  file/ase_format.cpp
  file/bmp_format.cpp
  file/file.cpp
  file/fli/fli.cpp
  file/fli_format.cpp
  file/format_options.cpp
  file/gif/format.cpp
  file/gif/lzw.cpp
  file/gif_format.cpp
  file/ico_format.cpp
  file/jpeg_format.cpp
  file/pcx_format.cpp
  file/png_format.cpp
  file/tga_format.cpp
  intl/intl.cpp
  intl/msgids.cpp
  jinete/jaccel.cpp
  jinete/jalert.cpp
  jinete/jbox.cpp
  jinete/jbutton.cpp
  jinete/jclipboard.cpp
  jinete/jcombobox.cpp
  jinete/jcustom_label.cpp
  jinete/jdraw.cpp
  jinete/jentry.cpp
  jinete/jexception.cpp
  jinete/jfile.cpp
  jinete/jfilesel.cpp
  jinete/jfont.cpp
  jinete/jfontbmp.cpp
  jinete/jgrid.cpp
  jinete/jhook.cpp
  jinete/jimage.cpp
  jinete/jinete.cpp
  jinete/jintern.cpp
  jinete/jlabel.cpp
  jinete/jlink_label.cpp
  jinete/jlist.cpp
  jinete/jlistbox.cpp
  jinete/jmanager.cpp
  jinete/jmem.cpp
  jinete/jmenu.cpp
  jinete/jmessage.cpp
  jinete/jpanel.cpp
  jinete/jpopup_window.cpp
  jinete/jquickmenu.cpp
  jinete/jrect.cpp
  jinete/jregion.cpp
  jinete/jsep.cpp
  jinete/jslider.cpp
  jinete/jstream.cpp
  jinete/jstring.cpp
  jinete/jsystem.cpp
  jinete/jtextbox.cpp
  jinete/jtheme.cpp
  jinete/jthread.cpp
  jinete/jtooltips.cpp
  jinete/jview.cpp
  jinete/jwidget.cpp
  jinete/jwindow.cpp
  jinete/themes/jstandard_theme.cpp
  modules/editors.cpp
  modules/gfx.cpp
  modules/gui.cpp
  modules/palettes.cpp
  modules/rootmenu.cpp
  modules/skinneable_theme.cpp
  raster/algo.cpp
  raster/algofill.cpp
  raster/algo_polygon.cpp
  raster/blend.cpp
  raster/cel.cpp
  raster/dirty.cpp
  raster/gfxobj.cpp
  raster/image.cpp
  raster/layer.cpp
  raster/mask.cpp
  raster/palette.cpp
  raster/path.cpp
  raster/pen.cpp
  raster/quant.cpp
  raster/rgbmap.cpp
  raster/rotate.cpp
  raster/sprite.cpp
  raster/stock.cpp
  raster/undo.cpp
  settings/ui_settings_impl.cpp
  tools/tool.cpp
  tools/toolbox.cpp
  util/autocrop.cpp
  util/boundary.cpp
  util/celmove.cpp
  util/clipboard.cpp
  util/col_file.cpp
  util/filetoks.cpp
  util/hash.cpp
  util/misc.cpp
  util/msk_file.cpp
  util/pic_file.cpp
  util/quantize.cpp
  util/recscr.cpp
  util/render.cpp
  util/thmbnail.cpp
  widgets/colbar.cpp
  widgets/colbut.cpp
  widgets/colsel.cpp
  widgets/colview.cpp
  widgets/curvedit.cpp
  widgets/editor/click.cpp
  widgets/editor/cursor.cpp
  widgets/editor/editor.cpp
  widgets/editor/keys.cpp
  widgets/editor/pixels_movement.cpp
  widgets/fileview.cpp
  widgets/groupbut.cpp
  widgets/menuitem.cpp
  widgets/paledit.cpp
  widgets/preview.cpp
  widgets/statebar.cpp
  widgets/tabs.cpp
  widgets/target.cpp
  widgets/toolbar.cpp)

target_link_libraries(aseprite alleg ${libs} ${sys_libs})

# Avoid MSVCRT.lib when we use Debug mode
if(MSVC)
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
  endif(CMAKE_BUILD_TYPE STREQUAL Debug)
endif(MSVC)

set_target_properties(aseprite PROPERTIES
  COMPILE_FLAGS "-DVACA_ALLEGRO -DDEBUGMODE -DALLEGRO_STATICLINK")
