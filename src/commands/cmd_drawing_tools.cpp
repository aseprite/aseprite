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

#include <allegro/unicode.h>

#include "jinete/jbase.h"

#include "commands/commands.h"
#include "modules/tools.h"

/* ======================== */
/* blur_tool               */
/* ======================== */

static bool cmd_blur_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_BLUR];
}

static void cmd_blur_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_BLUR]);
}

/* ======================== */
/* brush_tool               */
/* ======================== */

static bool cmd_brush_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_BRUSH];
}

static void cmd_brush_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_BRUSH]);
}

/* ======================== */
/* eraser_tool              */
/* ======================== */

static bool cmd_eraser_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_ERASER];
}

static void cmd_eraser_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_ERASER]);
}

/* ======================== */
/* ellipse_tool             */
/* ======================== */

static bool cmd_ellipse_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_ELLIPSE];
}

static void cmd_ellipse_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_ELLIPSE]);
}

/* ======================== */
/* floodfill_tool           */
/* ======================== */

static bool cmd_floodfill_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_FLOODFILL];
}

static void cmd_floodfill_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_FLOODFILL]);
}

/* ======================== */
/* line_tool                */
/* ======================== */

static bool cmd_line_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_LINE];
}

static void cmd_line_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_LINE]);
}

/* ======================== */
/* marker_tool              */
/* ======================== */

static bool cmd_marker_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_MARKER];
}

static void cmd_marker_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_MARKER]);
}

/* ======================== */
/* pencil_tool              */
/* ======================== */

static bool cmd_pencil_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_PENCIL];
}

static void cmd_pencil_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_PENCIL]);
}

/* ======================== */
/* rectangle_tool           */
/* ======================== */

static bool cmd_rectangle_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_RECTANGLE];
}

static void cmd_rectangle_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_RECTANGLE]);
}

/* ======================== */
/* spray_tool               */
/* ======================== */

static bool cmd_spray_tool_checked(const char *argument)
{
  return current_tool == tools_list[TOOL_SPRAY];
}

static void cmd_spray_tool_execute(const char *argument)
{
  select_tool(tools_list[TOOL_SPRAY]);
}

/* ================================================ */
/* Commands */

Command cmd_blur_tool = {
  CMD_BLUR_TOOL,
  NULL,
  cmd_blur_tool_checked,
  cmd_blur_tool_execute,
};

Command cmd_brush_tool = {
  CMD_BRUSH_TOOL,
  NULL,
  cmd_brush_tool_checked,
  cmd_brush_tool_execute,
};

Command cmd_ellipse_tool = {
  CMD_ELLIPSE_TOOL,
  NULL,
  cmd_ellipse_tool_checked,
  cmd_ellipse_tool_execute,
};

Command cmd_eraser_tool = {
  CMD_ERASER_TOOL,
  NULL,
  cmd_eraser_tool_checked,
  cmd_eraser_tool_execute,
};

Command cmd_floodfill_tool = {
  CMD_FLOODFILL_TOOL,
  NULL,
  cmd_floodfill_tool_checked,
  cmd_floodfill_tool_execute,
};

Command cmd_line_tool = {
  CMD_LINE_TOOL,
  NULL,
  cmd_line_tool_checked,
  cmd_line_tool_execute,
};

Command cmd_marker_tool = {
  CMD_MARKER_TOOL,
  NULL,
  cmd_marker_tool_checked,
  cmd_marker_tool_execute,
};

Command cmd_pencil_tool = {
  CMD_PENCIL_TOOL,
  NULL,
  cmd_pencil_tool_checked,
  cmd_pencil_tool_execute,
};

Command cmd_rectangle_tool = {
  CMD_RECTANGLE_TOOL,
  NULL,
  cmd_rectangle_tool_checked,
  cmd_rectangle_tool_execute,
};

Command cmd_spray_tool = {
  CMD_SPRAY_TOOL,
  NULL,
  cmd_spray_tool_checked,
  cmd_spray_tool_execute,
};
