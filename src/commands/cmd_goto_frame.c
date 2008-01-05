/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008  David A. Capello
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

#include "commands/commands.h"
#include "modules/gui.h"
#include "modules/editors.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "widgets/editor.h"

#endif

/* ======================== */
/* goto_first_frame         */
/* ======================== */

static bool cmd_goto_first_frame_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_goto_first_frame_execute(const char *argument)
{
  current_sprite->frame = 0;

  update_screen_for_sprite(current_sprite);
  editor_update_status_bar_for_standby(current_editor);
}

/* ======================== */
/* goto_previous_frame      */
/* ======================== */

static bool cmd_goto_previous_frame_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_goto_previous_frame_execute(const char *argument)
{
  if (current_sprite->frame > 0)
    current_sprite->frame--;
  else
    current_sprite->frame = current_sprite->frames-1;

  update_screen_for_sprite(current_sprite);
  editor_update_status_bar_for_standby(current_editor);
}

/* ======================== */
/* goto_next_frame          */
/* ======================== */

static bool cmd_goto_next_frame_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_goto_next_frame_execute(const char *argument)
{
  if (current_sprite->frame < current_sprite->frames-1)
    current_sprite->frame++;
  else
    current_sprite->frame = 0;

  update_screen_for_sprite(current_sprite);
  editor_update_status_bar_for_standby(current_editor);
}

/* ======================== */
/* goto_last_frame          */
/* ======================== */

static bool cmd_goto_last_frame_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_goto_last_frame_execute(const char *argument)
{
  current_sprite->frame = current_sprite->frames-1;

  update_screen_for_sprite(current_sprite);
  editor_update_status_bar_for_standby(current_editor);
}

Command cmd_goto_first_frame = {
  CMD_GOTO_FIRST_FRAME,
  cmd_goto_first_frame_enabled,
  NULL,
  cmd_goto_first_frame_execute,
};

Command cmd_goto_previous_frame = {
  CMD_GOTO_PREVIOUS_FRAME,
  cmd_goto_previous_frame_enabled,
  NULL,
  cmd_goto_previous_frame_execute,
};

Command cmd_goto_next_frame = {
  CMD_GOTO_NEXT_FRAME,
  cmd_goto_next_frame_enabled,
  NULL,
  cmd_goto_next_frame_execute,
};

Command cmd_goto_last_frame = {
  CMD_GOTO_LAST_FRAME,
  cmd_goto_last_frame_enabled,
  NULL,
  cmd_goto_last_frame_execute,
};
