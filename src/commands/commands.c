/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

/* #include <stdio.h> */
#include <string.h>
#include <allegro/unicode.h>

#include "jinete.h"

#include "console/console.h"
#include "commands/commands.h"

#endif

/********************************************************************/
/* All commands */

/* file */
void command_execute_new_file(const char *argument);
void command_execute_new_file(const char *argument);
void command_execute_open_file(const char *argument);
bool command_enabled_save_file(const char *argument);
void command_execute_save_file(const char *argument);
bool command_enabled_save_file_as(const char *argument);
void command_execute_save_file_as(const char *argument);
bool command_enabled_close_file(const char *argument);
void command_execute_close_file(const char *argument);
bool command_enabled_close_all_files(const char *argument);
void command_execute_close_all_files(const char *argument);
void command_execute_screen_shot(const char *argument);
bool command_checked_record_screen(const char *argument);
void command_execute_record_screen(const char *argument);
void command_execute_about(const char *argument);
void command_execute_exit(const char *argument);
/* edit */
bool command_enabled_undo(const char *argument);
void command_execute_undo(const char *argument);
bool command_enabled_redo(const char *argument);
void command_execute_redo(const char *argument);
bool command_enabled_cut(const char *argument);
void command_execute_cut(const char *argument);
bool command_enabled_copy(const char *argument);
void command_execute_copy(const char *argument);
bool command_enabled_paste(const char *argument);
void command_execute_paste(const char *argument);
bool command_enabled_clear(const char *argument);
void command_execute_clear(const char *argument);
bool command_enabled_flip_horizontal(const char *argument);
void command_execute_flip_horizontal(const char *argument);
bool command_enabled_flip_vertical(const char *argument);
void command_execute_flip_vertical(const char *argument);
bool command_enabled_replace_color(const char *argument);
void command_execute_replace_color(const char *argument);
bool command_enabled_invert_color(const char *argument);
void command_execute_invert_color(const char *argument);
void command_execute_refresh(const char *argument);
void command_execute_configure_screen(const char *argument);
void command_execute_advanced_mode(const char *argument);
void command_execute_make_unique_editor(const char *argument);
void command_execute_split_editor_vertically(const char *argument);
void command_execute_split_editor_horizontally(const char *argument);
void command_execute_close_editor(const char *argument);
bool command_enabled_preview(const char *argument);
void command_execute_preview_fit_to_screen(const char *argument);
void command_execute_preview_normal(const char *argument);
void command_execute_preview_tiled(const char *argument);
/* sprite */
bool command_enabled_sprite_properties(const char *argument);
void command_execute_sprite_properties(const char *argument);
bool command_enabled_duplicate_sprite(const char *argument);
void command_execute_duplicate_sprite(const char *argument);
bool command_enabled_change_image_type(const char *argument);
void command_execute_change_image_type(const char *argument);
bool command_enabled_crop_sprite(const char *argument);
void command_execute_crop_sprite(const char *argument);
bool command_enabled_autocrop_sprite(const char *argument);
void command_execute_autocrop_sprite(const char *argument);
/* layer */
bool command_enabled_layer_properties(const char *argument);
void command_execute_layer_properties(const char *argument);
bool command_enabled_new_layer(const char *argument);
void command_execute_new_layer(const char *argument);
bool command_enabled_new_layer_set(const char *argument);
void command_execute_new_layer_set(const char *argument);
bool command_enabled_remove_layer(const char *argument);
void command_execute_remove_layer(const char *argument);
bool command_enabled_duplicate_layer(const char *argument);
void command_execute_duplicate_layer(const char *argument);
bool command_enabled_merge_down_layer(const char *argument);
void command_execute_merge_down_layer(const char *argument);
bool command_enabled_flatten_layers(const char *argument);
void command_execute_flatten_layers(const char *argument);
bool command_enabled_crop_layer(const char *argument);
void command_execute_crop_layer(const char *argument);
/* frame */
bool command_enabled_frame_properties(const char *argument);
void command_execute_frame_properties(const char *argument);
bool command_enabled_remove_frame(const char *argument);
void command_execute_remove_frame(const char *argument);
bool command_enabled_new_frame(const char *argument);
void command_execute_new_frame(const char *argument);
/* cel */
bool command_enabled_cel_properties(const char *argument);
void command_execute_cel_properties(const char *argument);
bool command_enabled_remove_cel(const char *argument);
void command_execute_remove_cel(const char *argument);
bool command_enabled_new_cel(const char *argument);
void command_execute_new_cel(const char *argument);
bool command_enabled_move_cel(const char *argument);
void command_execute_move_cel(const char *argument);
bool command_enabled_copy_cel(const char *argument);
void command_execute_copy_cel(const char *argument);
bool command_enabled_link_cel(const char *argument);
void command_execute_link_cel(const char *argument);
bool command_enabled_crop_cel(const char *argument);
void command_execute_crop_cel(const char *argument);
/* select */
bool command_enabled_mask_all(const char *argument);
void command_execute_mask_all(const char *argument);
bool command_enabled_deselect_mask(const char *argument);
void command_execute_deselect_mask(const char *argument);
bool command_enabled_reselect_mask(const char *argument);
void command_execute_reselect_mask(const char *argument);
bool command_enabled_invert_mask(const char *argument);
void command_execute_invert_mask(const char *argument);
bool command_enabled_mask_by_color(const char *argument);
void command_execute_mask_by_color(const char *argument);
bool command_enabled_load_mask(const char *argument);
void command_execute_load_mask(const char *argument);
bool command_enabled_save_mask(const char *argument);
void command_execute_save_mask(const char *argument);
/* tools */
void command_execute_configure_tools(const char *argument);
bool command_checked_marker_tool(const char *argument);
void command_execute_marker_tool(const char *argument);
bool command_checked_dots_tool(const char *argument);
void command_execute_dots_tool(const char *argument);
bool command_checked_pencil_tool(const char *argument);
void command_execute_pencil_tool(const char *argument);
bool command_checked_brush_tool(const char *argument);
void command_execute_brush_tool(const char *argument);
bool command_checked_spray_tool(const char *argument);
void command_execute_spray_tool(const char *argument);
bool command_checked_floodfill_tool(const char *argument);
void command_execute_floodfill_tool(const char *argument);
bool command_checked_line_tool(const char *argument);
void command_execute_line_tool(const char *argument);
bool command_checked_rectangle_tool(const char *argument);
void command_execute_rectangle_tool(const char *argument);
bool command_checked_ellipse_tool(const char *argument);
void command_execute_ellipse_tool(const char *argument);
bool command_enabled_film_editor(const char *argument);
void command_execute_film_editor(const char *argument);
void command_execute_palette_editor(const char *argument);
bool command_enabled_convolution_matrix(const char *argument);
void command_execute_convolution_matrix(const char *argument);
bool command_enabled_color_curve(const char *argument);
void command_execute_color_curve(const char *argument);
bool command_enabled_despeckle(const char *argument);
void command_execute_despeckle(const char *argument);
/* void command_execute_draw_text(const char *argument); */
/* void command_execute_play_flic(const char *argument); */
void command_execute_run_script(const char *argument);
void command_execute_tips(const char *argument);
void command_execute_options(const char *argument);
/* internal commands */
bool command_enabled_select_file(const char *argument);
bool command_checked_select_file(const char *argument);
void command_execute_select_file(const char *argument);

/* EXE=execute, ENA=enabled, CHK=checked */
#define CMD_EXE(name)		{ #name, NULL, NULL, command_execute_##name, NULL }
#define CMD_EXE_ENA(name)	{ #name, command_enabled_##name, NULL, command_execute_##name, NULL }
#define CMD_EXE_ENA2(name,name2){ #name, command_enabled_##name2, NULL, command_execute_##name, NULL }
#define CMD_EXE_ENA_CHK(name)	{ #name, command_enabled_##name, command_checked_##name, command_execute_##name, NULL }
#define CMD_EXE_CHK(name)	{ #name, NULL, command_checked_##name, command_execute_##name, NULL }

static Command commands[] = {
  /* file */
  CMD_EXE(new_file),
  CMD_EXE(open_file),
  CMD_EXE_ENA(save_file),
  CMD_EXE_ENA(save_file_as),
  CMD_EXE_ENA(close_file),
  CMD_EXE_ENA(close_all_files),
  CMD_EXE(screen_shot),
  CMD_EXE_CHK(record_screen),
  CMD_EXE(about),
  CMD_EXE(exit),
  /* edit */
  CMD_EXE_ENA(undo),
  CMD_EXE_ENA(redo),
  CMD_EXE_ENA(cut),
  CMD_EXE_ENA(copy),
  CMD_EXE_ENA(paste),
  CMD_EXE_ENA(clear),
  CMD_EXE_ENA(flip_horizontal),
  CMD_EXE_ENA(flip_vertical),
  CMD_EXE_ENA(replace_color),
  CMD_EXE_ENA(invert_color),
  /* view */
  CMD_EXE(refresh),
  CMD_EXE(configure_screen),
  CMD_EXE(advanced_mode),
  CMD_EXE(make_unique_editor),
  CMD_EXE(split_editor_vertically),
  CMD_EXE(split_editor_horizontally),
  CMD_EXE(close_editor),
  CMD_EXE_ENA2(preview_tiled, preview),
  CMD_EXE_ENA2(preview_normal, preview),
  CMD_EXE_ENA2(preview_fit_to_screen, preview),
  /* sprite */
  CMD_EXE_ENA(sprite_properties),
  CMD_EXE_ENA(duplicate_sprite),
  CMD_EXE_ENA(change_image_type),
  CMD_EXE_ENA(crop_sprite),
  CMD_EXE_ENA(autocrop_sprite),
  /* layer */
  CMD_EXE_ENA(layer_properties),
  CMD_EXE_ENA(new_layer),
  CMD_EXE_ENA(new_layer_set),
  CMD_EXE_ENA(remove_layer),
  CMD_EXE_ENA(duplicate_layer),
  CMD_EXE_ENA(merge_down_layer),
  CMD_EXE_ENA(flatten_layers),
  CMD_EXE_ENA(crop_layer),
  /* frame */
  CMD_EXE_ENA(frame_properties),
  CMD_EXE_ENA(remove_frame),
  CMD_EXE_ENA(new_frame),
  /* cel */
  CMD_EXE_ENA(cel_properties),
  CMD_EXE_ENA(remove_cel),
  CMD_EXE_ENA(new_cel),
  CMD_EXE_ENA(move_cel),
  CMD_EXE_ENA(copy_cel),
  CMD_EXE_ENA(link_cel),
  CMD_EXE_ENA(crop_cel),
  /* mask */
  CMD_EXE_ENA(mask_all),
  CMD_EXE_ENA(deselect_mask),
  CMD_EXE_ENA(reselect_mask),
  CMD_EXE_ENA(invert_mask),
  CMD_EXE_ENA(mask_by_color),
  CMD_EXE_ENA(load_mask),
  CMD_EXE_ENA(save_mask),
  /* tools */
  CMD_EXE(configure_tools),
  CMD_EXE_CHK(marker_tool),
  CMD_EXE_CHK(dots_tool),
  CMD_EXE_CHK(pencil_tool),
  CMD_EXE_CHK(brush_tool),
  CMD_EXE_CHK(spray_tool),
  CMD_EXE_CHK(floodfill_tool),
  CMD_EXE_CHK(line_tool),
  CMD_EXE_CHK(rectangle_tool),
  CMD_EXE_CHK(ellipse_tool),
  CMD_EXE_ENA(film_editor),
  { CMD_PALETTE_EDITOR, NULL, NULL, NULL, NULL },
  CMD_EXE_ENA(convolution_matrix),
  CMD_EXE_ENA(color_curve),
  CMD_EXE_ENA(despeckle),
/*   { CMD_DRAW_TEXT, NULL, NULL, NULL, NULL }, */
/*   { CMD_PLAY_FLIC, NULL, NULL, NULL, NULL }, */
/*   { CMD_MAPGEN, NULL, NULL, NULL, NULL }, */
  CMD_EXE(run_script),
  CMD_EXE(tips),
  CMD_EXE(options),
  /* internal */
  CMD_EXE_ENA_CHK(select_file),
  { NULL, NULL, NULL, NULL, NULL }

};

Command *command_get_by_name(const char *name)
{
  Command *cmd;

  if (!name)
    return NULL;

  for (cmd=commands; cmd->name; cmd++) {
    if (strcmp(cmd->name, name) == 0)
      return cmd;
  }

  return NULL;
}

Command *command_get_by_key(JMessage msg)
{
  Command *cmd;

  for (cmd=commands; cmd->name; cmd++) {
    if (command_is_key_pressed(cmd, msg))
      return cmd;
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
  Command *cmd;

  for (cmd=commands; cmd->name; cmd++) {
    if (cmd->accel) {
      jaccel_free(cmd->accel);
      cmd->accel = NULL;
    }
  }
}
