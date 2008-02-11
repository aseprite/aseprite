/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include "jinete/jinete.h"

#include "console/console.h"
#include "commands/commands.h"

/* file */
extern Command cmd_new_file;
extern Command cmd_open_file;
extern Command cmd_save_file;
extern Command cmd_save_file_as;
extern Command cmd_close_file;
extern Command cmd_close_all_files;
extern Command cmd_screen_shot;
extern Command cmd_record_screen;
extern Command cmd_about;
extern Command cmd_exit;
/* edit */
extern Command cmd_undo;
extern Command cmd_redo;
extern Command cmd_cut;
extern Command cmd_copy;
extern Command cmd_paste;
extern Command cmd_clear;
extern Command cmd_flip_horizontal;
extern Command cmd_flip_vertical;
extern Command cmd_replace_color;
extern Command cmd_invert_color;
/* view */
extern Command cmd_refresh;
extern Command cmd_configure_screen;
extern Command cmd_advanced_mode;
extern Command cmd_make_unique_editor;
extern Command cmd_split_editor_vertically;
extern Command cmd_split_editor_horizontally;
extern Command cmd_close_editor;
extern Command cmd_preview_tiled, preview;
extern Command cmd_preview_normal, preview;
extern Command cmd_preview_fit_to_screen, preview;
/* sprite */
extern Command cmd_sprite_properties;
extern Command cmd_duplicate_sprite;
extern Command cmd_change_image_type;
extern Command cmd_crop_sprite;
extern Command cmd_autocrop_sprite;
/* layer */
extern Command cmd_layer_properties;
extern Command cmd_new_layer;
extern Command cmd_new_layer_set;
extern Command cmd_remove_layer;
extern Command cmd_duplicate_layer;
extern Command cmd_merge_down_layer;
extern Command cmd_flatten_layers;
extern Command cmd_crop_layer;
/* frame */
extern Command cmd_frame_properties;
extern Command cmd_remove_frame;
extern Command cmd_new_frame;
extern Command cmd_goto_first_frame;
extern Command cmd_goto_previous_frame;
extern Command cmd_goto_next_frame;
extern Command cmd_goto_last_frame;
extern Command cmd_play_animation;
/* cel */
extern Command cmd_cel_properties;
extern Command cmd_remove_cel;
extern Command cmd_new_cel;
extern Command cmd_move_cel;
extern Command cmd_copy_cel;
extern Command cmd_link_cel;
extern Command cmd_crop_cel;
/* mask */
extern Command cmd_mask_all;
extern Command cmd_deselect_mask;
extern Command cmd_reselect_mask;
extern Command cmd_invert_mask;
extern Command cmd_mask_by_color;
extern Command cmd_load_mask;
extern Command cmd_save_mask;
/* tools */
extern Command cmd_configure_tools;
extern Command cmd_marker_tool;
extern Command cmd_dots_tool;
extern Command cmd_pencil_tool;
extern Command cmd_brush_tool;
extern Command cmd_spray_tool;
extern Command cmd_floodfill_tool;
extern Command cmd_line_tool;
extern Command cmd_rectangle_tool;
extern Command cmd_ellipse_tool;
extern Command cmd_film_editor;
extern Command cmd_palette_editor;
extern Command cmd_convolution_matrix;
extern Command cmd_color_curve;
extern Command cmd_despeckle;
extern Command cmd_run_script;
extern Command cmd_tips;
extern Command cmd_options;
/* internal */
extern Command cmd_select_file;

static Command *commands[] = {
  /* file */
  &cmd_new_file,
  &cmd_open_file,
  &cmd_save_file,
  &cmd_save_file_as,
  &cmd_close_file,
  &cmd_close_all_files,
  &cmd_screen_shot,
  &cmd_record_screen,
  &cmd_about,
  &cmd_exit,
  /* edit */
  &cmd_undo,
  &cmd_redo,
  &cmd_cut,
  &cmd_copy,
  &cmd_paste,
  &cmd_clear,
  &cmd_flip_horizontal,
  &cmd_flip_vertical,
  &cmd_replace_color,
  &cmd_invert_color,
  /* view */
  &cmd_refresh,
  &cmd_configure_screen,
  &cmd_advanced_mode,
  &cmd_make_unique_editor,
  &cmd_split_editor_vertically,
  &cmd_split_editor_horizontally,
  &cmd_close_editor,
  &cmd_preview_tiled,
  &cmd_preview_normal,
  &cmd_preview_fit_to_screen,
  /* sprite */
  &cmd_sprite_properties,
  &cmd_duplicate_sprite,
  &cmd_change_image_type,
  &cmd_crop_sprite,
  &cmd_autocrop_sprite,
  /* layer */
  &cmd_layer_properties,
  &cmd_new_layer,
  &cmd_new_layer_set,
  &cmd_remove_layer,
  &cmd_duplicate_layer,
  &cmd_merge_down_layer,
  &cmd_flatten_layers,
  &cmd_crop_layer,
  /* frame */
  &cmd_frame_properties,
  &cmd_remove_frame,
  &cmd_new_frame,
  &cmd_goto_first_frame,
  &cmd_goto_previous_frame,
  &cmd_goto_next_frame,
  &cmd_goto_last_frame,
  &cmd_play_animation,
  /* cel */
  &cmd_cel_properties,
  &cmd_remove_cel,
  &cmd_new_cel,
  &cmd_move_cel,
  &cmd_copy_cel,
  &cmd_link_cel,
  &cmd_crop_cel,
  /* mask */
  &cmd_mask_all,
  &cmd_deselect_mask,
  &cmd_reselect_mask,
  &cmd_invert_mask,
  &cmd_mask_by_color,
  &cmd_load_mask,
  &cmd_save_mask,
  /* tools */
  &cmd_configure_tools,
  &cmd_marker_tool,
  &cmd_dots_tool,
  &cmd_pencil_tool,
  &cmd_brush_tool,
  &cmd_spray_tool,
  &cmd_floodfill_tool,
  &cmd_line_tool,
  &cmd_rectangle_tool,
  &cmd_ellipse_tool,
  &cmd_film_editor,
  &cmd_palette_editor,
  &cmd_convolution_matrix,
  &cmd_color_curve,
  &cmd_despeckle,
  /*   { CMD_DRAW_TEXT, NULL, NULL, NULL, NULL }, */
  /*   { CMD_PLAY_FLIC, NULL, NULL, NULL, NULL }, */
  /*   { CMD_MAPGEN, NULL, NULL, NULL, NULL }, */
  &cmd_run_script,
  &cmd_tips,
  &cmd_options,
  /* internal */
  &cmd_select_file,
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

Command *command_get_by_key(JMessage msg)
{
  Command **cmd;

  for (cmd=commands; *cmd; cmd++) {
    if (command_is_key_pressed(*cmd, msg))
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
  if (command && command->enabled)
    return (*command->enabled)(argument);
  else
    return TRUE;
}

bool command_is_checked(Command *command, const char *argument)
{
  if (command && command->checked)
    return (*command->checked)(argument);
  else
    return FALSE;
}

/**
 * Executes the command. You can be sure that the command will be
 * executed only if it is enabled.
 */
void command_execute(Command *command, const char *argument)
{
  console_open();

  if (command && command->execute &&
      command_is_enabled(command, argument)) {
    (*command->execute)(argument);
  }

  console_close();
}

bool command_is_key_pressed(Command *command, JMessage msg)
{
  if (command->accel) {
    return jaccel_check(command->accel,
			msg->any.shifts,
			msg->key.ascii,
			msg->key.scancode);
  }
  return FALSE;
}

void command_add_key(Command *command, const char *string)
{
  char buf[256];

  if (!command->accel)
    command->accel = jaccel_new();

  usprintf(buf, "<%s>", string);
  jaccel_add_keys_from_string(command->accel, buf);
}

void command_reset_keys()
{
  Command **cmd;

  for (cmd=commands; *cmd; cmd++) {
    if ((*cmd)->accel) {
      jaccel_free((*cmd)->accel);
      (*cmd)->accel = NULL;
    }
  }
}
