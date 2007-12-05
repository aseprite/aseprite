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

#include <allegro/unicode.h>

#include "jinete/jbase.h"

#include "commands/commands.h"
#include "modules/tools.h"

#endif

/* ======================== */
/* brush_tool               */
/* ======================== */

static bool cmd_brush_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_brush;
}

static void cmd_brush_tool_execute(const char *argument)
{
  select_tool(&ase_tool_brush);
}

/* ======================== */
/* dots_tool                */
/* ======================== */

static bool cmd_dots_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_dots;
}

static void cmd_dots_tool_execute(const char *argument)
{
  select_tool(&ase_tool_dots);
}

/* ======================== */
/* ellipse_tool             */
/* ======================== */

static bool cmd_ellipse_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_ellipse;
}

static void cmd_ellipse_tool_execute(const char *argument)
{
  select_tool(&ase_tool_ellipse);
}

/* ======================== */
/* floodfill_tool           */
/* ======================== */

static bool cmd_floodfill_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_floodfill;
}

static void cmd_floodfill_tool_execute(const char *argument)
{
  select_tool(&ase_tool_floodfill);
}

/* ======================== */
/* line_tool                */
/* ======================== */

static bool cmd_line_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_line;
}

static void cmd_line_tool_execute(const char *argument)
{
  select_tool(&ase_tool_line);
}

/* ======================== */
/* marker_tool              */
/* ======================== */

static bool cmd_marker_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_marker;
}

static void cmd_marker_tool_execute(const char *argument)
{
  select_tool(&ase_tool_marker);
}

/* ======================== */
/* pencil_tool              */
/* ======================== */

static bool cmd_pencil_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_pencil;
}

static void cmd_pencil_tool_execute(const char *argument)
{
  select_tool(&ase_tool_pencil);
}

/* ======================== */
/* rectangle_tool           */
/* ======================== */

static bool cmd_rectangle_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_rectangle;
}

static void cmd_rectangle_tool_execute(const char *argument)
{
  select_tool(&ase_tool_rectangle);
}

/* ======================== */
/* spray_tool               */
/* ======================== */

static bool cmd_spray_tool_checked(const char *argument)
{
  return current_tool && current_tool == &ase_tool_spray;
}

static void cmd_spray_tool_execute(const char *argument)
{
  select_tool(&ase_tool_spray);
}

/* ================================================ */
/* Commands */

Command cmd_brush_tool = {
  CMD_BRUSH_TOOL,
  NULL,
  cmd_brush_tool_checked,
  cmd_brush_tool_execute,
  NULL
};

Command cmd_dots_tool = {
  CMD_DOTS_TOOL,
  NULL,
  cmd_dots_tool_checked,
  cmd_dots_tool_execute,
  NULL
};

Command cmd_ellipse_tool = {
  CMD_ELLIPSE_TOOL,
  NULL,
  cmd_ellipse_tool_checked,
  cmd_ellipse_tool_execute,
  NULL
};

Command cmd_floodfill_tool = {
  CMD_FLOODFILL_TOOL,
  NULL,
  cmd_floodfill_tool_checked,
  cmd_floodfill_tool_execute,
  NULL
};

Command cmd_line_tool = {
  CMD_LINE_TOOL,
  NULL,
  cmd_line_tool_checked,
  cmd_line_tool_execute,
  NULL
};

Command cmd_marker_tool = {
  CMD_MARKER_TOOL,
  NULL,
  cmd_marker_tool_checked,
  cmd_marker_tool_execute,
  NULL
};

Command cmd_pencil_tool = {
  CMD_PENCIL_TOOL,
  NULL,
  cmd_pencil_tool_checked,
  cmd_pencil_tool_execute,
  NULL
};

Command cmd_rectangle_tool = {
  CMD_RECTANGLE_TOOL,
  NULL,
  cmd_rectangle_tool_checked,
  cmd_rectangle_tool_execute,
  NULL
};

Command cmd_spray_tool = {
  CMD_SPRAY_TOOL,
  NULL,
  cmd_spray_tool_checked,
  cmd_spray_tool_execute,
  NULL
};
