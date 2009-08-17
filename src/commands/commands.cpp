/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "config.h"

/* #include <stdio.h> */
#include <string.h>
#include <allegro/unicode.h>
#include <exception>

#include "jinete/jinete.h"

#include "console.h"
#include "commands/commands.h"

extern Command cmd_about;
extern Command cmd_advanced_mode;
extern Command cmd_autocrop_sprite;
extern Command cmd_background_from_layer;
extern Command cmd_blur_tool;
extern Command cmd_brush_tool;
extern Command cmd_canvas_size;
extern Command cmd_cel_properties;
extern Command cmd_change_image_type;
extern Command cmd_clear;
extern Command cmd_close_all_files;
extern Command cmd_close_editor;
extern Command cmd_close_file;
extern Command cmd_color_curve;
extern Command cmd_configure_screen;
extern Command cmd_configure_tools;
extern Command cmd_convolution_matrix;
extern Command cmd_copy;
extern Command cmd_copy_cel;
extern Command cmd_crop_sprite;
extern Command cmd_cut;
extern Command cmd_deselect_mask;
extern Command cmd_despeckle;
extern Command cmd_duplicate_layer;
extern Command cmd_duplicate_sprite;
extern Command cmd_ellipse_tool;
extern Command cmd_eraser_tool;
extern Command cmd_exit;
extern Command cmd_eyedropper_tool;
extern Command cmd_film_editor;
extern Command cmd_flatten_layers;
extern Command cmd_flip;
extern Command cmd_floodfill_tool;
extern Command cmd_frame_properties;
extern Command cmd_goto_first_frame;
extern Command cmd_goto_last_frame;
extern Command cmd_goto_next_frame;
extern Command cmd_goto_next_layer;
extern Command cmd_goto_previous_frame;
extern Command cmd_goto_previous_layer;
extern Command cmd_invert_color;
extern Command cmd_invert_mask;
extern Command cmd_layer_from_background;
extern Command cmd_layer_properties;
extern Command cmd_line_tool;
extern Command cmd_load_mask;
extern Command cmd_make_unique_editor;
extern Command cmd_marker_tool;
extern Command cmd_mask_all;
extern Command cmd_mask_by_color;
extern Command cmd_merge_down_layer;
extern Command cmd_move_cel;
extern Command cmd_new_file;
extern Command cmd_new_frame;
extern Command cmd_new_layer;
extern Command cmd_new_layer_set;
extern Command cmd_open_file;
extern Command cmd_options;
extern Command cmd_palette_editor;
extern Command cmd_paste;
extern Command cmd_pencil_tool;
extern Command cmd_play_animation;
extern Command cmd_preview_fit_to_screen;
extern Command cmd_preview_normal;
extern Command cmd_preview_tiled;
extern Command cmd_record_screen;
extern Command cmd_rectangle_tool;
extern Command cmd_redo;
extern Command cmd_refresh;
extern Command cmd_remove_cel;
extern Command cmd_remove_frame;
extern Command cmd_remove_layer;
extern Command cmd_replace_color;
extern Command cmd_reselect_mask;
extern Command cmd_rotate_canvas;
extern Command cmd_save_file;
extern Command cmd_save_file_as;
extern Command cmd_save_file_copy_as;
extern Command cmd_save_mask;
extern Command cmd_screen_shot;
extern Command cmd_select_file;
extern Command cmd_show_grid;
extern Command cmd_snap_to_grid;
extern Command cmd_split_editor_horizontally;
extern Command cmd_split_editor_vertically;
extern Command cmd_spray_tool;
extern Command cmd_sprite_properties;
extern Command cmd_sprite_size;
extern Command cmd_switch_colors;
extern Command cmd_tips;
extern Command cmd_undo;

static Command *commands[] = {

  &cmd_about,
  &cmd_advanced_mode,
  &cmd_autocrop_sprite,
  &cmd_background_from_layer,
  &cmd_blur_tool,
  &cmd_brush_tool,
  &cmd_canvas_size,
  &cmd_cel_properties,
  &cmd_change_image_type,
  &cmd_clear,
  &cmd_close_all_files,
  &cmd_close_editor,
  &cmd_close_file,
  &cmd_color_curve,
  &cmd_configure_screen,
  &cmd_configure_tools,
  &cmd_convolution_matrix,
  &cmd_copy,
  &cmd_copy_cel,
  &cmd_crop_sprite,
  &cmd_cut,
  &cmd_deselect_mask,
  &cmd_despeckle,
  &cmd_duplicate_layer,
  &cmd_duplicate_sprite,
  &cmd_ellipse_tool,
  &cmd_eraser_tool,
  &cmd_exit,
  &cmd_eyedropper_tool,
  &cmd_film_editor,
  &cmd_flatten_layers,
  &cmd_flip,
  &cmd_floodfill_tool,
  &cmd_frame_properties,
  &cmd_goto_first_frame,
  &cmd_goto_last_frame,
  &cmd_goto_next_frame,
  &cmd_goto_next_layer,
  &cmd_goto_previous_frame,
  &cmd_goto_previous_layer,
  &cmd_invert_color,
  &cmd_invert_mask,
  &cmd_layer_from_background,
  &cmd_layer_properties,
  &cmd_line_tool,
  &cmd_load_mask,
  &cmd_make_unique_editor,
  &cmd_marker_tool,
  &cmd_mask_all,
  &cmd_mask_by_color,
  &cmd_merge_down_layer,
  &cmd_move_cel,
  &cmd_new_file,
  &cmd_new_frame,
  &cmd_new_layer,
  &cmd_new_layer_set,
  &cmd_open_file,
  &cmd_options,
  &cmd_palette_editor,
  &cmd_paste,
  &cmd_pencil_tool,
  &cmd_play_animation,
  &cmd_preview_fit_to_screen,
  &cmd_preview_normal,
  &cmd_preview_tiled,
  &cmd_record_screen,
  &cmd_rectangle_tool,
  &cmd_redo,
  &cmd_refresh,
  &cmd_remove_cel,
  &cmd_remove_frame,
  &cmd_remove_layer,
  &cmd_replace_color,
  &cmd_reselect_mask,
  &cmd_rotate_canvas,
  &cmd_save_file,
  &cmd_save_file_as,
  &cmd_save_file_copy_as,
  &cmd_save_mask,
  &cmd_screen_shot,
  &cmd_select_file,
  &cmd_show_grid,
  &cmd_snap_to_grid,
  &cmd_split_editor_horizontally,
  &cmd_split_editor_vertically,
  &cmd_spray_tool,
  &cmd_sprite_properties,
  &cmd_sprite_size,
  &cmd_switch_colors,
  &cmd_tips,
  &cmd_undo,

  NULL
};

Command *command_get_by_name(const char *name)
{
  Command **cmd;

  if (!name)
    return NULL;

  for (cmd=commands; *cmd; cmd++) {
    if (strcmp((*cmd)->name, name) == 0)
      return *cmd;
  }

  return NULL;
}

/**
 * Returns true if the current state of the program fulfills the
 * preconditions to execute this command.
 */
bool command_is_enabled(Command *command, const char *argument)
{
  try {
    if (command && command->enabled)
      return (*command->enabled)(argument);
    else
      return true;
  }
  catch (...) {
    // did the "is_enabled" throw an exception? disabled this command so...
    return false;
  }
}

bool command_is_checked(Command *command, const char *argument)
{
  try {
    if (command && command->checked)
      return (*command->checked)(argument);
  }
  catch (...) {
    // do nothing
  }
  return false;
}

/**
 * Executes the command. You can be sure that the command will be
 * executed only if it is enabled.
 */
void command_execute(Command *command, const char *argument)
{
  Console console;

  try {
    if (command && command->execute &&
	command_is_enabled(command, argument)) {
      (*command->execute)(argument);
    }
  }
  catch (ase_exception& e) {
    e.show();
  }
  catch (std::exception& e) {
    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
  catch (...) {
    console.printf("An unknown error ocurred executing the command.\n"
		   "Please try again or report this bug.\n\n"
		   "Details: Unknown exception caught.");
  }
}
