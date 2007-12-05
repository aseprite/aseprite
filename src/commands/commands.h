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

#ifndef COMMANDS_COMMANDS_H
#define COMMANDS_COMMANDS_H

#include "jinete/jbase.h"

/* file */
#define CMD_NEW_FILE			"new_file"
#define CMD_OPEN_FILE			"open_file"
#define CMD_SAVE_FILE			"save_file"
#define CMD_SAVE_FILE_AS		"save_file_as"
#define CMD_CLOSE_FILE			"close_file"
#define CMD_CLOSE_ALL_FILES		"close_all_files"
#define CMD_SCREEN_SHOT			"screen_shot"
#define CMD_RECORD_SCREEN		"record_screen"
#define CMD_ABOUT			"about"
#define CMD_EXIT			"exit"
/* edit */
#define CMD_UNDO			"undo"
#define CMD_REDO			"redo"
#define CMD_CUT				"cut"
#define CMD_COPY			"copy"
#define CMD_PASTE			"paste"
#define CMD_CLEAR			"clear"
#define CMD_FLIP_HORIZONTAL		"flip_horizontal"
#define CMD_FLIP_VERTICAL		"flip_vertical"
#define CMD_REPLACE_COLOR		"replace_color"
#define CMD_INVERT_COLOR		"invert_color"
/* view */
#define CMD_REFRESH			"refresh"
#define CMD_CONFIGURE_SCREEN		"configure_screen"
#define CMD_ADVANCED_MODE		"advanced_mode"
#define CMD_MAKE_UNIQUE_EDITOR		"make_unique_editor"
#define CMD_SPLIT_EDITOR_VERTICALLY	"split_editor_vertically"
#define CMD_SPLIT_EDITOR_HORIZONTALLY	"split_editor_horizontally"
#define CMD_CLOSE_EDITOR		"close_editor"
#define CMD_PREVIEW_TILED		"preview_tiled"
#define CMD_PREVIEW_NORMAL		"preview_normal"
#define CMD_PREVIEW_FIT_TO_SCREEN	"preview_fit_to_screen"
/* sprite */
#define CMD_SPRITE_PROPERTIES		"sprite_properties"
#define CMD_DUPLICATE_SPRITE		"duplicate_sprite"
#define CMD_CHANGE_IMAGE_TYPE		"change_image_type"
#define CMD_CROP_SPRITE			"crop_sprite"
#define CMD_AUTOCROP_SPRITE		"autocrop_sprite"
/* layer */
#define CMD_LAYER_PROPERTIES		"layer_properties"
#define CMD_NEW_LAYER			"new_layer"
#define CMD_NEW_LAYER_SET		"new_layer_set"
#define CMD_REMOVE_LAYER		"remove_layer"
#define CMD_DUPLICATE_LAYER		"duplicate_layer"
#define CMD_MERGE_DOWN_LAYER		"merge_down_layer"
#define CMD_FLATTEN_LAYERS		"flatten_layers"
#define CMD_CROP_LAYER			"crop_layer"
/* frame */
#define CMD_FRAME_PROPERTIES		"frame_properties"
#define CMD_NEW_FRAME			"new_frame"
#define CMD_REMOVE_FRAME		"remove_frame"
/* cel */
#define CMD_CEL_PROPERTIES		"cel_properties"
#define CMD_REMOVE_CEL			"remove_cel"
#define CMD_NEW_CEL			"new_cel"
#define CMD_MOVE_CEL			"move_cel"
#define CMD_COPY_CEL			"copy_cel"
#define CMD_LINK_CEL			"link_cel"
#define CMD_CROP_CEL			"crop_cel"
/* select */
#define CMD_MASK_ALL			"mask_all"
#define CMD_DESELECT_MASK		"deselect_mask"
#define CMD_RESELECT_MASK		"reselect_mask"
#define CMD_INVERT_MASK			"invert_mask"
#define CMD_MASK_BY_COLOR		"mask_by_color"
#define CMD_LOAD_MASK			"load_mask"
#define CMD_SAVE_MASK			"save_mask"
/* tools */
#define CMD_CONFIGURE_TOOLS		"configure_tools"
#define CMD_MARKER_TOOL			"marker_tool"
#define CMD_DOTS_TOOL			"dots_tool"
#define CMD_PENCIL_TOOL			"pencil_tool"
#define CMD_BRUSH_TOOL			"brush_tool"
#define CMD_SPRAY_TOOL			"spray_tool"
#define CMD_FLOODFILL_TOOL		"floodfill_tool"
#define CMD_LINE_TOOL			"line_tool"
#define CMD_RECTANGLE_TOOL		"rectangle_tool"
#define CMD_ELLIPSE_TOOL		"ellipse_tool"
#define CMD_FILM_EDITOR			"film_editor"
#define CMD_PALETTE_EDITOR		"palette_editor"
#define CMD_CONVOLUTION_MATRIX		"convolution_matrix"
#define CMD_COLOR_CURVE			"color_curve"
#define CMD_DESPECKLE			"despeckle"
/* #define CMD_DRAW_TEXT			"draw_text" */
/* #define CMD_PLAY_FLIC			"play_flic" */
/* #define CMD_MAPGEN			"mapgen" */
#define CMD_RUN_SCRIPT			"run_script"
#define CMD_TIPS			"tips"
#define CMD_OPTIONS			"options"
/* internal commands */
#define CMD_SELECT_FILE			"select_file"


typedef struct Command Command;

struct Command
{
  const char *name;
  bool (*enabled)(const char *argument); /* preconditions to execute the command */
  bool (*checked)(const char *argument); /* should the menu-item be checked? */
  void (*execute)(const char *argument); /* execute the command (after check the preconditions) */
  JAccel accel;
};

Command *command_get_by_name(const char *name);
Command *command_get_by_key(JMessage msg);

bool command_is_enabled(Command *command, const char *argument);
bool command_is_checked(Command *command, const char *argument);
void command_execute(Command *command, const char *argument);

bool command_is_key_pressed(Command *command, JMessage msg);
void command_add_key(Command *command, const char *string);
void command_reset_keys();

#endif /* COMMANDS_COMMANDS_H */
